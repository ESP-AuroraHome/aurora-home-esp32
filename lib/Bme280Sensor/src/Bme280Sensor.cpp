#include "Bme280Sensor.h"

#include <math.h>

bool Bme280Sensor::begin() {
    if (!device_.begin(0x76) && !device_.begin(0x77)) return false;

    device_.setSampling(Adafruit_BME280::MODE_NORMAL,
                        Adafruit_BME280::SAMPLING_X2,
                        Adafruit_BME280::SAMPLING_X16,
                        Adafruit_BME280::SAMPLING_X1,
                        Adafruit_BME280::FILTER_X16,
                        Adafruit_BME280::STANDBY_MS_500);
    return true;
}

bool Bme280Sensor::read(float& temperature_C, float& humidity_pct, float& pressure_hPa) {
    temperature_C = device_.readTemperature();
    humidity_pct  = device_.readHumidity();
    pressure_hPa  = device_.readPressure() / 100.0f;

    return !isnan(temperature_C) && !isnan(humidity_pct) && !isnan(pressure_hPa);
}
