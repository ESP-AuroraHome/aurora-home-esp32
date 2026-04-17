#include "sensors.h"

#include <Adafruit_BME280.h>
#include <Arduino.h>
#include <BH1750.h>
#include <SensirionI2cScd30.h>
#include <Wire.h>

namespace {
SensirionI2cScd30 scd30;
BH1750 bh1750(0x23);
Adafruit_BME280 bme280;
}  // namespace

bool sensorsInitBh1750() {
    return bh1750.begin(BH1750::CONTINUOUS_HIGH_RES_MODE);
}

bool sensorsInitBme280() {
    if (!bme280.begin(0x76) && !bme280.begin(0x77)) return false;
    bme280.setSampling(Adafruit_BME280::MODE_NORMAL, Adafruit_BME280::SAMPLING_X2,
                       Adafruit_BME280::SAMPLING_X16, Adafruit_BME280::SAMPLING_X1,
                       Adafruit_BME280::FILTER_X16, Adafruit_BME280::STANDBY_MS_500);
    return true;
}

bool sensorsInitScd30() {
    scd30.begin(Wire, SCD30_I2C_ADDR_61);
    scd30.stopPeriodicMeasurement();
    scd30.softReset();
    delay(2000);
    uint8_t major = 0;
    uint8_t minor = 0;
    if (scd30.readFirmwareVersion(major, minor) != 0) return false;
    return scd30.startPeriodicMeasurement(0) == 0;
}

bool sensorsReadScd30(float& co2Ppm, float& tempC, float& humPct) {
    uint16_t dataReady = 0;
    scd30.getDataReady(dataReady);
    if (!dataReady) return false;
    return scd30.readMeasurementData(co2Ppm, tempC, humPct) == 0;
}

bool sensorsReadBme280(float& tempC, float& humPct, float& pressureHpa) {
    tempC = bme280.readTemperature();
    humPct = bme280.readHumidity();
    pressureHpa = bme280.readPressure() / 100.0f;
    return !isnan(tempC) && !isnan(humPct) && !isnan(pressureHpa);
}

bool sensorsReadBh1750(float& lux) {
    if (!bh1750.measurementReady()) return false;
    lux = bh1750.readLightLevel();
    return true;
}
