/* MQTT Mutual Authentication Example Wrapper for Arduino */

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

// Arduino core includes (replaces some ESP-IDF specific ones)
#include <Arduino.h>

#include "mqtt_manager.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "esp_log.h"
#include "mqtt_client.h"

static const char *TAG = "mqtt_manager";

#define MQTT_MANAGER_MAX_TOPIC_LEN 128
#define MQTT_MANAGER_MAX_PAYLOAD_LEN 512
#define MQTT_MANAGER_QUEUE_LENGTH 8

typedef struct {
    mqtt_manager_action_t action;
    int qos;
    bool retain;
    int topic_len;
    int payload_len;
    char topic[MQTT_MANAGER_MAX_TOPIC_LEN];
    char payload[MQTT_MANAGER_MAX_PAYLOAD_LEN];
} mqtt_manager_action_item_t;

static esp_mqtt_client_handle_t s_client = NULL;
static QueueHandle_t s_action_queue = NULL;
static bool s_connected = false;
static mqtt_manager_message_cb_t s_on_message = NULL;
static char s_broker_uri[128];

/*
 * NOTE: The old 'extern const uint8_t ... asm()' declarations have been REMOVED.
 * Certificates are now passed directly into mqtt_manager_init() as standard strings.
 */

static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0) {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}

static void mqtt_manager_do_action(const mqtt_manager_action_item_t *item)
{
    if (!s_client || !item) {
        return;
    }

    switch (item->action) {
    case MQTT_MANAGER_ACTION_SUBSCRIBE:
        esp_mqtt_client_subscribe(s_client, item->topic, item->qos);
        break;
    case MQTT_MANAGER_ACTION_UNSUBSCRIBE:
        esp_mqtt_client_unsubscribe(s_client, item->topic);
        break;
    case MQTT_MANAGER_ACTION_PUBLISH:
        esp_mqtt_client_publish(s_client,
                                item->topic,
                                item->payload,
                                item->payload_len,
                                item->qos,
                                item->retain);
        break;
    default:
        ESP_LOGW(TAG, "Unknown action: %d", item->action);
        break;
    }
}

static void mqtt_manager_flush_queue(void)
{
    if (!s_action_queue) {
        return;
    }

    mqtt_manager_action_item_t item = {};
    while (xQueueReceive(s_action_queue, &item, 0) == pdTRUE) {
        mqtt_manager_do_action(&item);
    }
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32, base, event_id);
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
    
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        s_connected = true;
        mqtt_manager_flush_queue();
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        s_connected = false;
        break;
    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        if (s_on_message) {
            // Forwarding to the user-defined callback
            s_on_message(event->topic, event->topic_len, event->data, event->data_len);
        }
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
            ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));
        }
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

static void mqtt_manager_build_uri(const char *endpoint, char *out, size_t out_len)
{
    if (!endpoint || !out || out_len == 0) {
        return;
    }

    if (strstr(endpoint, "://") != NULL) {
        snprintf(out, out_len, "%s", endpoint);
    } else {
        snprintf(out, out_len, "mqtts://%s:8883", endpoint);
    }
}

// UPDATE: Now accepts the certificates as string parameters
esp_err_t mqtt_manager_init(const char *endpoint, const char* root_ca, const char* client_cert, const char* client_key)
{
    if (!endpoint || endpoint[0] == '\0') {
        return ESP_ERR_INVALID_ARG;
    }
    if (s_client) {
        return ESP_ERR_INVALID_STATE;
    }

    mqtt_manager_build_uri(endpoint, s_broker_uri, sizeof(s_broker_uri));

    // UPDATE: We pass the string pointers directly to the config struct
    const esp_mqtt_client_config_t mqtt_cfg = {
        .broker = {
            .address = {
                .uri = s_broker_uri
            },
            .verification = {
                .certificate = root_ca
            }
        },
        .credentials = {
            .authentication = {
                .certificate = client_cert,
                .key = client_key
            }
        }
    };

    if (!s_action_queue) {
        s_action_queue = xQueueCreate(MQTT_MANAGER_QUEUE_LENGTH, sizeof(mqtt_manager_action_item_t));
        if (!s_action_queue) {
            return ESP_ERR_NO_MEM;
        }
    }

    ESP_LOGI(TAG, "[MQTT] Free memory: %" PRIu32 " bytes", esp_get_free_heap_size());
    s_client = esp_mqtt_client_init(&mqtt_cfg);
    if (!s_client) {
        return ESP_FAIL;
    }

    esp_mqtt_client_register_event(s_client, MQTT_EVENT_ANY, mqtt_event_handler, NULL);
    return esp_mqtt_client_start(s_client);
}

void mqtt_manager_set_message_cb(mqtt_manager_message_cb_t cb)
{
    s_on_message = cb;
}

esp_err_t mqtt_manager_action(mqtt_manager_action_t action,
                              const char *topic,
                              const char *message)
{
    if (!s_client || !topic || topic[0] == '\0') {
        return ESP_ERR_INVALID_STATE;
    }

    mqtt_manager_action_item_t item = {
        .action = action,
        .qos = 0,
        .retain = false,
        .topic_len = 0,
        .payload_len = 0,
    };

    item.topic_len = strnlen(topic, MQTT_MANAGER_MAX_TOPIC_LEN - 1);
    memcpy(item.topic, topic, item.topic_len);
    item.topic[item.topic_len] = '\0';

    if (action == MQTT_MANAGER_ACTION_PUBLISH) {
        if (!message) {
            message = "";
        }
        item.payload_len = strnlen(message, MQTT_MANAGER_MAX_PAYLOAD_LEN - 1);
        memcpy(item.payload, message, item.payload_len);
        item.payload[item.payload_len] = '\0';
    }

    if (s_connected) {
        mqtt_manager_do_action(&item);
        return ESP_OK;
    }

    if (!s_action_queue) {
        return ESP_ERR_INVALID_STATE;
    }

    if (xQueueSend(s_action_queue, &item, 0) != pdTRUE) {
        return ESP_ERR_NO_MEM;
    }

    return ESP_OK;
}