#include "telemetry.h"

#include <ArduinoJson.h>

#include <cmath>

namespace {
float round2(float v) {
    return std::roundf(v * 100.0f) / 100.0f;
}
} /* namespace */

size_t telemetryFormat(char* out, size_t outSize, float tempC, float humPct, float pressureHpa,
                       float co2Ppm, float lux) {
    if (out == nullptr || outSize == 0) return 0;

    JsonDocument doc;
    doc["temperature_c"] = round2(tempC);
    doc["humidity_pct"] = round2(humPct);
    doc["pressure_hpa"] = round2(pressureHpa);
    doc["co2_ppm"] = round2(co2Ppm);
    doc["light_lx"] = round2(lux);
    const size_t written = serializeJson(doc, out, outSize);
    if (written == 0 || written >= outSize) return 0;
    return written;
}
