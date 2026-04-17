#pragma once

/**
 * @file sensors.h
 * @brief Init et lecture des 3 capteurs I2C (BH1750, BME280, SCD30).
 *
 * Les instances sont internes a sensors.cpp : on expose uniquement des
 * fonctions libres. Aucune classe wrapper autour des libs vendor.
 */

bool sensorsInitBh1750();
bool sensorsInitBme280();
bool sensorsInitScd30();

/**
 * @brief Lit un echantillon SCD30 si un nouveau est pret (non-bloquant).
 * @return false si aucun echantillon frais, true sinon.
 */
bool sensorsReadScd30(float& co2Ppm, float& tempC, float& humPct);

bool sensorsReadBme280(float& tempC, float& humPct, float& pressureHpa);

/**
 * @brief Lit le luxmetre si une mesure est prete (non-bloquant).
 */
bool sensorsReadBh1750(float& lux);
