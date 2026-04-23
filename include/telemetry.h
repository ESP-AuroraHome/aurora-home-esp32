#pragma once

#include <cstddef>

/**
 * @file telemetry.h
 * @brief Serialisation JSON du snapshot capteurs (ArduinoJson).
 *
 * Valeurs numeriques; les unites sont portees par le nom de cle
 * (temperature_c, humidity_pct, pressure_hpa, co2_ppm, light_lx).
 */

/**
 * @brief Serialise un snapshot en JSON dans @p out.
 * @return Nombre d'octets ecrits (hors null terminator). 0 si serialisation
 *         echoue ou buffer trop petit (verifier `n > 0 && n < outSize`).
 */
size_t telemetryFormat(char* out, size_t outSize, float tempC, float humPct, float pressureHpa,
                       float co2Ppm, float lux);
