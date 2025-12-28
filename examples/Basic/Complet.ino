#include <AsyncOTAManager.h>

// Creează serverul și managerul OTA
AsyncWebServer server(80);
AsyncOTAManager otaManager(server);

void setup() {
  Serial.begin(115200);
  
  // WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin("NumeRețea", "ParolaRețea");
  
  // Așteaptă conexiunea
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Conectare la WiFi...");
  }
  
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
  
  // Configurează OTA
  otaManager.setareAutentificare("admin", "securepass");
  otaManager.begin("/update"); // Schimbă calea dacă dorești
  
  // Alte rute ale serverului...
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", "Salut! Mergi la /update pentru OTA");
  });
  
  server.begin();
  Serial.println("Server pornit. Accesează http://" + WiFi.localIP().toString() + "/update");
}

void loop() {
  // Aici poți rula alte task-uri
  delay(1000);
}