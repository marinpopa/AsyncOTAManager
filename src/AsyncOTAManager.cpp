#include "AsyncOTAManager.h"
#include <Update.h>
#include <SPIFFS.h>
#include <LittleFS.h>
#include "ota_html_gz.h"

AsyncOTAManager::AsyncOTAManager(AsyncWebServer &server) {
  _server = &server;
}

void AsyncOTAManager::setAuthentication(const char *user, const char *pass) {
  _authEnabled = true;
  _otaUser = user;
  _otaPass = pass;
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

String AsyncOTAManager::detectFileSystemType() {
  if (SPIFFS.begin()) {
    SPIFFS.end();
    return "SPIFFS";
  }
  if (LittleFS.begin()) {
    LittleFS.end();
    return "LittleFS";
  }
  return "UNKNOWN";
}

void AsyncOTAManager::begin(const char *path) {

  // ============ OTA PAGE ============
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

  // ============ UNIVERSAL UPLOAD ============
  _server->on("/update", HTTP_POST,
    [this](AsyncWebServerRequest *request) {
      if (!checkAuthentication(request)) return;

      if (Update.hasError()) {
        request->send(500, "text/plain", "Update failed");
      } else {
        request->send(200, "text/plain", "Update successful. Rebooting...");
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

      if (!checkAuthentication(request)) return;

      if (index == 0) {
        if (_autoDetect) {
          if (filename.endsWith(".bin") || filename.endsWith(".ino.bin")) {
            Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH);
          } else if (filename.endsWith(".spiffs.bin") || filename.endsWith(".littlefs.bin")) {
            String fsType = detectFileSystemType();
            if (fsType == "SPIFFS") {
              SPIFFS.end();
              Update.begin(UPDATE_SIZE_UNKNOWN, U_SPIFFS);
            } else if (fsType == "LittleFS") {
              LittleFS.end();
              Update.begin(UPDATE_SIZE_UNKNOWN, U_LITTLEFS);
            }
          }
        } else {
          Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH);
        }
      }

      if (Update.write(data, len) != len) {
        Update.printError(Serial);
      }

      if (final) {
        if (Update.end(true)) {
          Serial.println("Update completed successfully");
        } else {
          Update.printError(Serial);
        }
      }
    }
  );

  // Legacy endpoints for compatibility
  _server->on("/update-fw", HTTP_POST,
    [this](AsyncWebServerRequest *request) {
      if (!checkAuthentication(request)) return;
    },
    [this](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
      if (!checkAuthentication(request)) return;

      if (index == 0) Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH);
      Update.write(data, len);

      if (final) {
        if (Update.end(true)) {
          request->send(200, "text/plain", "Firmware updated. Rebooting...");
          delay(1000);
          ESP.restart();
        } else {
          request->send(500, "text/plain", "Firmware update failed");
        }
      }
    }
  );

  _server->on("/update-spiffs", HTTP_POST,
    [this](AsyncWebServerRequest *request) {
      if (!checkAuthentication(request)) return;
    },
    [this](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
      if (!checkAuthentication(request)) return;

      if (index == 0) {
        SPIFFS.end();
        Update.begin(UPDATE_SIZE_UNKNOWN, U_SPIFFS);
      }
      Update.write(data, len);

      if (final) {
        if (Update.end(true)) {
          request->send(200, "text/plain", "SPIFFS updated. Rebooting...");
          delay(1000);
          ESP.restart();
        } else {
          request->send(500, "text/plain", "SPIFFS update failed");
        }
      }
    }
  );

  _server->on("/update-littlefs", HTTP_POST,
    [this](AsyncWebServerRequest *request) {
      if (!checkAuthentication(request)) return;
    },
    [this](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
      if (!checkAuthentication(request)) return;

      if (index == 0) {
        LittleFS.end();
        Update.begin(UPDATE_SIZE_UNKNOWN, U_LITTLEFS);
      }
      Update.write(data, len);

      if (final) {
        if (Update.end(true)) {
          request->send(200, "text/plain", "LittleFS updated. Rebooting...");
          delay(1000);
          ESP.restart();
        } else {
          request->send(500, "text/plain", "LittleFS update failed");
        }
      }
    }
  );

  Serial.println("[OTA Manager] Ready with auto-detection and authentication");
}
