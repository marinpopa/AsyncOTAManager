#include "AsyncOTAManager.h"
#include <Update.h>
#include <SPIFFS.h>
#include <LittleFS.h>
#include "ota_html_gz.h"

AsyncOTAManager::AsyncOTAManager(AsyncWebServer &server) {
  _server = &server;
}

void AsyncOTAManager::setareAutentificare(const char *utilizator, const char *parola) {
  _autentificareActiva = true;
  _otaUtilizator = utilizator;
  _otaParola = parola;
}

void AsyncOTAManager::activareDetectareAutomata(bool activat) {
  _detectareAutomata = activat;
}

bool AsyncOTAManager::verificaAutentificare(AsyncWebServerRequest *request) {
  if (!_autentificareActiva) return true;
  
  if (!request->authenticate(_otaUtilizator.c_str(), _otaParola.c_str())) {
    request->requestAuthentication();
    return false;
  }
  return true;
}

String AsyncOTAManager::detecteazaTipSistemFisiere() {
  // Detecteaza tipul de sistem de fi?iere curent
  if (SPIFFS.begin()) {
    SPIFFS.end();
    return "SPIFFS";
  }
  if (LittleFS.begin()) {
    LittleFS.end();
    return "LittleFS";
  }
  return "NECUNOSCUT";
}

void AsyncOTAManager::begin(const char *cale) {
  
  // ============ PAGINA OTA ============
  _server->on(cale, HTTP_GET, [this](AsyncWebServerRequest *request) {
    if (!verificaAutentificare(request)) return;
    
    AsyncWebServerResponse *response = request->beginResponse_P(
      200,
      "text/html",
      ota_html_gz,
      ota_html_gz_len
    );
    
    response->addHeader("Content-Encoding", "gzip");
    response->addHeader("Cache-Control", "no-cache");
    request->send(response);
  });
  
  // ============ UPLOAD OTA GENERAL (detectare automata) ============
  _server->on("/update", HTTP_POST,
    [this](AsyncWebServerRequest *request) {
      if (!verificaAutentificare(request)) return;
      
      // Mesaj de raspuns dupa finalizare
      if (Update.hasError()) {
        request->send(500, "text/plain", "Actualizare e?uata");
      } else {
        request->send(200, "text/plain", "Actualizare reu?ita. Repornire...");
        delay(1000);
        ESP.restart();
      }
    },
    [this](AsyncWebServerRequest *request, 
           String filename, 
           size_t index, 
           uint8_t *data, 
           size_t len, 
           bool final) {
      
      if (!verificaAutentificare(request)) return;
      
      // La primul chunk, decidem ce tip de actualizare este
      if (index == 0) {
        if (_detectareAutomata) {
          // Detectare automata dupa extensie
          if (filename.endsWith(".bin") || filename.endsWith(".ino.bin")) {
            Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH);
          } else if (filename.endsWith(".spiffs.bin") || filename.endsWith(".littlefs.bin")) {
            // Determina tipul de sistem de fi?iere
            String fsType = detecteazaTipSistemFisiere();
            if (fsType == "SPIFFS") {
              SPIFFS.end();
              Update.begin(UPDATE_SIZE_UNKNOWN, U_SPIFFS);
            } else if (fsType == "LittleFS") {
              LittleFS.end();
              Update.begin(UPDATE_SIZE_UNKNOWN, U_LITTLEFS);
            }
          }
        } else {
          // Mod manual - pentru backward compatibility
          Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH);
        }
      }
      
      // Scriere date
      if (Update.write(data, len) != len) {
        Update.printError(Serial);
      }
      
      // Finalizare
      if (final) {
        if (Update.end(true)) {
          Serial.println("Actualizare finalizata cu succes");
        } else {
          Update.printError(Serial);
        }
      }
    }
  );
  
  // ============ ENDPOINTURI SPECIFICE (pentru compatibilitate) ============
  _server->on("/update-fw", HTTP_POST,
    [this](AsyncWebServerRequest *request) {
      if (!verificaAutentificare(request)) return;
      // Handler pentru finalizare
    },
    [this](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
      if (!verificaAutentificare(request)) return;
      
      if (index == 0) {
        Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH);
      }
      
      Update.write(data, len);
      
      if (final) {
        if (Update.end(true)) {
          request->send(200, "text/plain", "Firmware actualizat. Repornire...");
          delay(1000);
          ESP.restart();
        } else {
          request->send(500, "text/plain", "E?uare actualizare firmware");
        }
      }
    }
  );
  
  _server->on("/update-spiffs", HTTP_POST,
    [this](AsyncWebServerRequest *request) {
      if (!verificaAutentificare(request)) return;
    },
    [this](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
      if (!verificaAutentificare(request)) return;
      
      if (index == 0) {
        SPIFFS.end();
        Update.begin(UPDATE_SIZE_UNKNOWN, U_SPIFFS);
      }
      
      Update.write(data, len);
      
      if (final) {
        if (Update.end(true)) {
          request->send(200, "text/plain", "SPIFFS actualizat. Repornire...");
          delay(1000);
          ESP.restart();
        } else {
          request->send(500, "text/plain", "E?uare actualizare SPIFFS");
        }
      }
    }
  );
  
  _server->on("/update-littlefs", HTTP_POST,
    [this](AsyncWebServerRequest *request) {
      if (!verificaAutentificare(request)) return;
    },
    [this](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
      if (!verificaAutentificare(request)) return;
      
      if (index == 0) {
        LittleFS.end();
        Update.begin(UPDATE_SIZE_UNKNOWN, U_LITTLEFS);
      }
      
      Update.write(data, len);
      
      if (final) {
        if (Update.end(true)) {
          request->send(200, "text/plain", "LittleFS actualizat. Repornire...");
          delay(1000);
          ESP.restart();
        } else {
          request->send(500, "text/plain", "E?uare actualizare LittleFS");
        }
      }
    }
  );
  
  Serial.println("[OTA Manager] Gata cu detectare automata ?i autentificare");
}