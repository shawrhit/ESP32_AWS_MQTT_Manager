#pragma once

#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    MQTT_MANAGER_ACTION_SUBSCRIBE = 0,
    MQTT_MANAGER_ACTION_UNSUBSCRIBE,
    MQTT_MANAGER_ACTION_PUBLISH,
} mqtt_manager_action_t;

typedef void (*mqtt_manager_message_cb_t)(const char *topic,
                                          int topic_len,
                                          const char *data,
                                          int data_len);

// Initialize MQTT client with the AWS IoT endpoint and certificate strings.
esp_err_t mqtt_manager_init(const char *endpoint, 
                            const char *root_ca, 
                            const char *client_cert, 
                            const char *client_key);

// Register a callback for inbound messages.
void mqtt_manager_set_message_cb(mqtt_manager_message_cb_t cb);

// Perform an MQTT action. For publish, message can be text or JSON.
esp_err_t mqtt_manager_action(mqtt_manager_action_t action,
                              const char *topic,
                              const char *message);

#ifdef __cplusplus
}
#endif