#pragma once

void statusScreenPortal();
void statusScreenConnectFailed();
void statusScreenWifiReset();
void statusScreenFirmwareUpdate();

/** Saved-network connect animation (call Tick until connect finishes). */
void statusScreenConnectingBegin(const char* ssid);
void statusScreenConnectingTick();
