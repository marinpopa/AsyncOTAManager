#include <AsyncOTAManager.h>

AsyncWebServer server(80);
AsyncOTAManager otaManager(server);

void setup() {
  otaManager.setAuthentication("admin", "password123");
  otaManager.enableAutoDetect(true);
  otaManager.begin("/ota");
  server.begin();
}

void loop() {
  // Codul tău principal
}