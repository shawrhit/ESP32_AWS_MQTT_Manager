#include <WiFi.h>
#include <time.h>
#include "esp_log.h" // Exposes hidden system logs for troubleshooting
#include "ESP32_AWS_MQTT_Manager.h" // Core MQTT Manager Library
#include "aws_certs.h" // This contains all your certificates and must be in the same folder as this sketch

// ===================================================
// Configuration - Update these with your details!
// ===================================================
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
const char* aws_endpoint = "mqtts://YOUR_ENDPOINT-ats.iot.YOUR_REGION.amazonaws.com:8883";

// Define your MQTT topic
const char* SUB_TOPIC = "device/inbound";

// ===================================================
// Global Variables for Asynchronous Receiving
// ===================================================
volatile bool new_command = false;
String received_command = "idle";

// ===================================================
// Callback Function for Incoming Messages
// ===================================================
static void on_msg(const char *topic, int topic_len, const char *data, int data_len) {
  received_command = String(data).substring(0, data_len);
  new_command = true; 
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
  while (WiFi.status() != WL_CONNECTED) { 
    delay(500); 
    Serial.print("."); 
  }
  Serial.println("\nWi-Fi Connected!");

  // 2. Sync Time (CRITICAL FOR AWS TLS CERTIFICATES)
  Serial.print("Syncing time via NTP...");
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  struct tm timeinfo;
  while (!getLocalTime(&timeinfo)) { 
    delay(1000); 
    Serial.print("."); 
  }
  Serial.println("\nTime synced!");

  // 3. Initialize MQTT Manager
  mqtt_manager_set_message_cb(on_msg);
  esp_err_t init_status = mqtt_manager_init(aws_endpoint, AWS_ROOT_CA, CLIENT_CERT, CLIENT_PRIVATE_KEY);
  
  if (init_status == ESP_OK) {
    Serial.println("MQTT Manager Initialized");
  } else {
    Serial.printf("ERROR: MQTT Init Failed with code: %d\n", init_status);
  }
  
  // 4. Subscribe to the inbound topic
  esp_err_t sub_status = mqtt_manager_action(MQTT_MANAGER_ACTION_SUBSCRIBE, SUB_TOPIC, NULL);
  
  if (sub_status == ESP_OK) {
    Serial.printf("Subscribed to: %s\n", SUB_TOPIC);
  } else {
    Serial.printf("ERROR: Subscribe Action failed with code: %d\n", sub_status);
  }
  
  Serial.println("----------------------------------------");
  Serial.println("Device listening for incoming commands...\n");
}

// ===================================================
// Main Loop
// ===================================================
void loop() {
  // Act on the data in the main loop
  if (new_command) {
    new_command = false;
    
    Serial.println("\n--- NEW COMMAND RECEIVED ---");
    Serial.println("Payload: " + received_command);
    Serial.println("----------------------------\n");
    
    // --> Insert logic here to act on the command (e.g., control motors, update display) <--
  }
}