#include "Scd30Sensor.h"

#include <Arduino.h>
#include <Wire.h>

#ifdef NO_ERROR
#undef NO_ERROR
#endif
#define NO_ERROR 0

bool Scd30Sensor::begin() {
    device_.begin(Wire, SCD30_I2C_ADDR_61);
    device_.stopPeriodicMeasurement();
    device_.softReset();
    delay(2000);

    uint8_t major = 0;
    uint8_t minor = 0;
    if (device_.readFirmwareVersion(major, minor) != NO_ERROR) return false;

    return device_.startPeriodicMeasurement(0) == NO_ERROR;
}

bool Scd30Sensor::read(float& co2_ppm, float& temperature_C, float& humidity_pct) {
    uint16_t dataReady = 0;
    device_.getDataReady(dataReady);
    if (!dataReady) return false;

    device_.readMeasurementData(co2_ppm, temperature_C, humidity_pct);
    return true;
}
