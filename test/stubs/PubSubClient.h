#pragma once
#include <cstdint>

#include "IPAddress.h"
#include "WiFi.h"

/* Controllable state — reset in setUp() between tests. */
inline bool g_mqtt_connect_result = true;
inline int g_mqtt_connect_fail_n = 0; /* fail first N calls, then succeed */
inline bool g_mqtt_connected = false;
inline int g_mqtt_connect_calls = 0;
inline int g_mqtt_set_server_calls = 0;
inline int g_mqtt_publish_calls = 0;
inline IPAddress g_mqtt_last_server;
inline uint16_t g_mqtt_last_port = 0;

class PubSubClient {
 public:
    explicit PubSubClient(WiFiClient&) {}

    PubSubClient& setServer(IPAddress ip, uint16_t port) {
        ++g_mqtt_set_server_calls;
        g_mqtt_last_server = ip;
        g_mqtt_last_port = port;
        return *this;
    }

    PubSubClient& setKeepAlive(uint16_t) { return *this; }

    bool connect(const char*, const char*, const char*, const char*, uint8_t, bool, const char*) {
        ++g_mqtt_connect_calls;
        bool ok = g_mqtt_connect_result && (g_mqtt_connect_calls > g_mqtt_connect_fail_n);
        if (ok) g_mqtt_connected = true;
        return ok;
    }

    bool connected() const { return g_mqtt_connected; }

    bool publish(const char*, const char*, bool = false) {
        ++g_mqtt_publish_calls;
        return true;
    }

    bool loop() { return g_mqtt_connected; }
};
