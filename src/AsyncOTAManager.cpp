#include "AsyncOTAManager.h"
#include <Update.h>
#include <esp_partition.h>   // FIX #8: detectie partitie fara mount/unmount
#include "ota_html_gz.h"

// ---------------------------------------------------------
// CONSTRUCTOR
// ---------------------------------------------------------

AsyncOTAManager::AsyncOTAManager(AsyncWebServer &server) {
  _server = &server;
}

// ---------------------------------------------------------
// AUTHENTICATION
// ---------------------------------------------------------

void AsyncOTAManager::setAuthentication(const char *user, const char *pass) {
  _authEnabled = true;
  _otaUser     = user;
  _otaPass     = pass;
}

void AsyncOTAManager::enableAutoDetect(bool enable) {
  _autoDetect = enable;
}

bool AsyncOTAManager::checkAuthentication(AsyncWebServerRequest *request) {
  if (!_authEnabled) return true;

  if (!request->authenticate(_otaUser.c_str(), _otaPass.c_str())) {
    request->requestAuthentication();
    return false;
  }
  return true;
}

// ---------------------------------------------------------
// FIX #8: Detectie tip FS prin tabelul de partitii
//         Nu mai monteaza/demonteaza FS pentru detectie
//         → nu corupe starea unui FS deja montat
// ---------------------------------------------------------

bool AsyncOTAManager::isLittleFSPartitionPresent() {
  // Cauta o partitie de tip data cu subtip littlefs (0x83) sau spiffs (0x82)
  const esp_partition_t *part = esp_partition_find_first(
      ESP_PARTITION_TYPE_DATA,
      (esp_partition_subtype_t)0x83,  // ESP_PARTITION_SUBTYPE_DATA_LITTLEFS
      nullptr
  );
  if (part != nullptr) return true;

  // Fallback: cauta si subtipul spiffs (0x82) — poate fi redenumit in littlefs
  part = esp_partition_find_first(
      ESP_PARTITION_TYPE_DATA,
      (esp_partition_subtype_t)0x82,  // ESP_PARTITION_SUBTYPE_DATA_SPIFFS
      nullptr
  );
  return (part != nullptr);
}

// ---------------------------------------------------------
// BEGIN — inregistreaza toate rutele OTA pe serverul shared
// ---------------------------------------------------------

