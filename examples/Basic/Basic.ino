#include <WiFi.h>
#include <AsyncOTAManager.h>

AsyncWebServer server(80);
AsyncOTAManager otaManager(server);

void setup() {
  Serial.begin(115200);
  
  // Connect to WiFi
  WiFi.begin("SSID", "password");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("\nConnected! IP: " + WiFi.localIP().toString());
  
  // Configure OTA
  otaManager.setAuthentication("admin", "password123"); // Optional
  otaManager.enableAutoDetect(true); // Enabled by default
  otaManager.begin("/update");
  
  server.begin();
}

void loop() {
  // Your main code
}
