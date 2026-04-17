#pragma once

#include <Adafruit_BME280.h>
#include <ISensor.h>

/**
 * @brief BME280 temperature / humidity / pressure sensor driver.
 */
class Bme280Sensor : public ISensor {
 public:
    bool begin() override;
    const char* name() const override { return "BME280"; }

    /**
     * @brief Read a full environmental sample.
     * @param[out] temperature_C Temperature in degrees Celsius.
     * @param[out] humidity_pct  Relative humidity in percent.
     * @param[out] pressure_hPa  Pressure in hectopascals.
     * @return true on success, false if any reading is NaN.
     */
    bool read(float& temperature_C, float& humidity_pct, float& pressure_hPa);

 private:
    Adafruit_BME280 device_;
};
