#include <Arduino.h>
#include <IPAddress.h>
#include <Wire.h>

#include "Bh1750Sensor.h"
#include "Bme280Sensor.h"
#include "Config.h"
#include "Logger.h"
#include "MqttPublisher.h"
#include "Scd30Sensor.h"
#include "Telemetry.h"
#include "WifiAp.h"

namespace {

Bh1750Sensor bh1750;
Bme280Sensor bme280;
Scd30Sensor scd30;
WifiAp wifiAp(AURORA_WIFI_AP_SSID, AURORA_WIFI_AP_PASSWORD);
MqttPublisher mqtt(AURORA_MQTT_PORT, AURORA_MQTT_TOPIC_DATA);

const IPAddress kBrokerCandidates[] = {
    IPAddress(192, 168, 4, 2),
    IPAddress(192, 168, 4, 3),
    IPAddress(192, 168, 4, 4),
    IPAddress(192, 168, 4, 5),
};
constexpr size_t kBrokerCandidateCount = sizeof(kBrokerCandidates) / sizeof(kBrokerCandidates[0]);

void haltOnFailure(const char* message) {
    LOG_ERROR("%s", message);
    while (true)
        delay(1000);
}

}  // namespace

void setup() {
    Serial.begin(115200);
    while (!Serial)
        delay(100);

    Wire.begin(AURORA_I2C_SDA, AURORA_I2C_SCL);

    LOG_INFO("--- 1. Sensor initialization ---");
    if (!bh1750.begin()) LOG_ERROR("BH1750 init failed");
    if (!scd30.begin()) haltOnFailure("SCD30 init failed");
    if (!bme280.begin()) haltOnFailure("BME280 init failed");

    LOG_INFO("--- 2. Starting access point ---");
    wifiAp.begin();
    LOG_INFO("AP SSID: %s", AURORA_WIFI_AP_SSID);
    LOG_INFO("ESP32 IP: %s", wifiAp.ip().toString().c_str());

    LOG_INFO("--- 3. Waiting for client ---");
    while (!wifiAp.hasClient()) {
        delay(500);
    }
    LOG_INFO("Client connected.");
}

void loop() {
    if (!wifiAp.hasClient()) {
        delay(1000);
        return;
    }

    if (!mqtt.connected()) {
        if (!mqtt.connectScanning(kBrokerCandidates, kBrokerCandidateCount)) {
            LOG_WARN("Broker not found. Retrying in 3s...");
            delay(3000);
            return;
        }
        LOG_INFO("MQTT connected.");
    }
    mqtt.loop();

    float co2 = 0.0f;
    float scdTemp = 0.0f;
    float scdHum = 0.0f;
    if (!scd30.read(co2, scdTemp, scdHum)) {
        delay(10);
        return;
    }

    float bmeTemp = 0.0f;
    float bmeHum = 0.0f;
    float pressure = 0.0f;
    if (!bme280.read(bmeTemp, bmeHum, pressure)) LOG_WARN("BME280 read failed");

    float lux = 0.0f;
    if (!bh1750.read(lux)) LOG_WARN("BH1750 read failed");

    const float avgTemp = (scdTemp + bmeTemp) / 2.0f;
    const float avgHum = (scdHum + bmeHum) / 2.0f;

    char payload[512];
    if (Telemetry::formatJson(payload, sizeof(payload), avgTemp, avgHum, pressure, co2, lux) > 0) {
        LOG_DEBUG("Publishing: %s", payload);
        mqtt.publish(payload);
    }

    delay(10000);
}
