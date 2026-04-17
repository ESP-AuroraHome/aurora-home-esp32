#pragma once

#include <stddef.h>

/**
 * @brief Formats sensor measurements into a JSON payload.
 *
 * Kept deliberately minimal (snprintf) so it stays host-testable. Will switch
 * to ArduinoJson at step 8 (see CLAUDE.md) once the CI has native tests wired.
 */
class Telemetry {
 public:
    /**
     * @brief Serialize a full sensor snapshot as a JSON string.
     * @param[out] out            Destination buffer (null-terminated on success).
     * @param      out_size       Size of @p out in bytes.
     * @param      temperature_C  Temperature in degrees Celsius.
     * @param      humidity_pct   Relative humidity in percent.
     * @param      pressure_hPa   Pressure in hectopascals.
     * @param      co2_ppm        CO2 concentration in ppm.
     * @param      lux            Light level in lux.
     * @return Number of bytes written (excluding the null terminator),
     *         or -1 on error or buffer too small.
     */
    static int formatJson(char* out, size_t out_size, float temperature_C, float humidity_pct,
                          float pressure_hPa, float co2_ppm, float lux);
};
