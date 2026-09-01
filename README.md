# Plane Radar

<img width="800" height="450" alt="plane-radar" src="https://github.com/user-attachments/assets/716d0992-dab8-47ba-8f1a-2aec7f607419" />

**3D printed case (STL + assembly):** [MakerWorld — Waveshare ESP32-S3 1.28″](https://makerworld.com/en/models/2913572-esp32-s3-1-28-waveshare-plane-radar#profileId-3258909) · **Firmware:** [Releases](../../releases)

The Super Mini / non-touch case is a different print and does **not** fit this board.

Firmware for the **Waveshare ESP32-S3-Touch-LCD-1.28** (GC9A01 240×240 with CST816S capacitive touch). Shows a circular **ADS-B radar** around your configured location, with flight routes, detailed aircraft models, local weather/time, browser settings, authenticated OTA updates, and a tap-to-inspect plane card.

## What it does

1. **Wi‑Fi setup** (if needed) — captive portal on AP **`PlaneRadarTouch-Setup`**
2. **Radar** — live aircraft from [adsb.fi](https://opendata.adsb.fi/) on a sonar-style grid
3. **Useful labels** — route (for example `BOS-IND`), detailed aircraft model, and altitude
4. **Readable footer** — current conditions, temperature, humidity, local time, and date

After Wi‑Fi is saved, the device reconnects automatically; the radar runs in the main loop with periodic ADS-B updates (~3 s).

## Controls

| Action | Effect |
|--------|--------|
| **Tap the weather/time footer** | Cycle range preset (5 → 10 → 15 → 25 km); saved to flash |
| **Tap a plane, its tag, or a rim dot** | Open a detail card (callsign, route, type, altitude, heading, speed, distance) |
| **Tap the card or empty space** | Dismiss the detail card |
| **Hold BOOT 3 s** (GPIO 0) | Factory-reset Wi‑Fi, location, units, display settings, and OTA password; reboot into setup portal |

During setup you can also hold BOOT at power-on to force a credential reset (same as the long press).

## Wi‑Fi setup portal

**First-time setup** (no saved Wi‑Fi):

1. Connect to **`PlaneRadarTouch-Setup`**
2. Open **`http://planeradartouch.local`** (preferred) or **`http://192.168.4.1`** — both are shown on the yellow setup screen; captive portal may open automatically
3. Set home Wi‑Fi, then save

**Reconfigure anytime** (after the device is on your network):

1. Open **`http://planeradartouch.local`** or **`http://<device-ip>`** (e.g. from your router or serial log at boot)
2. Choose **Setup**
3. Change coordinates, display options, units, or OTA password; save

The same portal runs on the setup AP and on the device’s LAN IP while connected to Wi‑Fi. mDNS hostname is `planeradartouch` → **planeradartouch.local** (`kPortalHostname` in `config.h`). Some clients resolve `.local` slowly; use the IP if needed.

Changing coordinates no longer requires a credential reset. The new position is validated in the browser and firmware, saved to NVS, and used by the next aircraft/weather refresh.

**Custom fields** (stored in NVS):

| Field | Purpose |
|-------|---------|
| **Latitude / Longitude** | Radar center and ADS-B query position (defaults in `config.h` until set) |
| **Top bearing** | Phone-compass heading that should sit at the top of the screen (0 = north) |
| **Display distances in miles** | Ring scale label in **mi** instead of **km** (e.g. `6mi` vs `10km`) |
| **Show airport runways** | Major-airport runway overlay on the radar (off to hide) |
| **Show weather and clock** | Enables/disables the complete bottom footer |
| **Show current weather** | Shows current condition, temperature, and humidity |
| **Temperature in Fahrenheit** | Uses °F instead of °C |
| **Use 24-hour clock** | Uses 24-hour instead of compact 12-hour time |
| **Radar text size (%)** | Scales radar labels and footer text from 80–130%; default is 110% |
| **OTA password** | Password for firmware uploads; username is `admin` |

After a reset, the device reboots and shows the setup screen immediately (no “Connecting” loop on stale credentials).

## Radar display

### Grid

- Dark blue background, subdued green rings and crosshairs
- White **N / S / E / W** at the bezel; range label on the **east** spoke (ring 3 = ¾ of outer radius)
- White center dot

Layout and colors: `include/ui/radar_theme.h`.

### Range presets

| Ring 3 label | Outer radius (aircraft scale) |
|------------|-------------------------------|
| 5 km / 3 mi | ~6.7 km |
| 10 km / 6 mi | ~13.3 km (default) |
| 15 km / 9 mi | ~20 km |
| 25 km / 16 mi | ~33.3 km |

Preset and miles/km choice persist across reboot (`planeradar` NVS namespace).

### Runways

- Major airports from OurAirports (`large_airport`); all open runway strips in range (helipads excluded)
- Teal runway lines with one ICAO label per airport (e.g. `KJFK`); toggle in the Wi‑Fi setup portal
- Update the embedded list: `python3 scripts/build_large_airports.py`

### Aircraft

- **Inside the outer ring** — red heading triangle, magenta speed vector (clipped at the ring), route / detailed type / altitude tags
- **Outside the ring** (still within ADS-B fetch) — small **red dot on the screen rim** at the correct bearing (direction cue; not distance-accurate past the ring)
- **Tags** — placed toward the **center**: west (left) → tag on the **right** of the symbol; east (right) → tag on the **left**
- **Route fallback** — the callsign is shown until route data arrives, and remains the fallback when no route is known
- **Detailed type** — aircraft data is compacted for the display (for example `Boeing 737-800` → `B737-800`)

As range decreases (or aircraft approach), targets move inward; beyond-ring dots become full symbols when they cross the outer ring.

Origin/destination is not transmitted in ADS-B messages. The firmware enriches each active callsign through [ADSBDB](https://www.adsbdb.com/), rate-limits lookups, and caches successful results for six hours (misses for ten minutes). It first requests aircraft and route together, then retries the callsign-only endpoint when ADSBDB does not recognize the aircraft hex code. Route databases are based on known/scheduled callsigns, so private, repositioning, diverted, or recently changed flights may still have no route or an imperfect match.

### Weather and time

The bottom overlay uses the radar coordinates. Current conditions come from [Open-Meteo](https://open-meteo.com/) every 15 minutes; its location timezone offset and NTP provide the clock. The two rows use plain-language conditions and label relative humidity (`RH`):

```text
OVERCAST 82F RH100%
21:45 25 JUL
```

Disable just the weather row or the complete footer in **Setup**. Radar and
footer text defaults to 110% and can be adjusted from 80–130% in the same page.

### ADS-B

- Source: `https://opendata.adsb.fi/api/v3/`
- Route and aircraft enrichment: `https://api.adsbdb.com/v0/`
- Fetch radius: `ui::radar::fetchRadiusKm()` — scales with the active preset to roughly the screen edge (so rim dots have data)
- Poll interval: `kAdsbFetchIntervalMs` (3 s) in `config.h`
- Ground aircraft hidden by default (`kAdsbShowGroundAircraft`)

The device sends the configured coordinates to adsb.fi and Open-Meteo, and sends active callsign/Mode-S identifiers to ADSBDB.

## Configuration

Edit **`include/config.h`** for hardware and behavior:

| Area | Keys / notes |
|------|----------------|
| Portal | `kPortalApName`, `kPortalIp`, `kPortalHostname` / `kPortalHostUrl` (mDNS; needs `-DWM_MDNS` in `platformio.ini`) |
| Wi‑Fi timing | connect attempts, reconnect grace, portal timeout (`0` = no timeout) |
| BOOT | `kBootPin`, `kBootResetHoldMs`, `kBootTapMinMs` |
| Display SPI | pins, `kDisplayInvert`, `kDisplayRgbOrder`, `kDisplaySpiWriteHz` |
| Default location | `kDefaultRadarLat`, `kDefaultRadarLon` (until portal overrides) |
| ADS-B | `kAdsbFetchIntervalMs`, `kAdsbShowGroundAircraft` |
| Flight enrichment | lookup interval, timeout, and cache durations |
| Weather | endpoint, request timeout, and refresh interval |
| Defaults | initial OTA credentials |

Range presets: `include/ui/radar_range.h` (`kRangePresets`).

## Project layout

```
include/
  config.h
  hardware/
    lgfx_config.hpp
    display.h
    display_font.h
  data/
    large_airports.h
  ui/
    radar_theme.h
    radar_range.h
    radar_display.h
    runway_overlay.h
    status_screens.h
  services/
    wifi_setup.h
    radar_location.h
    adsb_client.h
    display_settings.h
    ota_update.h
    weather_time.h
data/
  ui_font.vlw              — embedded smooth UI font (Noto Sans Bold)
scripts/
  build_large_airports.py
src/
  main.cpp
  data/
    large_airports_data.cpp
  hardware/
  ui/
  services/
```

## Wiring (Waveshare ESP32-S3-Touch-LCD-1.28)

On-board GC9A01 + CST816S. Flash layout matches the stock 8 MB image that boots this board (no 16 MB / PSRAM overrides).

| Function | GPIO |
|----------|------|
| Display RST | 14 |
| Display CS | 9 |
| Display DC | 8 |
| Display MOSI (SDA) | 11 |
| Display SCLK (SCL) | 10 |
| Display backlight | 2 |
| Touch SDA | 6 |
| Touch SCL | 7 |
| Touch INT | 5 |
| Touch RST | 13 |
| BOOT (user) | 0 |

USB-C on this board is a **CH343** UART, not native USB. Put the board in download mode (hold **BOOT**, tap **RESET**, keep holding **BOOT**) before serial upload.

## Build

```bash
pio run -t upload
pio device monitor
```

- PlatformIO env: **`supermini`** (board `esp32-s3-devkitm-1`, default 8 MB partitions)
- Serial: **115200** baud, CH343 COM port
- Windows helper: double-click `flash.bat` after holding BOOT

### Web-flashable release image

Merged `.bin` for [esptool-js](https://espressif.github.io/esptool-js/) (ESP32-S3, 8 MB, flash at **0x0**):

```bash
chmod +x scripts/merge-firmware.sh   # once
./scripts/merge-firmware.sh
```

Writes `release/plane-radar-merged.bin`. Skip rebuild if firmware is already built:

```bash
./scripts/merge-firmware.sh --no-build
```

Or via PlatformIO only (output: `.pio/build/supermini/firmware-merged.bin`):

```bash
pio run -e supermini
pio run -t merge -e supermini
```

Put the board in download mode (hold **BOOT**, tap **RESET**), then flash with Chrome/Edge over USB.

### OTA firmware updates

The firmware uses two 1.75 MB application slots. After the OTA-capable partition table is installed, updates can be uploaded without USB:

1. Open `http://planeradartouch.local`
2. Choose **Firmware update**
3. Sign in with username `admin` and your configured OTA password
4. Upload the release file ending in **`-ota.bin`** (or PlatformIO's `.pio/build/supermini/firmware.bin`)
5. Keep power connected while the device writes flash and restarts

The initial password is **`plane-radar`**. Change it under **Setup** before using the device on a shared network.

> **One-time migration:** firmware built with the old single-app partition cannot install this new partition table through app-only OTA. Flash the new **`-full.bin`**/merged image over USB once. Every later update can use the OTA image.

Never upload the merged/full image to the OTA form; it contains the bootloader and partition table and is only for flashing at offset `0x0`.

### CI and releases (GitHub Actions)

| Workflow | When | Output |
|----------|------|--------|
| [Build](.github/workflows/build.yml) | Push / PR to `main` | Artifact `plane-radar-touch` (merged + split `.bin` files, ~90 days) |
| [Release](.github/workflows/release.yml) | Git tag `v*` (e.g. `v1.0.0`) | GitHub Release `-full.bin` and `-ota.bin` assets + checksums |

To ship a version users can download:

```bash
git tag v1.0.0
git push origin v1.0.0
```

The release workflow attaches both images. Use `-full.bin` at offset `0x0` for first install/recovery and `-ota.bin` in the device's authenticated firmware page.

## Dependencies

- [LovyanGFX](https://github.com/lovyan03/LovyanGFX)
- [WiFiManager](https://github.com/tzapu/WiFiManager)
- [ArduinoJson](https://github.com/bblanchon/ArduinoJson)

Runtime data services:

- [adsb.fi](https://opendata.adsb.fi/) — nearby aircraft
- [ADSBDB](https://www.adsbdb.com/) — route and detailed aircraft data
- [Open-Meteo](https://open-meteo.com/) — current weather and local timezone offset
