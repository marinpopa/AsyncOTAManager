#pragma once

#include <Arduino.h>
#include <ESPAsyncWebServer.h>

// Compatibilitate File System
#if defined(ESP32)
  #include <LittleFS.h>
  // SPIFFS este depreciat pe ESP32 Core 2.0+, dar îl includem condiționat
  #if ARDUINO_ARCH_ESP32_VERSION_MAJOR < 2
    #include <SPIFFS.h>
  #endif
#elif defined(ESP8266)
  #include <LittleFS.h>
  #include <SPIFFS.h>
#endif

class AsyncOTAManager {
public:
    AsyncOTAManager(AsyncWebServer &server);

    // Start OTA service
    void begin(const char *path = "/ota");

    // Authentication
    void setAuthentication(const char *user, const char *pass);

    // Auto-detection (based on file extension, not mounting)
    void enableAutoDetect(bool enable = true);

private:
    AsyncWebServer *_server;
    bool _authEnabled = false;
    String _otaUser;
    String _otaPass;
    bool _autoDetect = true;

    bool checkAuthentication(AsyncWebServerRequest *request);

    // Romanian aliases for compatibility
    void setareAutentificare(const char *utilizator, const char *parola) {
        setAuthentication(utilizator, parola);
    }
    void activareDetectareAutomata(bool activat = true) {
        enableAutoDetect(activat);
    }
};
