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


### 🔧 API

## Constructor

```cpp

AsyncOTAManager(AsyncWebServer &server);
```
### Main Methods

```cpp

// Start OTA service
void begin(const char *path = "/update");

// Set authentication (optional)
void setAuthentication(const char *user, const char *pass);

// Enable/disable auto-detection
void enableAutoDetect(bool enable = true);
```



## 🌐 Available Endpoints

Endpoint	Method	Description

/ota	GET	OTA web upload page

/update	POST	Universal upload with auto-detection

/update-fw	POST	Firmware upload (legacy)

/update-spiffs	POST	SPIFFS upload (legacy)

/update-littlefs	POST	LittleFS upload

## 🧠 Auto-Detection

### The library automatically detects the update type based on file extension:

File Extension	    Update Type
---
.bin, .ino.bin    	Firmware
---
.spiffs.bin         SPIFFS
---
.littlefs.bin        LittleFS
---
## 📝 Complete Example

```cpp

#include <AsyncOTAManager.h>

// Create server and OTA manager
AsyncWebServer server(80);
AsyncOTAManager otaManager(server);

void setup() {
  Serial.begin(115200);
  
  // WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin("NetworkSSID", "NetworkPassword");
  
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
  
  // Configure OTA
  otaManager.setAuthentication("admin", "securepass");
  otaManager.begin("/update"); // Change path if desired
  
  // Other server routes...
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", "Hello! Go to /update for OTA");
  });
  
  server.begin();
  Serial.println("Server started. Access http://" + WiFi.localIP().toString() + "/update");
}

void loop() {
  // Run other tasks here
  delay(1000);
}
```

## 🛠 Requirements

### Required Libraries

    ESP Async WebServer

    AsyncTCP (ESP32)

    ESPAsyncTCP (ESP8266)

### Supported Platforms

    ESP32 (tested on ESP32 DevKit, NodeMCU-32S)

    ESP8266 (tested on NodeMCU, Wemos D1 Mini)

## 📁 Project Structure
```text

AsyncOTAManager/
├── examples/
│   ├── BasicOTA/
│   │   └── BasicOTA.ino
│   └── SecureOTA/
│       └── SecureOTA.ino
├── src/
│   ├── AsyncOTAManager.h
│   ├── AsyncOTAManager.cpp
│   └── ota_html_gz.h (generated separately)
├── library.properties
├── keywords.txt
├── README.md
└── LICENSE
```

## 🤝 Contributions

### Contributions are welcome! To contribute:

Fork the repository

Create a new branch (git checkout -b feature/new)

Commit your changes (git commit -am 'Add feature X')

Push to the branch (git push origin feature/new)

Open a Pull Request

## 📄 License

### This library is licensed under the MIT License. See the LICENSE file for details.

## 🙏 Acknowledgments

ESPAsyncWebServer for the async web server

Arduino Core for ESP32/ESP8266 support

### Made with ❤️ for the Arduino community 🚀











