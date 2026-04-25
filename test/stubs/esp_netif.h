#pragma once
#include <cstdint>

#include "esp_wifi.h"

struct esp_ip4_addr_t {
    uint32_t addr;
};
struct esp_netif_sta_entry_t {
    esp_ip4_addr_t ip;
};
struct esp_netif_sta_list_t {
    esp_netif_sta_entry_t sta[4];
    int num;
};

inline bool g_esp_netif_ok = true;
inline int g_esp_netif_num = 0;
inline uint32_t g_esp_netif_ip = 0;

inline esp_err_t esp_netif_get_sta_list(const wifi_sta_list_t*, esp_netif_sta_list_t* out) {
    if (!g_esp_netif_ok) return -1;
    out->num = g_esp_netif_num;
    if (g_esp_netif_num > 0) out->sta[0].ip.addr = g_esp_netif_ip;
    return ESP_OK;
}
