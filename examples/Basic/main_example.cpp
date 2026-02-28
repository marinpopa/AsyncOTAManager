/**
 * main.cpp — Exemplu utilizare corecta
 * AsyncWiFiManagerSimple (port 80) + AsyncOTAManager (port 8080)
 *
 * Porturi separate → fara niciun conflict intre manageri
 */

#include <Arduino.h>
#include <LittleFS.h>

// Redirectioneaza global SPIFFS → LittleFS (inainte de orice include)
#ifndef SPIFFS
  #define SPIFFS LittleFS
#endif

#include <ESPAsyncWebServer.h>
#include "AsyncWiFiManagerSimple.h"
#include "AsyncOTAManager.h"

// ─── Serverul aplicatiei tale — port 8080 ──────────────────────────────────
// AsyncWiFiManagerSimple are intern serverul sau pe port 80 (captive portal)
// AsyncOTAManager primeste referinta la serverul tau de pe 8080
AsyncWebServer appServer(8080);

AsyncWiFiManagerSimple wifiManager;
AsyncOTAManager        otaManager(appServer);

void setup() {
    Serial.begin(115200);
    delay(300);
    Serial.println("\n=== BOOT ===");

    // 1. Monteaza LittleFS o singura data
    if (!LittleFS.begin(true)) {
        Serial.println("[FS] ⚠️  LittleFS mount esuat!");
    } else {
        Serial.println("[FS] ✅ LittleFS montat.");
    }

    // 2. WiFi — se conecteaza la retea salvata sau porneste AP pe port 80
    wifiManager.Setup();
    // SAU cu AP personalizat:
    // wifiManager.Setup("NumeAP", "parolaAP");

    // 3. OTA — inregistreaza rutele pe appServer (port 8080)
    otaManager.enableAutoDetect(true);
    otaManager.setAuthentication("admin", "esp32ota"); // comenteaza daca nu vrei auth
    otaManager.begin("/ota");

    // 4. Rutele tale proprii pe appServer (port 8080)
    appServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/plain", "Hello from ESP32-S3!");
    });

    appServer.on("/status", HTTP_GET, [](AsyncWebServerRequest *request) {
        String json = "{\"ip\":\""   + WiFi.localIP().toString() + "\""
                    + ",\"rssi\":"   + String(WiFi.RSSI())
                    + ",\"ssid\":\"" + WiFi.SSID() + "\""
                    + ",\"heap\":"   + String(ESP.getFreeHeap()) + "}";
        request->send(200, "application/json", json);
    });

    // 5. Porneste serverul aplicatiei
    //    (WiFiManager isi porneste serverul intern pe 80 singur, in startConfigMode)
    appServer.begin();

    Serial.println("[Main] ✅ App server pornit pe port 8080");
    Serial.printf("[Main] OTA: http://%s:8080/ota\n",
                  WiFi.localIP().toString().c_str());
}

void loop() {
    // WiFiManager: captive portal DNS, timeout AP, reconectare automata
    wifiManager.loop();
}
