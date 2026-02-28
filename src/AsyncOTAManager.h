#pragma once

#include <ESPAsyncWebServer.h>

// FIX #1: Nu include ambele FS headers simultan — conflict VFS pe ESP32-S3
// Folosim DOAR LittleFS; SPIFFS e deprecated si nu e recomandat pe ESP32-S3
#include <LittleFS.h>

// FIX #1 cont.: Daca alte librarii folosesc SPIFFS, redirectioneaza catre LittleFS
//               Aceasta definitie trebuie sa fie INAINTE de orice include al lor
#ifndef SPIFFS
  #define SPIFFS LittleFS
#endif

class AsyncOTAManager {
public:
  // FIX #3: Serverul e injectat din exterior (shared cu AsyncWiFiManagerSimple)
  //         → evita doua instante AsyncWebServer pe acelasi port 80
  AsyncOTAManager(AsyncWebServer &server);

  // Porneste serviciul OTA
  // path = ruta pentru pagina web OTA (default "/ota")
  void begin(const char *path = "/ota");

  // Seteaza autentificare HTTP Basic
  void setAuthentication(const char *user, const char *pass);

  // Detectie automata tip fisier (.bin = firmware, .littlefs.bin = filesystem)
  void enableAutoDetect(bool enable = true);

private:
  AsyncWebServer *_server;

  bool _authEnabled  = false;
  String _otaUser;
  String _otaPass;
  bool _autoDetect   = true;

  // FIX #9: Autentificarea in upload callbacks se face DOAR prin flag
  //         requestAuthentication() nu functioneaza in al doilea lambda
  bool _uploadAuthOk = false;

  // Verifica autentificare pentru GET / POST response handler
  bool checkAuthentication(AsyncWebServerRequest *request);

  // FIX #8: Detectie FS fara mount/unmount — verifica direct tipul partitiei
  //         in loc sa apeleze begin()/end() care pot corupe starea FS
  bool isLittleFSPartitionPresent();
};
