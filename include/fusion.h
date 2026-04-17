#pragma once

/**
 * @file fusion.h
 * @brief Fusion capteurs : logique pure testable en natif.
 *
 * Header-only, aucune dependance Arduino/ESP32. Compile tel quel dans
 * l'env PlatformIO `native` (Unity, sans hardware).
 */

/**
 * @brief Moyenne arithmetique de deux mesures.
 */
inline float fusionAverage(float a, float b) {
    return (a + b) / 2.0f;
}
