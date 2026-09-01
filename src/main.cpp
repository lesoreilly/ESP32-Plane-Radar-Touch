/**
 * Plane Radar — WiFi setup, then radar UI on the round GC9A01 display.
 */

#include <Arduino.h>
#include <WiFi.h>

#include "config.h"
#include "hardware/display.h"
#include "services/adsb_client.h"
#include "services/display_settings.h"
#include "services/ota_update.h"
#include "services/radar_location.h"
#include "services/radar_rotation.h"
#include "services/weather_time.h"
#include "services/wifi_setup.h"
#include "ui/radar_display.h"
#include "ui/radar_range.h"
#include "ui/status_screens.h"

namespace {

bool g_radar_visible = false;
unsigned long g_wifi_down_since = 0;
unsigned long g_last_reconnect_ms = 0;
unsigned long g_last_adsb_fetch_ms = 0;
bool g_touching = false;

void showRadarIfConnected() {
  if (WiFi.status() != WL_CONNECTED) {
    g_radar_visible = false;
    return;
  }
  services::weather::begin();
  ui::radarDisplayDraw();
  g_radar_visible = true;
}

void handleBootButton() {
  bootButtonPollLongPress();
  bootButtonConsumeTap();
}

void handleRadarTouch() {
  lgfx::touch_point_t point = {};
  const int count = tft.getTouch(&point, 1);
  if (count <= 0) {
    g_touching = false;
    return;
  }
  if (g_touching) {
    return;
  }
  g_touching = true;
  if (!g_radar_visible || WiFi.status() != WL_CONNECTED) {
    return;
  }
  ui::radarDisplayOnTouch(point.x, point.y);
}

void fetchAndDrawAircraft() {
  const float fetch_km = ui::radar::fetchRadiusKm();
  if (!services::adsb::fetchUpdate(services::location::lat(),
                                   services::location::lon(), fetch_km)) {
    handleBootButton();
    handleRadarTouch();
    return;
  }
  ui::radarDisplayRefreshAircraft();
  handleBootButton();
  handleRadarTouch();
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println();
  Serial.println("Plane Radar Touch");

  bootButtonInit();
  displayInit();
  if (wifiShowsSetupScreenOnBoot()) {
    statusScreenPortal();
  }
  services::location::init();
  services::radar_rotation::init();
  ui::radar::rangeInit();
  services::settings::init();
  services::adsb::setPollFn(wifiLoop);
  services::weather::setPollFn(wifiLoop);

  if (wifiSetupConnect()) {
    showRadarIfConnected();
  }
}

void loop() {
  handleBootButton();
  handleRadarTouch();
  wifiLoop();

  if (services::ota::inProgress()) {
    delay(10);
    return;
  }

  if (WiFi.status() != WL_CONNECTED) {
    if (g_radar_visible) {
      Serial.println("WiFi lost — will reconnect");
      g_radar_visible = false;
    }

    if (g_wifi_down_since == 0) {
      g_wifi_down_since = millis();
    }

    const unsigned long down_ms = millis() - g_wifi_down_since;
    if (down_ms >= config::kWifiDownGraceMs &&
        millis() - g_last_reconnect_ms >= config::kWifiReconnectIntervalMs) {
      g_last_reconnect_ms = millis();
      if (wifiReconnect()) {
        g_wifi_down_since = 0;
        showRadarIfConnected();
      }
    }
  } else {
    g_wifi_down_since = 0;
    if (!g_radar_visible) {
      showRadarIfConnected();
    } else if (millis() - g_last_adsb_fetch_ms >= config::kAdsbFetchIntervalMs) {
      g_last_adsb_fetch_ms = millis();
      fetchAndDrawAircraft();
    } else if (services::weather::refreshIfDue(
                   services::location::lat(), services::location::lon())) {
      ui::radarDisplayRefreshAircraft();
    } else if (services::adsb::enrichOnePending()) {
      ui::radarDisplayRefreshAircraft();
    }
  }

  delay(10);
}
