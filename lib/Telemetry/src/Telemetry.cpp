#include "Telemetry.h"

#include <ArduinoJson.h>

int Telemetry::formatJson(char* out, size_t outSize, float temperatureC, float humidityPct,
                          float pressureHpa, float co2Ppm, float lux) {
    JsonDocument doc;
    doc["temperature_c"] = temperatureC;
    doc["humidity_pct"] = humidityPct;
    doc["pressure_hpa"] = pressureHpa;
    doc["co2_ppm"] = co2Ppm;
    doc["light_lx"] = lux;

    const size_t n = serializeJson(doc, out, outSize);
    if (n == 0 || n >= outSize) return -1;
    return static_cast<int>(n);
}
