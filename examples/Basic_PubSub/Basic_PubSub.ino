#include <WiFi.h>
#include <time.h>
#include "esp_log.h" // Exposes hidden system logs for troubleshooting
#include "mqtt_manager.h"
#include "aws_certs.h"  // This contains all your certificates and must be in the same folder as this sketch

// ===================================================
// Configuration - Update these with your details!
// ===================================================
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
const char* aws_endpoint = "mqtts://YOUR_ENDPOINT-ats.iot.YOUR_REGION.amazonaws.com:8883";

// Define your MQTT topics
const char* SUB_TOPIC = "device/inbound";
const char* PUB_TOPIC = "device/outbound";

// ===================================================
// Global Variables for Asynchronous Receiving
// ===================================================
volatile bool incoming_data_flag = false;
String received_topic = "";
String received_payload = "";
const unsigned long publish_interval = 10000; // Publish every 10 seconds

// ===================================================
// Callback Function for Incoming Messages
// ===================================================
static void on_msg(const char *topic, int topic_len, const char *data, int data_len) {
  received_topic = String(topic).substring(0, topic_len);
  received_payload = String(data).substring(0, data_len);
  incoming_data_flag = true; 
}

// ===================================================
// Setup
// ===================================================
void setup() {
  Serial.begin(115200);
  delay(1000); 
  
  // Force ESP-IDF logs on to help users debug TLS/Certificate issues
  esp_log_level_set("mqtt_client", ESP_LOG_INFO);
  esp_log_level_set("mqtt_manager", ESP_LOG_INFO);
  esp_log_level_set("esp-tls", ESP_LOG_INFO);

  Serial.println("\n--- Starting AWS IoT Setup ---");

  // 1. Connect to Wi-Fi
  Serial.print("Connecting to Wi-Fi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println("\n✅ Wi-Fi Connected!");

  // 2. Sync Time (CRITICAL FOR AWS TLS CERTIFICATES)
  Serial.print("Syncing time via NTP...");
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  struct tm timeinfo;
  while (!getLocalTime(&timeinfo)) { delay(1000); Serial.print("."); }
  Serial.println("\nTime synced!");

  // 3. Initialize MQTT Manager
  mqtt_manager_set_message_cb(on_msg);
  esp_err_t init_status = mqtt_manager_init(aws_endpoint, AWS_ROOT_CA, CLIENT_CERT, CLIENT_PRIVATE_KEY);
  
  if (init_status == ESP_OK) {
    Serial.println("MQTT Manager Initialized");
  } else {
    Serial.printf("MQTT Init Failed with code: %d\n", init_status);
  }
  
  // 4. Subscribe to the inbound topic
  mqtt_manager_action(MQTT_MANAGER_ACTION_SUBSCRIBE, SUB_TOPIC, NULL);
  Serial.printf("Subscribed to: %s\n", SUB_TOPIC);
  Serial.println("----------------------------------------\n");
}

// ===================================================
// Main Loop
// ===================================================
void loop() {
  // -------------------------------------------------
  // TASK 1: Print any incoming text
  // -------------------------------------------------
  if (incoming_data_flag) {
    incoming_data_flag = false;
    Serial.println("\n--- NEW MESSAGE RECEIVED ---");
    Serial.println("Topic:   " + received_topic);
    Serial.println("Message: " + received_payload);
    Serial.println("--------------------------------\n");
  }

  // -------------------------------------------------
  // TASK 2: Publish simple text periodically
  // -------------------------------------------------
  static unsigned long last_publish = 0;
  if (millis() - last_publish >= publish_interval) {
    last_publish = millis();
    
    // Safety check
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("ERROR: Wi-Fi disconnected. Cannot publish.");
      return;
    }

    // Create a simple text payload
    String msg = "Hello from the ESP32! Uptime: " + String(millis() / 1000) + "s";
    Serial.println("Publishing: " + msg);
    
    // Send it to AWS
    esp_err_t err = mqtt_manager_action(MQTT_MANAGER_ACTION_PUBLISH, PUB_TOPIC, msg.c_str());
    
    // Check for queue overflows or errors
    if (err == ESP_OK) {
      Serial.println("Message queued successfully.");
    } 
    else if (err == ESP_ERR_NO_MEM) {
      Serial.println("ERROR: Queue is FULL! Check AWS connection/policy.");
    } 
    else {
      Serial.printf("ERROR: Publish failed with code: %d\n", err);
    }
  }
}