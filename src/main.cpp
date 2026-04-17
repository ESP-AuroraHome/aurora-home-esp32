#include <Arduino.h>
#include <IPAddress.h>
#include <Wire.h>

#include "Bh1750Sensor.h"
#include "Bme280Sensor.h"
#include "Config.h"
#include "MqttPublisher.h"
#include "Scd30Sensor.h"
#include "Telemetry.h"
#include "WifiAp.h"

namespace {

Bh1750Sensor  bh1750;
Bme280Sensor  bme280;
Scd30Sensor   scd30;
WifiAp        wifiAp(AURORA_WIFI_AP_SSID, AURORA_WIFI_AP_PASSWORD);
MqttPublisher mqtt(AURORA_MQTT_PORT, AURORA_MQTT_TOPIC);

const IPAddress kBrokerCandidates[] = {
    IPAddress(192, 168, 4, 2),
    IPAddress(192, 168, 4, 3),
    IPAddress(192, 168, 4, 4),
    IPAddress(192, 168, 4, 5),
};
constexpr size_t kBrokerCandidateCount =
    sizeof(kBrokerCandidates) / sizeof(kBrokerCandidates[0]);

void haltOnFailure(const char* message) {
    Serial.println(message);
    while (true) delay(1000);
}

}  // namespace

void setup() {
    Serial.begin(115200);
    while (!Serial) delay(100);

    Wire.begin(AURORA_I2C_SDA, AURORA_I2C_SCL);

    Serial.println("\n--- 1. Sensor initialization ---");
    if (!bh1750.begin()) Serial.println("BH1750 ERROR");
    if (!scd30.begin())  haltOnFailure("SCD30 ERROR");
    if (!bme280.begin()) haltOnFailure("BME280 ERROR");

    Serial.println("\n--- 2. Starting access point ---");
    wifiAp.begin();
    Serial.print("AP SSID: ");  Serial.println(AURORA_WIFI_AP_SSID);
    Serial.print("ESP32 IP: "); Serial.println(wifiAp.ip());

    Serial.println("\n--- 3. Waiting for client ---");
    while (!wifiAp.hasClient()) {
        Serial.print(".");
        delay(500);
    }
    Serial.println("\nClient connected.");
}

void loop() {
    if (!wifiAp.hasClient()) {
        delay(1000);
        return;
    }

    if (!mqtt.connected()) {
        if (!mqtt.connectScanning(kBrokerCandidates, kBrokerCandidateCount)) {
            Serial.println("Broker not found. Retrying in 3s...");
            delay(3000);
            return;
        }
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
    float bmeHum  = 0.0f;
    float pressure = 0.0f;
    if (!bme280.read(bmeTemp, bmeHum, pressure)) Serial.println("Warn: BME280 fail");

    float lux = 0.0f;
    if (!bh1750.read(lux)) Serial.println("Warn: BH1750 fail");

    const float avgTemp = (scdTemp + bmeTemp) / 2.0f;
    const float avgHum  = (scdHum  + bmeHum)  / 2.0f;

    char payload[512];
    if (Telemetry::formatJson(payload, sizeof(payload),
                              avgTemp, avgHum, pressure, co2, lux) > 0) {
        Serial.print("Publishing MQTT: ");
        Serial.println(payload);
        mqtt.publish(payload);
    }

    delay(10000);
}
