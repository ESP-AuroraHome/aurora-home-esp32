#include <Arduino.h>
#include <ESPmDNS.h>
#include <IPAddress.h>
#include <Wire.h>
#include <esp_arduino_version.h>
#include <esp_task_wdt.h>

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
MqttPublisher mqtt(AURORA_MQTT_PORT, AURORA_MQTT_TOPIC_DATA, AURORA_MQTT_CLIENT_ID,
                   AURORA_MQTT_TOPIC_STATUS, AURORA_MQTT_STATUS_ONLINE, AURORA_MQTT_STATUS_OFFLINE);

const IPAddress kBrokerCandidates[] = {
    IPAddress(192, 168, 4, 2),
    IPAddress(192, 168, 4, 3),
    IPAddress(192, 168, 4, 4),
    IPAddress(192, 168, 4, 5),
};
constexpr size_t kBrokerCandidateCount = sizeof(kBrokerCandidates) / sizeof(kBrokerCandidates[0]);

constexpr uint32_t kMqttBackoffInitialMs = 1000;
constexpr uint32_t kMqttBackoffMaxMs = 60000;

uint32_t mqttBackoffMs = kMqttBackoffInitialMs;
uint32_t mqttLastAttemptMs = 0;
uint32_t lastPublishMs = 0;

// Dernier echantillon SCD30 (rafraichi ~toutes les 2s, publie selon
// AURORA_PUBLISH_INTERVAL_MS pour ne pas saturer MQTT).
float co2 = 0.0f;
float scdTemp = 0.0f;
float scdHum = 0.0f;
bool haveScd = false;

void initWatchdog() {
#if defined(ESP_ARDUINO_VERSION) && ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
    const esp_task_wdt_config_t cfg = {
        .timeout_ms = AURORA_WATCHDOG_TIMEOUT_S * 1000U,
        .idle_core_mask = (1U << portNUM_PROCESSORS) - 1U,
        .trigger_panic = true,
    };
    esp_task_wdt_init(&cfg);
#else
    esp_task_wdt_init(AURORA_WATCHDOG_TIMEOUT_S, true);
#endif
    esp_task_wdt_add(nullptr);
}

[[noreturn]] void fatalReboot(const char* message) {
    LOG_ERROR("FATAL: %s - rebooting in 2s", message);
    delay(2000);
    ESP.restart();
    while (true)
        delay(1000);  // unreachable
}

}  // namespace

void setup() {
    Serial.begin(115200);
    while (!Serial)
        delay(100);

    Wire.begin(AURORA_I2C_SDA, AURORA_I2C_SCL);

    LOG_INFO("--- 1. Sensor initialization ---");
    if (!bh1750.begin()) LOG_ERROR("BH1750 init failed");
    if (!scd30.begin()) fatalReboot("SCD30 init");
    if (!bme280.begin()) fatalReboot("BME280 init");

    LOG_INFO("--- 2. Starting access point ---");
    wifiAp.begin();
    LOG_INFO("AP SSID: %s", AURORA_WIFI_AP_SSID);
    LOG_INFO("ESP32 IP: %s", wifiAp.ip().toString().c_str());

    if (!MDNS.begin(AURORA_MDNS_HOSTNAME)) {
        LOG_WARN("mDNS responder failed to start");
    } else {
        LOG_INFO("mDNS responder up as %s.local", AURORA_MDNS_HOSTNAME);
    }

    LOG_INFO("--- 3. Watchdog + waiting for client ---");
    initWatchdog();
    while (!wifiAp.hasClient()) {
        esp_task_wdt_reset();
        delay(500);
    }
    LOG_INFO("Client connected.");
}

void loop() {
    esp_task_wdt_reset();
    const uint32_t now = millis();

    if (!wifiAp.hasClient()) {
        delay(500);
        return;
    }

    if (!mqtt.connected()) {
        if (now - mqttLastAttemptMs < mqttBackoffMs) {
            delay(50);
            return;
        }
        mqttLastAttemptMs = now;
        const bool ok = mqtt.connectMdns(AURORA_MQTT_SERVICE, AURORA_MQTT_PROTO) ||
                        mqtt.connectScanning(kBrokerCandidates, kBrokerCandidateCount);
        if (!ok) {
            LOG_WARN("Broker not found; backoff %u ms", mqttBackoffMs);
            mqttBackoffMs = min(mqttBackoffMs * 2, kMqttBackoffMaxMs);
            return;
        }
        LOG_INFO("MQTT connected.");
        mqttBackoffMs = kMqttBackoffInitialMs;
    }
    mqtt.loop();

    float c = 0.0f;
    float t = 0.0f;
    float h = 0.0f;
    if (scd30.read(c, t, h)) {
        co2 = c;
        scdTemp = t;
        scdHum = h;
        haveScd = true;
    }

    if (!haveScd) {
        delay(10);
        return;
    }
    if (now - lastPublishMs < AURORA_PUBLISH_INTERVAL_MS) {
        delay(10);
        return;
    }
    lastPublishMs = now;

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
}
