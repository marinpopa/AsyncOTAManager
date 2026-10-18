#include "AsyncOTAManager.h"
#include <Update.h>
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
            // Acest handler rulează DUPĂ ce upload-ul s-a terminat
            if (!checkAuthentication(request)) return;

            if (Update.hasError()) {
                request->send(500, "text/plain", "Update failed");
                Update.printError(Serial);
            } else {
                request->send(200, "text/plain", "Update successful. Rebooting...");
                delay(1000); // Așteptăm să se trimită pachetul HTTP înainte de restart
                ESP.restart();
            }
        },
        [this](AsyncWebServerRequest *request,
               String filename,
               size_t index,
               uint8_t *data,
               size_t len,
               bool final) {
            // Acest handler rulează ÎN TIMPUL upload-ului
            if (!checkAuthentication(request)) return;

            if (index == 0) {
                uint8_t command = U_FLASH; // Default la Firmware

                if (_autoDetect) {
                    if (filename.endsWith(".spiffs.bin")) {
                        #if defined(ESP8266) || (defined(ESP32) && ARDUINO_ARCH_ESP32_VERSION_MAJOR < 2)
                            SPIFFS.end(); // Demontăm doar dacă e necesar
                            command = U_SPIFFS;
                        #else
                            request->send(500, "text/plain", "SPIFFS not supported on this board");
                            return;
                        #endif
                    }
                    else if (filename.endsWith(".littlefs.bin")) {
                        LittleFS.end(); // Demontăm LittleFS înainte de update
                        command = U_LITTLEFS;
                    }
                    // Altfel rămâne U_FLASH
                }

                if (!Update.begin(UPDATE_SIZE_UNKNOWN, command)) {
                    Update.printError(Serial);
                }
            }

            if (Update.write(data, len) != len) {
                Update.printError(Serial);
            }

            if (final) {
                if (!Update.end(true)) {
                    Update.printError(Serial);
                }
            }
        }
    );

    // ============ LEGACY ENDPOINTS (Standardizate) ============

    // Firmware Only
    _server->on("/update-fw", HTTP_POST,
        [this](AsyncWebServerRequest *request) {
            if (!checkAuthentication(request)) return;
            if (Update.hasError()) {
                request->send(500, "text/plain", "Firmware update failed");
            } else {
                request->send(200, "text/plain", "Firmware updated. Rebooting...");
                delay(1000);
                ESP.restart();
            }
        },
        [this](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
            if (!checkAuthentication(request)) return;
            if (index == 0) Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH);
            Update.write(data, len);
            if (final) Update.end(true);
        }
    );

    // LittleFS Only
    _server->on("/update-littlefs", HTTP_POST,
        [this](AsyncWebServerRequest *request) {
            if (!checkAuthentication(request)) return;
            if (Update.hasError()) {
                request->send(500, "text/plain", "LittleFS update failed");
            } else {
                request->send(200, "text/plain", "LittleFS updated. Rebooting...");
                delay(1000);
                ESP.restart();
            }
        },
        [this](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
            if (!checkAuthentication(request)) return;
            if (index == 0) {
                LittleFS.end();
                Update.begin(UPDATE_SIZE_UNKNOWN, U_LITTLEFS);
            }
            Update.write(data, len);
            if (final) Update.end(true);
        }
    );

    // SPIFFS Only (Protejat pentru compatibilitate)
    _server->on("/update-spiffs", HTTP_POST,
        [this](AsyncWebServerRequest *request) {
            if (!checkAuthentication(request)) return;
            if (Update.hasError()) {
                request->send(500, "text/plain", "SPIFFS update failed");
            } else {
                request->send(200, "text/plain", "SPIFFS updated. Rebooting...");
                delay(1000);
                ESP.restart();
            }
        },
        [this](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
            if (!checkAuthentication(request)) return;
            if (index == 0) {
                #if defined(ESP8266) || (defined(ESP32) && ARDUINO_ARCH_ESP32_VERSION_MAJOR < 2)
                    SPIFFS.end();
                    Update.begin(UPDATE_SIZE_UNKNOWN, U_SPIFFS);
                #else
                    Update.printError(Serial);
                    return;
                #endif
            }
            Update.write(data, len);
            if (final) Update.end(true);
        }
    );

    Serial.println("[OTA Manager] Ready");
}