void AsyncOTAManager::begin(const char *path) {

  // ============================================================
  // PAGINA WEB OTA (GET)
  // ============================================================
  _server->on(path, HTTP_GET, [this](AsyncWebServerRequest *request) {
    if (!checkAuthentication(request)) return;

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

  // ============================================================
  // UPLOAD UNIVERSAL /update
  // Detecteaza automat dupa extensia fisierului:
  //   .bin / .ino.bin         → firmware (U_FLASH)
  //   .spiffs.bin / .littlefs.bin → filesystem (U_LITTLEFS)
  // ============================================================
  _server->on(
    "/update", HTTP_POST,

    // --- Handler POST response (se executa DUPA upload complet) ---
    [this](AsyncWebServerRequest *request) {
      if (!checkAuthentication(request)) return;

      // FIX #9: Verifica flag-ul setat in upload callback
      if (!_uploadAuthOk && _authEnabled) {
        request->send(401, "text/plain", "Unauthorized");
        return;
      }

      if (Update.hasError()) {
        String err = "Update failed: ";
        // Preia mesajul de eroare din Update
        StreamString errStr;
        Update.printError(errStr);
        err += errStr;
        request->send(500, "text/plain", err);
      } else {
        request->send(200, "text/plain", "Update reusit! Repornire...");
      }

      // FIX #4: Restart non-blocking — evita WDT reset in task async
      xTaskCreate(
        [](void*) {
          vTaskDelay(800 / portTICK_PERIOD_MS);
          ESP.restart();
        },
        "ota_restart", 1024, nullptr, 1, nullptr
      );
    },

    // --- Handler upload chunks ---
    [this](AsyncWebServerRequest *request,
           String filename,
           size_t index,
           uint8_t *data,
           size_t len,
           bool final) {

      // FIX #9: Autentificarea se valideaza o data la index==0
      //         requestAuthentication() NU functioneaza in upload callback
      if (index == 0) {
        _uploadAuthOk = !_authEnabled ||
                        request->authenticate(_otaUser.c_str(), _otaPass.c_str());

        if (!_uploadAuthOk) {
          Serial.println("[OTA] Upload refuzat — autentificare esuata.");
          return;
        }
      }

      if (!_uploadAuthOk) return;

      // La inceputul uploadului, determina tipul de update
      if (index == 0) {
        Serial.printf("[OTA] Upload inceput: %s\n", filename.c_str());

        int updateType = U_FLASH; // implicit firmware

        if (_autoDetect) {
          if (filename.endsWith(".spiffs.bin") || filename.endsWith(".littlefs.bin")) {
            // FIX #6: Pe ESP32-S3 folosim intotdeauna U_LITTLEFS
            //         U_SPIFFS e alias pentru U_LITTLEFS in SDK-ul nou, dar
            //         explicit e mai sigur
            if (isLittleFSPartitionPresent()) {
              LittleFS.end();  // demonteaza inainte de scriere
              updateType = U_LITTLEFS;
              Serial.println("[OTA] Tip detectat: LittleFS filesystem");
            } else {
              Serial.println("[OTA] ⚠️  Partitie LittleFS negasita!");
              request->send(400, "text/plain", "LittleFS partition not found");
              return;
            }
          } else if (filename.endsWith(".bin") || filename.endsWith(".ino.bin")) {
            updateType = U_FLASH;
            Serial.println("[OTA] Tip detectat: Firmware (Flash)");
          } else {
            Serial.printf("[OTA] ⚠️  Extensie necunoscuta: %s\n", filename.c_str());
            request->send(400, "text/plain", "Unknown file type. Use .bin or .littlefs.bin");
            return;
          }
        }

        size_t updateSize = (updateType == U_FLASH)
            ? UPDATE_SIZE_UNKNOWN
            : UPDATE_SIZE_UNKNOWN;

        if (!Update.begin(updateSize, updateType)) {
          StreamString errStr;
          Update.printError(errStr);
          Serial.printf("[OTA] begin() esuat: %s\n", errStr.c_str());
          return;
        }
      }

      // Scrie chunk-ul curent
      if (Update.write(data, len) != len) {
        StreamString errStr;
        Update.printError(errStr);
        Serial.printf("[OTA] Eroare scriere: %s\n", errStr.c_str());
      }

      // Finalizeaza uploadul
      if (final) {
        if (Update.end(true)) {
          Serial.printf("[OTA] ✅ Update complet: %u bytes\n", index + len);
        } else {
          StreamString errStr;
          Update.printError(errStr);
          Serial.printf("[OTA] ❌ Eroare finalizare: %s\n", errStr.c_str());
        }
      }
    }
  );

  // ============================================================
  // ENDPOINT LEGACY: /update-fw  (doar firmware)
  // Pastrat pentru compatibilitate cu tooluri externe
  // ============================================================
  _server->on(
    "/update-fw", HTTP_POST,

    [this](AsyncWebServerRequest *request) {
      if (!checkAuthentication(request)) return;
      if (!_uploadAuthOk && _authEnabled) {
        request->send(401, "text/plain", "Unauthorized");
        return;
      }
      if (Update.hasError()) {
        request->send(500, "text/plain", "Firmware update failed");
      } else {
        request->send(200, "text/plain", "Firmware updated. Rebooting...");
        xTaskCreate([](void*){ vTaskDelay(800/portTICK_PERIOD_MS); ESP.restart(); },
                    "ota_rst_fw", 1024, nullptr, 1, nullptr);
      }
    },

    [this](AsyncWebServerRequest *request, String filename, size_t index,
           uint8_t *data, size_t len, bool final) {

      // FIX #9: Autentificare prin flag
      if (index == 0) {
        _uploadAuthOk = !_authEnabled ||
                        request->authenticate(_otaUser.c_str(), _otaPass.c_str());
        if (!_uploadAuthOk) return;
      }
      if (!_uploadAuthOk) return;

      // FIX #7: begin() apelat o singura data la index==0
      if (index == 0) {
        Serial.println("[OTA] Legacy /update-fw inceput");
        if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH)) {
          Update.printError(Serial);
          return;
        }
      }

      if (Update.write(data, len) != len) {
        Update.printError(Serial);
      }

      if (final) {
        if (Update.end(true)) {
          Serial.println("[OTA] Legacy firmware update OK");
        } else {
          Update.printError(Serial);
        }
      }
    }
  );

  // ============================================================
  // ENDPOINT LEGACY: /update-littlefs  (filesystem)
  // FIX #6: Redenumit de la /update-spiffs la /update-littlefs
  //         si foloseste U_LITTLEFS in loc de U_SPIFFS
  // ============================================================
  _server->on(
    "/update-littlefs", HTTP_POST,

    [this](AsyncWebServerRequest *request) {
      if (!checkAuthentication(request)) return;
      if (!_uploadAuthOk && _authEnabled) {
        request->send(401, "text/plain", "Unauthorized");
        return;
      }
      if (Update.hasError()) {
        request->send(500, "text/plain", "LittleFS update failed");
      } else {
        request->send(200, "text/plain", "LittleFS updated. Rebooting...");
        xTaskCreate([](void*){ vTaskDelay(800/portTICK_PERIOD_MS); ESP.restart(); },
                    "ota_rst_fs", 1024, nullptr, 1, nullptr);
      }
    },

    [this](AsyncWebServerRequest *request, String filename, size_t index,
           uint8_t *data, size_t len, bool final) {

      if (index == 0) {
        _uploadAuthOk = !_authEnabled ||
                        request->authenticate(_otaUser.c_str(), _otaPass.c_str());
        if (!_uploadAuthOk) return;
      }
      if (!_uploadAuthOk) return;

      if (index == 0) {
        Serial.println("[OTA] Legacy /update-littlefs inceput");
        LittleFS.end();  // demonteaza inainte de scriere
        // FIX #6: U_LITTLEFS explicit in loc de U_SPIFFS
        if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_LITTLEFS)) {
          Update.printError(Serial);
          return;
        }
      }

      if (Update.write(data, len) != len) {
        Update.printError(Serial);
      }

      if (final) {
        if (Update.end(true)) {
          Serial.println("[OTA] Legacy LittleFS update OK");
        } else {
          Update.printError(Serial);
        }
      }
    }
  );

  // ============================================================
  // ENDPOINT LEGACY: /update-spiffs
  // Pastrat DOAR pentru compatibilitate cu tooluri vechi
  // Redirectioneaza intern catre LittleFS (nu catre U_SPIFFS)
  // ============================================================
  _server->on(
    "/update-spiffs", HTTP_POST,

    [this](AsyncWebServerRequest *request) {
      if (!checkAuthentication(request)) return;
      if (!_uploadAuthOk && _authEnabled) {
        request->send(401, "text/plain", "Unauthorized");
        return;
      }
      if (Update.hasError()) {
        request->send(500, "text/plain", "LittleFS update failed");
      } else {
        request->send(200, "text/plain", "LittleFS (SPIFFS compat) updated. Rebooting...");
        xTaskCreate([](void*){ vTaskDelay(800/portTICK_PERIOD_MS); ESP.restart(); },
                    "ota_rst_sp", 1024, nullptr, 1, nullptr);
      }
    },

    [this](AsyncWebServerRequest *request, String filename, size_t index,
           uint8_t *data, size_t len, bool final) {

      if (index == 0) {
        _uploadAuthOk = !_authEnabled ||
                        request->authenticate(_otaUser.c_str(), _otaPass.c_str());
        if (!_uploadAuthOk) return;
      }
      if (!_uploadAuthOk) return;

      if (index == 0) {
        Serial.println("[OTA] Legacy /update-spiffs → redirectat la U_LITTLEFS");
        LittleFS.end();
        // FIX #6: Chiar si pentru endpoint-ul /update-spiffs folosim U_LITTLEFS
        //         SPIFFS e deprecated pe ESP32-S3
        if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_LITTLEFS)) {
          Update.printError(Serial);
          return;
        }
      }

      if (Update.write(data, len) != len) {
        Update.printError(Serial);
      }

      if (final) {
        if (Update.end(true)) {
          Serial.println("[OTA] Legacy SPIFFS→LittleFS update OK");
        } else {
          Update.printError(Serial);
        }
      }
    }
  );

  Serial.println("[OTA Manager] Gata. Rute active: /ota, /update, /update-fw, /update-littlefs, /update-spiffs");
  Serial.printf("[OTA Manager] Autentificare: %s\n", _authEnabled ? "DA" : "NU");
  Serial.printf("[OTA Manager] Auto-detectie: %s\n", _autoDetect ? "DA" : "NU");
}
