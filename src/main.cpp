#include <Arduino.h>
#include <Wire.h>
#include <esp_arduino_version.h>
#include <esp_task_wdt.h>

#include "Config.h"
#include "Logger.h"
#include "net.h"
#include "sensors.h"
#include "telemetry.h"

/**
 * @file main.cpp
 * @brief Orchestration : setup() + loop() AuroraHome firmware (ESP32).
 *
 * Toute la logique metier vit dans sensors.cpp / net.cpp / telemetry.cpp.
 */

namespace {

constexpr uint32_t kMqttBackoffInitialMs = 1000;
constexpr uint32_t kMqttBackoffMaxMs = 60000;

uint32_t mqttBackoffMs = kMqttBackoffInitialMs;
uint32_t mqttLastAttemptMs = 0;
uint32_t lastPublishMs = 0;

/**
 * @brief Dernier echantillon SCD30.
 *
 * Rafraichi ~toutes les 2s (cadence capteur), publie selon
 * AURORA_PUBLISH_INTERVAL_MS pour ne pas saturer le broker.
 */
float co2Ppm = 0.0f;
float scdTempC = 0.0f;
float scdHumPct = 0.0f;
bool haveScdData = false;

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
        delay(1000);
}

} /* namespace */

void setup() {
    Serial.begin(115200);
    while (!Serial)
        delay(100);

    Wire.begin(AURORA_I2C_SDA, AURORA_I2C_SCL);

    LOG_INFO("--- 1. Sensor initialization ---");
    if (!sensorsInitBh1750()) LOG_ERROR("BH1750 init failed");
    if (!sensorsInitScd30()) fatalReboot("SCD30 init");
    if (!sensorsInitBme280()) fatalReboot("BME280 init");

    LOG_INFO("--- 2. Starting access point ---");
    netBegin();

    LOG_INFO("--- 3. Watchdog + waiting for client ---");
    initWatchdog();
    while (!netHasClient()) {
        esp_task_wdt_reset();
        delay(500);
    }
    LOG_INFO("Client connected.");
}

void loop() {
    esp_task_wdt_reset();
    const uint32_t now = millis();

    if (!netHasClient()) {
        delay(500);
        return;
    }

    if (!netMqttConnected()) {
        if (now - mqttLastAttemptMs < mqttBackoffMs) {
            delay(50);
            return;
        }
        mqttLastAttemptMs = now;
        if (!netMqttTryConnect()) {
            LOG_WARN("Broker not found; backoff %u ms", mqttBackoffMs);
            mqttBackoffMs = min(mqttBackoffMs * 2, kMqttBackoffMaxMs);
            return;
        }
        LOG_INFO("MQTT connected.");
        mqttBackoffMs = kMqttBackoffInitialMs;
    }
    netMqttLoop();

    float c = 0.0f;
    float t = 0.0f;
    float h = 0.0f;
    if (sensorsReadScd30(c, t, h)) {
        co2Ppm = c;
        scdTempC = t;
        scdHumPct = h;
        haveScdData = true;
    }

    if (!haveScdData) {
        delay(10);
        return;
    }
    if (now - lastPublishMs < AURORA_PUBLISH_INTERVAL_MS) {
        delay(10);
        return;
    }
    lastPublishMs = now;

    float bmeTempC = 0.0f;
    float bmeHumPct = 0.0f;
    float pressureHpa = 0.0f;
    if (!sensorsReadBme280(bmeTempC, bmeHumPct, pressureHpa)) LOG_WARN("BME280 read failed");

    float lux = 0.0f;
    if (!sensorsReadBh1750(lux)) LOG_WARN("BH1750 read failed");

    const float avgTempC = (scdTempC + bmeTempC) / 2.0f;
    const float avgHumPct = (scdHumPct + bmeHumPct) / 2.0f;

    char payload[256];
    const size_t n =
        telemetryFormat(payload, sizeof(payload), avgTempC, avgHumPct, pressureHpa, co2Ppm, lux);
    if (n == 0 || n >= sizeof(payload)) {
        LOG_WARN("Payload serialization failed");
        return;
    }
    LOG_DEBUG("Publishing: %s", payload);
    netMqttPublish(AURORA_MQTT_TOPIC_DATA, payload);
}
