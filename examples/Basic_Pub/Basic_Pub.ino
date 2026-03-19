#include <WiFi.h>
#include <time.h>
#include "esp_log.h" // Exposes hidden system logs for troubleshooting
#include "mqtt_manager.h"
#include "aws_certs.h" // This must be in the same folder as this sketch

// ===================================================
// Configuration - Update these with your details!
// ===================================================
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
const char* aws_endpoint = "mqtts://YOUR_ENDPOINT-ats.iot.YOUR_REGION.amazonaws.com:8883";

// Define your MQTT topic
const char* PUB_TOPIC = "device/outbound";

// Publish interval in milliseconds
const unsigned long PUBLISH_INTERVAL = 5000; 

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
  // Note: We do not set a callback here because this node only publishes data.
  esp_err_t init_status = mqtt_manager_init(aws_endpoint, AWS_ROOT_CA, CLIENT_CERT, CLIENT_PRIVATE_KEY);
  
  if (init_status == ESP_OK) {
    Serial.println("MQTT Manager Initialized");
  } else {
    Serial.printf("ERROR: MQTT Init Failed with code: %d\n", init_status);
  }
  
  Serial.println("----------------------------------------");
  Serial.println("Telemetry Node Online. Beginning data transmission...\n");
}

// ===================================================
// Main Loop
// ===================================================
void loop() {
  static unsigned long last_publish = 0;
  
  // Publish simulated telemetry data at the set interval
  if (millis() - last_publish >= PUBLISH_INTERVAL) {
    last_publish = millis();
    
    // Safety check: ensure Wi-Fi is still connected before trying to publish
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("ERROR: Wi-Fi disconnected. Cannot publish.");
      return;
    }

    // Simulate a sensor reading (e.g., battery voltage)
    float battery_voltage = 7.4 - (random(0, 50) / 100.0); 
    String payload = "{\"battery_v\":" + String(battery_voltage) + "}";
    
    Serial.println("Attempting to publish: " + payload);
    
    // Send it to AWS
    esp_err_t err = mqtt_manager_action(MQTT_MANAGER_ACTION_PUBLISH, PUB_TOPIC, payload.c_str());
    
    // Check for queue overflows or errors
    if (err == ESP_OK) {
      Serial.println("Message queued successfully.\n");
    } 
    else if (err == ESP_ERR_NO_MEM) {
      Serial.println("ERROR: Queue is FULL! Check AWS connection/policy.\n");
    } 
    else {
      Serial.printf("ERROR: Publish failed with code: %d\n\n", err);
    }
  }
}