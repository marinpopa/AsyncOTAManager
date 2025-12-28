#pragma once
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <SPIFFS.h>

class AsyncOTAManager {
public:
  AsyncOTAManager(AsyncWebServer &server);
  
  // Pornire serviciu OTA (cu autentificare opțională)
  void begin(const char *cale = "/ota");
  void setareAutentificare(const char *utilizator, const char *parola);
  void activareDetectareAutomata(bool activat = true);

private:
  AsyncWebServer *_server;
  bool _autentificareActiva = false;
  String _otaUtilizator;
  String _otaParola;
  bool _detectareAutomata = true;
  
  bool verificaAutentificare(AsyncWebServerRequest *request);
  String detecteazaTipSistemFisiere();
};