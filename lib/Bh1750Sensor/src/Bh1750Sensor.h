#pragma once

#include <BH1750.h>
#include <ISensor.h>
#include <stdint.h>

/**
 * @brief BH1750 ambient light sensor driver.
 *
 * Thin wrapper around the claws/BH1750 library that matches the ISensor
 * contract used throughout the firmware.
 */
class Bh1750Sensor : public ISensor {
 public:
    /**
     * @brief Construct the sensor driver.
     * @param address I2C address (0x23 = ADDR pin low, 0x5C = ADDR high).
     */
    explicit Bh1750Sensor(uint8_t address = 0x23);

    bool begin() override;
    const char* name() const override { return "BH1750"; }

    /**
     * @brief Read the current light level if a new measurement is ready.
     * @param[out] lux Populated with the measured light level in lux.
     * @return true if a fresh measurement was read, false otherwise.
     */
    bool read(float& lux);

 private:
    BH1750 device_;
};
