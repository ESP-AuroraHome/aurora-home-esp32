#pragma once
#include <cstdint>

#include "IPAddress.h"

inline int g_mdns_service_count = 0;
inline bool g_mdns_begin_result = true;
inline IPAddress g_mdns_ips[4];
inline uint16_t g_mdns_ports[4] = {};

struct MDNSClass {
    static bool begin(const char*) { return g_mdns_begin_result; }
    static int queryService(const char*, const char*) { return g_mdns_service_count; }
    static IPAddress IP(int i) { return g_mdns_ips[i]; }
    static uint16_t port(int i) { return g_mdns_ports[i]; }
};
inline MDNSClass MDNS;
