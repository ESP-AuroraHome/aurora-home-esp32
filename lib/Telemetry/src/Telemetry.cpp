#include "Telemetry.h"

#include <stdio.h>

int Telemetry::formatJson(char* out, size_t out_size, float temperature_C, float humidity_pct,
                          float pressure_hPa, float co2_ppm, float lux) {
    const int n = snprintf(out, out_size,
                           "{"
                           "\"temperature\": \"%.2f °C\", "
                           "\"humidity\": \"%.2f %%\", "
                           "\"pressure\": \"%.2f hPa\", "
                           "\"co2\": \"%.0f ppm\", "
                           "\"light\": \"%.0f lx\""
                           "}",
                           temperature_C, humidity_pct, pressure_hPa, co2_ppm, lux);

    if (n < 0 || static_cast<size_t>(n) >= out_size) return -1;
    return n;
}
