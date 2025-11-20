#include <SensirionI2cScd30.h>
#include <Wire.h>

#ifdef NO_ERROR
#undef NO_ERROR
#endif
#define NO_ERROR 0

SensirionI2cScd30 sensor;

bool scd30_init(Wire &Wire) {
    sensor.begin(Wire, SCD30_I2C_ADDR_61);

    sensor.stopPeriodicMeasurement();
    sensor.softReset();
    delay(2000);
    uint8_t major = 0;
    uint8_t minor = 0;
    int16_t error = sensor.readFirmwareVersion(major, minor);
    if (error != NO_ERROR) {
        return false;
    }
    error = sensor.startPeriodicMeasurement(0);
    if (error != NO_ERROR) {
        return false;
    }
    return true;
}

int scd30_read(SensirionI2cScd30 &sensor, float &co2Concentration, float &temperature, float &humidity) {
    delay(1500);
    int16_t data = sensor.blockingReadMeasurementData(co2Concentration,temperature, humidity);
    return data;
}