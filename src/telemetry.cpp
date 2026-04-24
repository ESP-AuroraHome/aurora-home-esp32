#include "telemetry.h"

#include <ArduinoJson.h>

#include <cstdio>

namespace {
void fmtFloat(char* const buf, size_t size, float v) {
    (void)snprintf(buf, size, "%.2f", v);
}
} /* namespace */

size_t telemetryFormat(char* out, size_t outSize, float tempC, float humPct, float pressureHpa,
                       float co2Ppm, float lux) {
    if (out == nullptr || outSize == 0) return 0;

    char temperature[16];
    char humidity[16];
    char pressure[16];
    char co2[16];
    char light[16];
    fmtFloat(temperature, sizeof(temperature), tempC);
    fmtFloat(humidity, sizeof(humidity), humPct);
    fmtFloat(pressure, sizeof(pressure), pressureHpa);
    fmtFloat(co2, sizeof(co2), co2Ppm);
    fmtFloat(light, sizeof(light), lux);

    JsonDocument doc;
    doc["temperature"] = temperature;
    doc["humidity"] = humidity;
    doc["pressure"] = pressure;
    doc["co2"] = co2;
    doc["light"] = light;
    const size_t written = serializeJson(doc, out, outSize);
    if (written == 0 || written >= outSize) return 0;
    return written;
}
