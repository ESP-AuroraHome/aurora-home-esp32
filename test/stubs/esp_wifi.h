#pragma once
#include <cstdint>

typedef int32_t esp_err_t;
#define ESP_OK 0

struct wifi_sta_list_t {};

inline bool g_esp_wifi_ok = true;

inline esp_err_t esp_wifi_ap_get_sta_list(wifi_sta_list_t*) {
    return g_esp_wifi_ok ? ESP_OK : -1;
}
