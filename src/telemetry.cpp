#include "telemetry.h"

#include <ArduinoJson.h>

size_t telemetryFormat(char* out, size_t outSize, float tempC, float humPct, float pressureHpa,
                       float co2Ppm, float lux) {
    JsonDocument doc;
    doc["temperature_c"] = tempC;
    doc["humidity_pct"] = humPct;
    doc["pressure_hpa"] = pressureHpa;
    doc["co2_ppm"] = co2Ppm;
    doc["light_lx"] = lux;
    return serializeJson(doc, out, outSize);
}
