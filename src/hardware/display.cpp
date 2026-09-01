#include "hardware/display.h"

#include <Arduino.h>

#include "config.h"
#include "hardware/display_font.h"

LGFX tft;

void displayInit() {
  pinMode(config::kDisplayPinBl, OUTPUT);
  digitalWrite(config::kDisplayPinBl, HIGH);

  tft.init();
  tft.setRotation(0);
  tft.setBrightness(255);
  tft.setTextWrap(false);
  displayFontInit();
}
