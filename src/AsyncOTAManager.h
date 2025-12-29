#pragma once
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <SPIFFS.h>

class AsyncOTAManager {
public:
  AsyncOTAManager(AsyncWebServer &server);

  // Start OTA service
  void begin(const char *path = "/ota");

  // Authentication
  void setAuthentication(const char *user, const char *pass);

  // Auto-detection
  void enableAutoDetect(bool enable = true);

private:
  AsyncWebServer *_server;
  bool _authEnabled = false;
  String _otaUser;
  String _otaPass;
  bool _autoDetect = true;

  bool checkAuthentication(AsyncWebServerRequest *request);
  String detectFileSystemType();

  // Romanian aliases for compatibility
  void setareAutentificare(const char *utilizator, const char *parola) {
    setAuthentication(utilizator, parola);
  }
  void activareDetectareAutomata(bool activat = true) {
    enableAutoDetect(activat);
  }
};
