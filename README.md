# AsyncOTAManager 📡

An Arduino library for ESP32/ESP8266 that provides an elegant web page for OTA (Over-The-Air) firmware and filesystem updates, with authentication and auto-detection.

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Platform: ESP32/ESP8266](https://img.shields.io/badge/Platform-ESP32%2FESP8266-blue.svg)](https://www.arduino.cc/)

## ✨ Features

🌐 Built-in OTA web page - compressed HTML interface (gzip)

🔒 Optional authentication - protects update access

🔍 Auto-detection - recognizes update type by file extension

📁 Dual support - compatible with SPIFFS and LittleFS

🔄 Backward compatibility - maintains old endpoints

⚡ Performance - uses ESPAsyncWebServer for async connections

## 📦 Installation

### Via Arduino IDE

Sketch → Include Library → Manage Libraries...

Search for "AsyncOTAManager"

Click Install

### Manual Installation

Download the latest version from here

Extract to the Arduino IDE libraries folder

Restart Arduino IDE

## 🚀 Quick Start
```cpp

#include <WiFi.h>
#include <AsyncOTAManager.h>

AsyncWebServer server(80);
AsyncOTAManager otaManager(server);

void setup() {
  Serial.begin(115200);
  
  // Connect to WiFi
  WiFi.begin("SSID", "password");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("\nConnected! IP: " + WiFi.localIP().toString());
  
  // Configure OTA
  otaManager.setAuthentication("admin", "password123"); // Optional
  otaManager.enableAutoDetect(true); // Enabled by default
  otaManager.begin("/update");
  
  server.begin();
}

void loop() {
  // Your main code
}
```











