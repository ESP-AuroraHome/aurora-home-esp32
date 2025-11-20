#pragma once

#include <SensirionI2cScd30.h>

/**
 * Initialize the SCD30 sensor and required interfaces (e.g. I2C).
 *
 * This function performs any hardware or driver initialization necessary to
 * communicate with the SCD30. It may perform a soft reset or sensor
 * configuration depending on the implementation.
 *
 * @param[in,out] sensor Reference to the SensirionI2cScd30 sensor object to initialize.
 * @return true if initialization succeeded and the sensor is ready to use,
 *         false on failure.
 */
bool scd30_init(SensirionI2cScd30 &sensor);

/**
 * Read a measurement from the SCD30 sensor.
 *
 * The measured values are written into the provided output references.
 * The call may block while waiting for the sensor to reply.
 *
 * @param[in,out] sensor Reference to the SensirionI2cScd30 sensor object to read from.
 * @param[out] co2Concentration CO2 concentration in parts per million (ppm).
 * @param[out] temperature Temperature in degrees Celsius (°C).
 * @param[out] humidity Relative humidity in percent (%RH).
 * @return 0 on success. Non-zero on error (see note above for common codes).
 */
int scd30_read(SensirionI2cScd30 &sensor, float &co2Concentration, float &temperature, float &humidity);
