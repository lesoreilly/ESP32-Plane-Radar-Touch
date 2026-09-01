#include "services/ota_update.h"

#include <Update.h>
#include <WiFiManager.h>

#include "config.h"
#include "services/display_settings.h"
#include "ui/status_screens.h"

namespace services::ota {
namespace {

constexpr char kFirmwarePage[] PROGMEM = R"HTML(
<!doctype html><html lang="en"><head>
<meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Plane Radar firmware</title>
<style>
body{font-family:system-ui,sans-serif;background:#07101f;color:#eef6ff;margin:0}
main{max-width:34rem;margin:3rem auto;padding:1.5rem}
.card{background:#101d31;border:1px solid #274464;border-radius:14px;padding:1.5rem}
input,button{box-sizing:border-box;width:100%;margin-top:1rem;padding:.8rem}
button{background:#2b8aef;color:white;border:0;border-radius:8px;font-weight:700}
small{color:#a9bad0}a{color:#73b7ff}
</style></head><body><main><div class="card">
<h1>Firmware update</h1>
<p>Upload the OTA application image ending in <strong>-ota.bin</strong>
or PlatformIO's <strong>firmware.bin</strong>.</p>
<form method="post" action="/firmware-upload" enctype="multipart/form-data">
<input type="file" name="firmware" accept=".bin,application/octet-stream" required>
<button type="submit">Install and restart</button></form>
<p><small>Do not upload the merged/full flash image here. Keep power connected
until the device restarts.</small></p><p><a href="/">Back to settings</a></p>
</div></main></body></html>
)HTML";

WiFiManager* s_manager = nullptr;
AdditionalRoutesFn s_additional_routes = nullptr;
bool s_in_progress = false;
bool s_upload_authenticated = false;
String s_upload_error;

WebServer* server() {
  if (s_manager == nullptr || !s_manager->server) {
    return nullptr;
  }
  return s_manager->server.get();
}

bool requestAuthenticated() {
  WebServer* web = server();
  if (web == nullptr) {
    return false;
  }
  if (web->authenticate(config::kOtaUsername, settings::otaPassword())) {
    return true;
  }
  web->requestAuthentication();
  return false;
}

void showFirmwarePage() {
  WebServer* web = server();
  if (web == nullptr || !requestAuthenticated()) {
    return;
  }
  web->send_P(200, PSTR("text/html"), kFirmwarePage);
}

void recordUpdateError() {
  s_upload_error = Update.errorString();
  if (s_upload_error.length() == 0) {
    s_upload_error = "Unknown flash error";
  }
  Update.end();
}

void handleUploadChunk() {
  WebServer* web = server();
  if (web == nullptr) {
    return;
  }
  HTTPUpload& upload = web->upload();

  if (upload.status == UPLOAD_FILE_START) {
    s_upload_authenticated =
        web->authenticate(config::kOtaUsername, settings::otaPassword());
    s_upload_error.clear();
    if (!s_upload_authenticated) {
      return;
    }
    s_in_progress = true;
    if (!upload.filename.endsWith(".bin")) {
      s_upload_error = "Please select an OTA .bin application image";
      return;
    }

    statusScreenFirmwareUpdate();
    Serial.printf("OTA: receiving %s\n", upload.filename.c_str());
    if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH)) {
      recordUpdateError();
    }
    return;
  }

  if (!s_upload_authenticated || s_upload_error.length() != 0) {
    return;
  }

  if (upload.status == UPLOAD_FILE_WRITE) {
    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      recordUpdateError();
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (!Update.end(true)) {
      recordUpdateError();
    } else {
      Serial.printf("OTA: wrote %u bytes\n", upload.totalSize);
    }
  } else if (upload.status == UPLOAD_FILE_ABORTED) {
    s_upload_error = "Upload aborted";
    Update.end();
  }
}

void handleUploadDone() {
  WebServer* web = server();
  if (web == nullptr) {
    return;
  }
  if (!s_upload_authenticated) {
    s_in_progress = false;
    web->requestAuthentication();
    return;
  }

  if (s_upload_error.length() != 0 || Update.hasError()) {
    s_in_progress = false;
    const String message =
        String("<!doctype html><meta name=viewport content='width=device-width'>"
               "<h1>Update failed</h1><p>") +
        (s_upload_error.length() != 0 ? s_upload_error : Update.errorString()) +
        "</p><p><a href='/firmware'>Try again</a></p>";
    web->send(500, "text/html", message);
    Serial.printf("OTA failed: %s\n", s_upload_error.c_str());
    return;
  }

  web->client().setNoDelay(true);
  web->send(
      200, "text/html",
      "<!doctype html><meta name=viewport content='width=device-width'>"
      "<h1>Update installed</h1><p>Plane Radar is restarting...</p>");
  delay(250);
  web->client().stop();
  ESP.restart();
}

void attachRoutes() {
  WebServer* web = server();
  if (web == nullptr) {
    return;
  }

  web->on("/firmware", HTTP_GET, showFirmwarePage);
  web->on("/firmware-upload", HTTP_POST, handleUploadDone,
          handleUploadChunk);

  // Claim WiFiManager's legacy unauthenticated paths before it registers them.
  web->on("/update", HTTP_GET, showFirmwarePage);
  web->on("/u", HTTP_POST, handleUploadDone, handleUploadChunk);

  if (s_additional_routes != nullptr) {
    s_additional_routes();
  }
}

}  // namespace

void configure(WiFiManager& manager, AdditionalRoutesFn additional_routes) {
  s_manager = &manager;
  s_additional_routes = additional_routes;
  manager.setShowInfoUpdate(false);
  manager.setCustomMenuHTML(
      "<form action='/firmware' method='get'><button>Firmware update</button>"
      "</form><br/>\n");
  const char* menu[] = {"wifi", "param", "info", "custom",
                        "sep",  "restart", "exit"};
  manager.setMenu(menu, sizeof(menu) / sizeof(menu[0]));
  manager.setWebServerCallback(attachRoutes);
}

bool inProgress() { return s_in_progress; }

}  // namespace services::ota
