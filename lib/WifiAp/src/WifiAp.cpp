#include "WifiAp.h"

#include <WiFi.h>

WifiAp::WifiAp(const char* ssid, const char* password)
    : ssid_(ssid), password_(password) {}

bool WifiAp::begin() {
    return WiFi.softAP(ssid_, password_);
}

size_t WifiAp::clientCount() const {
    return WiFi.softAPgetStationNum();
}

IPAddress WifiAp::ip() const {
    return WiFi.softAPIP();
}
