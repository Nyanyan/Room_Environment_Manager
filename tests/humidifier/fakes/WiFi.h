#pragma once
#include "Arduino.h"
constexpr int WIFI_STA = 1, WIFI_IF_STA = 1;
struct FakeWiFi {
  void mode(int) {}
  void disconnect() {}
  const char *macAddress() { return "02:00:00:00:00:01"; }
};
inline FakeWiFi WiFi;
