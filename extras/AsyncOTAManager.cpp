#include "AsyncOTAManager.h"
#include <Update.h>
#include <SPIFFS.h>

// ================= EMBEDDED OTA PAGE =================
static const char OTA_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<title>ESP32 OTA Update</title>
<style>
body{
  font-family: Arial, sans-serif;
  background:#111;
  color:#eee;
  max-width:480px;
  margin:auto;
  padding:20px
}
.card{
  border:1px solid #333;
  border-radius:8px;
  padding:15px;
  margin-bottom:20px
}
button{
  width:100%;
  padding:10px;
  background:#0a84ff;
  border:none;
  color:#fff;
  font-size:16px;
  border-radius:6px;
  cursor:pointer
}
progress{
  width:100%;
  height:20px
}
</style>
</head>

<body>

<h2>ESP32 OTA Update</h2>

<div class="card">
<h3>Firmware Update (.bin)</h3>
<input type="file" id="fw">
<br><br>
<button onclick="upload('fw','/update-fw')">Upload Firmware</button>
<progress id="fw_p" value="0" max="100"></progress>
</div>

<div class="card">
<h3>SPIFFS Update (spiffs.bin)</h3>
<input type="file" id="fs">
<br><br>
<button onclick="upload('fs','/update-spiffs')">Upload SPIFFS</button>
<progress id="fs_p" value="0" max="100"></progress>
</div>

<script>
function upload(id, url){
  const file = document.getElementById(id).files[0];
  if(!file){
    alert("Please select a file!");
    return;
  }

  const xhr = new XMLHttpRequest();
  const progress = document.getElementById(id + "_p");

  xhr.upload.onprogress = e => {
    if(e.lengthComputable){
      progress.value = (e.loaded / e.total) * 100;
    }
  };

  const form = new FormData();
  form.append("update", file);
  xhr.open("POST", url, true);
  xhr.send(form);
}
</script>

</body>
</html>
)rawliteral";
// =====================================================

AsyncOTAManager::AsyncOTAManager(AsyncWebServer &server) {
  _server = &server;
}

void AsyncOTAManager::begin(const char *path) {

  // Pagina OTA
  _server->on(path, HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", OTA_HTML);
  });

  // ============ OTA Firmware ============
  _server->on("/update-fw", HTTP_POST,
    [](AsyncWebServerRequest *request){},
    [](AsyncWebServerRequest *request,
       String filename,
       size_t index,
       uint8_t *data,
       size_t len,
       bool final){

      if (!index) Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH);
      Update.write(data, len);

      if (final) {
        if (Update.end(true)) {
          request->send(200, "text/plain", "Firmware OK. Reboot...");
          ESP.restart();
        } else {
          request->send(500, "text/plain", "Firmware FAILED");
        }
      }
    }
  );

  // ============ OTA SPIFFS ============
  _server->on("/update-spiffs", HTTP_POST,
    [](AsyncWebServerRequest *request){},
    [](AsyncWebServerRequest *request,
       String filename,
       size_t index,
       uint8_t *data,
       size_t len,
       bool final){

      if (!index) {
        SPIFFS.end();
        Update.begin(UPDATE_SIZE_UNKNOWN, U_SPIFFS);
      }
      Update.write(data, len);

      if (final) {
        if (Update.end(true)) {
          request->send(200, "text/plain", "SPIFFS OK. Reboot...");
          ESP.restart();
        } else {
          request->send(500, "text/plain", "SPIFFS FAILED");
        }
      }
    }
  );
}
