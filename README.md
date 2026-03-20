# ESP32 AWS MQTT Manager

A minimal, asynchronous Arduino wrapper around the native ESP-IDF `esp-mqtt` component for connecting ESP32 microcontrollers to AWS IoT Core using mutual TLS (mTLS).

This library hides the complex ESP-MQTT event plumbing and exposes a small, non-blocking API for `init`, `subscribe`, and `publish`. It is designed to keep your main `loop()` fast and responsive while handling heavy cryptography and background networking securely.

## Features
* **True Mutual TLS:** Pass AWS device certificates directly as strings. No SPIFFS/LittleFS file system mounting required.
* **Asynchronous Receiving:** Incoming messages trigger a lightweight callback, never blocking your main loop.
* **Built-in Message Queue:** Safely queues outgoing messages until the background TLS handshake completes.
* **Native Performance:** Leverages the pre-compiled ESP-IDF framework already built into the ESP32 Arduino Core.

## Hardware Compatibility
This library requires the **ESP32 Arduino Core** to be installed via the Boards Manager. Standard AVR Arduino boards (Uno, Nano, Mega) are not supported.

Currently tested and verified working on:
* **ESP32 DevKit V1** (Xtensa Dual-Core)
* **Seeed Studio XIAO ESP32C3** (RISC-V Single-Core)

## Installation

### Method 1: Arduino Library Manager (Recommended)
[Image of Arduino IDE Library Manager interface showing library search]
1. Open the Arduino IDE.
2. Go to **Tools** -> **Manage Libraries...**
3. Search for **ESP32 AWS MQTT Manager**.
4. Click **Install**.

### Method 2: Manual ZIP Installation
1. Download this repository as a `.zip` file from the releases page.
2. Open the Arduino IDE.
3. Go to **Sketch** -> **Include Library** -> **Add .ZIP Library**.
4. Select the downloaded `.zip` file.

## How to Add Your AWS Certificates
AWS IoT Core requires mutual authentication. You must provide the Amazon Root CA, your Device Certificate, and your Private Key.

To keep your secret keys out of the library source code, this library uses a local header file pattern:

1. Open one of the provided examples (e.g., **File** -> **Examples** -> **ESP32 AWS MQTT Manager** -> **Basic_PubSub**).
2. The Arduino IDE will open a second tab at the top named `aws_certs.h`.
3. Open the certificate files you downloaded from the AWS console using a standard text editor.
4. Copy the entire text block (including the `-----BEGIN...-----` and `-----END...-----` lines).
5. Paste the text directly into the corresponding placeholder variables inside `aws_certs.h`.

## Quick Start API
* `mqtt_manager_init(endpoint, root_ca, client_cert, private_key)`: Connects to your specific AWS ATS endpoint.
* `mqtt_manager_set_message_cb(callback_function)`: Registers the function to fire when a message arrives.
* `mqtt_manager_action(action_type, topic, payload)`: Queues a publish or subscribe command.

See the `examples/` folder for complete, copy-pasteable implementations of Publish-Only, Subscribe-Only, and Bidirectional nodes.

## Common Troubleshooting

### Error: "WiFi.h: No such file or directory"
You are trying to compile for a non-ESP32 board. Go to **Tools** -> **Board** and select your specific ESP32 model.

### Error: "MQTT Queue is FULL!" or Messages Not Appearing in AWS
If your publish attempts return an `ESP_ERR_NO_MEM` code, your ESP3
