#pragma once

#include <ISensor.h>
#include <SensirionI2cScd30.h>

/**
 * @brief Sensirion SCD30 CO2 / temperature / humidity sensor driver.
 */
class Scd30Sensor : public ISensor {
 public:
    bool begin() override;
    const char* name() const override { return "SCD30"; }

    /**
     * @brief Read the latest measurement if one is ready.
     * @param[out] co2_ppm       CO2 concentration in ppm.
     * @param[out] temperature_C Temperature in degrees Celsius.
     * @param[out] humidity_pct  Relative humidity in percent.
     * @return true if fresh data was read, false if no new sample is available.
     */
    bool read(float& co2_ppm, float& temperature_C, float& humidity_pct);

 private:
    SensirionI2cScd30 device_;
};
