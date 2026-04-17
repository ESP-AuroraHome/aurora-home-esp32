#pragma once

#include <cstddef>

/**
 * @brief Serialise un snapshot capteurs en JSON via ArduinoJson.
 *
 * Valeurs numeriques (pas d'unites encastrees dans les strings) pour que les
 * consommateurs (Home Assistant, Grafana) puissent traiter/agreger directement.
 * Les unites sont portees par le nom de cle (ex: `temperature_c`).
 */
class Telemetry {
 public:
    /**
     * @brief Serialise un snapshot capteurs en JSON.
     * @param[out] out            Buffer destination (null-termine en sortie).
     * @param      outSize        Taille de @p out en octets.
     * @param      temperatureC   Temperature en degres Celsius.
     * @param      humidityPct    Humidite relative en pourcent.
     * @param      pressureHpa    Pression en hectopascals.
     * @param      co2Ppm         Concentration CO2 en ppm.
     * @param      lux            Luminosite en lux.
     * @return Nombre d'octets ecrits (hors null terminator), ou -1 si buffer
     *         trop petit / erreur de serialisation.
     */
    static int formatJson(char* out, size_t outSize, float temperatureC, float humidityPct,
                          float pressureHpa, float co2Ppm, float lux);
};
