@echo off
cd /d "%~dp0"
set PIO=C:\Users\User\.platformio\penv\Scripts\pio.exe
set PORT=

echo.
echo Flash Super Mini radar features onto the Waveshare Touch board.
echo Flash layout matches the stock firmware that recovered the unit.
echo Do not use a 16MB merged image.
echo.
echo 1. Plug the board in with a data USB cable.
echo 2. Hold BOOT, tap RESET, keep holding BOOT.
echo 3. Press a key here.
pause

for /f "usebackq delims=" %%P in (`powershell -NoProfile -Command "$m = Get-CimInstance Win32_SerialPort | Where-Object { $_.Name -match 'CH343' -or $_.PNPDeviceID -match 'VID_1A86' } | Select-Object -First 1; if ($m) { $m.DeviceID }"`) do set PORT=%%P

echo.
if defined PORT (
  echo Using %PORT%
  "%PIO%" run -e supermini -t upload --upload-port %PORT%
) else (
  echo CH343 port not found automatically. PlatformIO will try to pick a port.
  "%PIO%" run -e supermini -t upload
)

if errorlevel 1 (
  echo.
  echo Upload failed. Hold BOOT, tap RESET, keep holding BOOT, then press a key to retry.
  pause
  "%PIO%" run -e supermini -t upload
)

echo.
echo If it succeeded, press RESET on the board.
echo Setup AP: PlaneRadarTouch-Setup
echo Browser:  http://planeradartouch.local
pause
