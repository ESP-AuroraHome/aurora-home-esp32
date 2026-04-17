#include "telemetry.h"

#include <ArduinoJson.h>

size_t telemetryFormat(char* out, size_t outSize, float tempC, float humPct, float pressureHpa,
                       float co2Ppm, float lux) {
    if (out == nullptr || outSize == 0) return 0;

    JsonDocument doc;
    doc["temperature_c"] = tempC;
    doc["humidity_pct"] = humPct;
    doc["pressure_hpa"] = pressureHpa;
    doc["co2_ppm"] = co2Ppm;
    doc["light_lx"] = lux;
    const size_t written = serializeJson(doc, out, outSize);
    if (written == 0 || written >= outSize) return 0;
    return written;
}
