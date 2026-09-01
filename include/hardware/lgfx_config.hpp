#pragma once

#define LGFX_USE_V1
#include <LovyanGFX.hpp>

#include "config.h"

/** LovyanGFX device: GC9A01 on SPI + CST816S. Display pins match stock. */
class LGFX : public lgfx::LGFX_Device {
  lgfx::Bus_SPI _bus;
  lgfx::Panel_GC9A01 _panel;
  lgfx::Touch_CST816S _touch;

public:
  LGFX() {
    {
      auto cfg = _bus.config();
      cfg.spi_host = SPI2_HOST;
      cfg.freq_write = config::kDisplaySpiWriteHz;
      cfg.pin_sclk = static_cast<int>(config::kDisplayPinSclk);
      cfg.pin_mosi = static_cast<int>(config::kDisplayPinMosi);
      cfg.pin_miso = -1;
      cfg.pin_dc = static_cast<int>(config::kDisplayPinDc);
      _bus.config(cfg);
      _panel.setBus(&_bus);
    }
    {
      auto cfg = _panel.config();
      cfg.pin_cs = static_cast<int>(config::kDisplayPinCs);
      cfg.pin_rst = static_cast<int>(config::kDisplayPinRst);
      cfg.invert = config::kDisplayInvert;
      cfg.rgb_order = config::kDisplayRgbOrder;
      _panel.config(cfg);
    }
    {
      auto cfg = _touch.config();
      cfg.i2c_port = 0;
      cfg.i2c_addr = 0x15;
      cfg.pin_sda = static_cast<int>(config::kTouchPinSda);
      cfg.pin_scl = static_cast<int>(config::kTouchPinScl);
      cfg.pin_int = static_cast<int>(config::kTouchPinInt);
      cfg.pin_rst = static_cast<int>(config::kTouchPinRst);
      cfg.freq = 400000;
      cfg.x_min = 0;
      cfg.x_max = config::kDisplayWidth - 1;
      cfg.y_min = 0;
      cfg.y_max = config::kDisplayHeight - 1;
      _touch.config(cfg);
      _panel.setTouch(&_touch);
    }
    setPanel(&_panel);
  }
};
