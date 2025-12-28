# AsyncOTAManager 📡

O bibliotecă Arduino pentru ESP32/ESP8266 care oferă o pagină web elegantă pentru actualizarea OTA (Over-The-Air) a firmware-ului și a sistemului de fișiere, cu autentificare și detectare automată.

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Platform: ESP32/ESP8266](https://img.shields.io/badge/Platform-ESP32%2FESP8266-blue.svg)](https://www.arduino.cc/)

## ✨ Caracteristici

- 🌐 **Pagină web OTA integrată** - interfață HTML comprimată (gzip)
- 🔒 **Autentificare opțională** - protejează accesul la actualizări
- 🔍 **Detectare automată** - recunoaște tipul de actualizare după extensie fișier
- 📁 **Suport dual** - compatibil cu SPIFFS și LittleFS
- 🔄 **Compatibilitate** - păstrează endpoint-urile vechi pentru backward compatibility
- ⚡ **Performanță** - folosește ESPAsyncWebServer pentru conexiuni asincrone

## 📦 Instalare

### Prin Arduino IDE
1. **Sketch** → **Include Library** → **Manage Libraries...**
2. Caută "AsyncOTAManager"
3. Click **Install**

### Manual
1. Descarcă ultima versiune [de aici](https://github.com/marinpopa/AsyncOTAManager/releases)
2. Extrage în folderul `libraries` al Arduino IDE
3. Repornește Arduino IDE

## 🚀 Utilizare rapidă

```cpp
#include <WiFi.h>
#include <AsyncOTAManager.h>

AsyncWebServer server(80);
AsyncOTAManager otaManager(server);

void setup() {
  Serial.begin(115200);
  
  // Conectare la WiFi
  WiFi.begin("SSID", "parola");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("\nConectat! IP: " + WiFi.localIP().toString());
  
  // Configurare OTA
  otaManager.setareAutentificare("admin", "parola123"); // Opțional
  otaManager.activareDetectareAutomata(true); // Implicit activ
  otaManager.begin("/ota");
  
  server.begin();
}

void loop() {
  // Codul tău principal
}









