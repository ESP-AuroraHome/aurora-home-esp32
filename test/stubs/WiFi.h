#pragma once
#include <cstdint>

#include "IPAddress.h"

struct WiFiClient {};

inline int g_wifi_station_num = 0;

struct WiFiClass {
    static bool softAP(const char*, const char*) { return true; }
    static IPAddress softAPIP() { return IPAddress(192, 168, 4, 1); }
    static int softAPgetStationNum() { return g_wifi_station_num; }
};
inline WiFiClass WiFi;
