#include "Bh1750Sensor.h"

Bh1750Sensor::Bh1750Sensor(uint8_t address) : device_(address) {}

bool Bh1750Sensor::begin() {
    return device_.begin(BH1750::CONTINUOUS_HIGH_RES_MODE);
}

bool Bh1750Sensor::read(float& lux) {
    if (!device_.measurementReady()) return false;
    lux = device_.readLightLevel();
    return true;
}
