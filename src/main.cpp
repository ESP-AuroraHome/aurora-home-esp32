#include <Adafruit_BME280.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include <BH1750.h>
#include <ESPmDNS.h>
#include <IPAddress.h>
#include <PubSubClient.h>
#include <SensirionI2cScd30.h>
#include <WiFi.h>
#include <Wire.h>
#include <esp_arduino_version.h>
#include <esp_task_wdt.h>

#include "Config.h"
#include "Logger.h"

/**
 * @file main.cpp
 * @brief AuroraHome firmware (ESP32).
 *
 * ESP32 agit en Wi-Fi soft-AP ; un broker MQTT tourne sur le client Orange Pi
 * sur le sous-reseau 192.168.4.x. Le firmware :
 *  - initialise les 3 capteurs I2C (BH1750, BME280, SCD30),
 *  - arme un watchdog task (ESP.restart si la loop se bloque),
 *  - attend le premier client Wi-Fi puis tente une connexion MQTT avec
 *    decouverte mDNS prioritaire et fallback scan IP,
 *  - publie un snapshot JSON (ArduinoJson) toutes les AURORA_PUBLISH_INTERVAL_MS
 *    en respectant la cadence d'echantillonnage SCD30 (~2s),
 *  - arme un Last Will retained sur AURORA_MQTT_TOPIC_STATUS.
 */

namespace {

// ----------------------------------------------------------------------------
// Constantes runtime.
// ----------------------------------------------------------------------------

/// IPs candidates si la resolution mDNS echoue (Orange Pi, DHCP softAP).
const IPAddress kBrokerCandidates[] = {
    IPAddress(192, 168, 4, 2),
    IPAddress(192, 168, 4, 3),
    IPAddress(192, 168, 4, 4),
    IPAddress(192, 168, 4, 5),
};
constexpr size_t kBrokerCandidateCount = sizeof(kBrokerCandidates) / sizeof(kBrokerCandidates[0]);

/// Backoff exponentiel sur tentative de reconnexion MQTT.
constexpr uint32_t kMqttBackoffInitialMs = 1000;
constexpr uint32_t kMqttBackoffMaxMs = 60000;

/// LWT arme sur chaque connexion.
constexpr uint8_t kWillQos = 1;
constexpr bool kWillRetain = true;

// ----------------------------------------------------------------------------
// Objets globaux.
// ----------------------------------------------------------------------------

SensirionI2cScd30 scd30;
BH1750 bh1750(0x23);
Adafruit_BME280 bme280;

WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

// ----------------------------------------------------------------------------
// Etat runtime.
// ----------------------------------------------------------------------------

uint32_t mqttBackoffMs = kMqttBackoffInitialMs;
uint32_t mqttLastAttemptMs = 0;
uint32_t lastPublishMs = 0;

// Dernier echantillon SCD30 (rafraichi ~toutes les 2s, publie a la cadence
// AURORA_PUBLISH_INTERVAL_MS pour ne pas saturer le broker).
float co2Ppm = 0.0f;
float scdTempC = 0.0f;
float scdHumPct = 0.0f;
bool haveScdData = false;

// ----------------------------------------------------------------------------
// Helpers.
// ----------------------------------------------------------------------------

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

bool initScd30() {
    scd30.begin(Wire, SCD30_I2C_ADDR_61);
    scd30.stopPeriodicMeasurement();
    scd30.softReset();
    delay(2000);
    uint8_t major = 0;
    uint8_t minor = 0;
    if (scd30.readFirmwareVersion(major, minor) != 0) return false;
    return scd30.startPeriodicMeasurement(0) == 0;
}

bool readScd30(float& co2, float& tempC, float& humPct) {
    uint16_t dataReady = 0;
    scd30.getDataReady(dataReady);
    if (!dataReady) return false;
    return scd30.readMeasurementData(co2, tempC, humPct) == 0;
}

bool initBme280() {
    if (!bme280.begin(0x76) && !bme280.begin(0x77)) return false;
    bme280.setSampling(Adafruit_BME280::MODE_NORMAL, Adafruit_BME280::SAMPLING_X2,
                       Adafruit_BME280::SAMPLING_X16, Adafruit_BME280::SAMPLING_X1,
                       Adafruit_BME280::FILTER_X16, Adafruit_BME280::STANDBY_MS_500);
    return true;
}

bool readBme280(float& tempC, float& humPct, float& pressureHpa) {
    tempC = bme280.readTemperature();
    humPct = bme280.readHumidity();
    pressureHpa = bme280.readPressure() / 100.0f;
    return !isnan(tempC) && !isnan(humPct) && !isnan(pressureHpa);
}

bool readBh1750(float& lux) {
    if (!bh1750.measurementReady()) return false;
    lux = bh1750.readLightLevel();
    return true;
}

bool mqttConnectWithLwt() {
    if (!mqtt.connect(AURORA_MQTT_CLIENT_ID, nullptr, nullptr, AURORA_MQTT_TOPIC_STATUS, kWillQos,
                      kWillRetain, AURORA_MQTT_STATUS_OFFLINE)) {
        return false;
    }
    mqtt.publish(AURORA_MQTT_TOPIC_STATUS, AURORA_MQTT_STATUS_ONLINE, kWillRetain);
    return true;
}

bool mqttConnectMdns() {
    const int found = MDNS.queryService(AURORA_MQTT_SERVICE, AURORA_MQTT_PROTO);
    for (int i = 0; i < found; ++i) {
        mqtt.setServer(MDNS.IP(i), MDNS.port(i));
        if (mqttConnectWithLwt()) return true;
    }
    return false;
}

bool mqttConnectScan() {
    for (size_t i = 0; i < kBrokerCandidateCount; ++i) {
        mqtt.setServer(kBrokerCandidates[i], AURORA_MQTT_PORT);
        if (mqttConnectWithLwt()) return true;
    }
    return false;
}

size_t formatPayload(char* out, size_t outSize, float tempC, float humPct, float pressureHpa,
                     float co2, float lux) {
    JsonDocument doc;
    doc["temperature_c"] = tempC;
    doc["humidity_pct"] = humPct;
    doc["pressure_hpa"] = pressureHpa;
    doc["co2_ppm"] = co2;
    doc["light_lx"] = lux;
    return serializeJson(doc, out, outSize);
}

}  // namespace

// ----------------------------------------------------------------------------
// Arduino entry points.
// ----------------------------------------------------------------------------

void setup() {
    Serial.begin(115200);
    while (!Serial)
        delay(100);

    Wire.begin(AURORA_I2C_SDA, AURORA_I2C_SCL);

    LOG_INFO("--- 1. Sensor initialization ---");
    if (!bh1750.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) LOG_ERROR("BH1750 init failed");
    if (!initScd30()) fatalReboot("SCD30 init");
    if (!initBme280()) fatalReboot("BME280 init");

    LOG_INFO("--- 2. Starting access point ---");
    WiFi.softAP(AURORA_WIFI_AP_SSID, AURORA_WIFI_AP_PASSWORD);
    LOG_INFO("AP SSID: %s", AURORA_WIFI_AP_SSID);
    LOG_INFO("ESP32 IP: %s", WiFi.softAPIP().toString().c_str());

    if (!MDNS.begin(AURORA_MDNS_HOSTNAME)) {
        LOG_WARN("mDNS responder failed to start");
    } else {
        LOG_INFO("mDNS responder up as %s.local", AURORA_MDNS_HOSTNAME);
    }

    LOG_INFO("--- 3. Watchdog + waiting for client ---");
    initWatchdog();
    while (WiFi.softAPgetStationNum() == 0) {
        esp_task_wdt_reset();
        delay(500);
    }
    LOG_INFO("Client connected.");
}

void loop() {
    esp_task_wdt_reset();
    const uint32_t now = millis();

    if (WiFi.softAPgetStationNum() == 0) {
        delay(500);
        return;
    }

    if (!mqtt.connected()) {
        if (now - mqttLastAttemptMs < mqttBackoffMs) {
            delay(50);
            return;
        }
        mqttLastAttemptMs = now;
        if (!mqttConnectMdns() && !mqttConnectScan()) {
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
    if (readScd30(c, t, h)) {
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
    if (!readBme280(bmeTempC, bmeHumPct, pressureHpa)) LOG_WARN("BME280 read failed");

    float lux = 0.0f;
    if (!readBh1750(lux)) LOG_WARN("BH1750 read failed");

    const float avgTempC = (scdTempC + bmeTempC) / 2.0f;
    const float avgHumPct = (scdHumPct + bmeHumPct) / 2.0f;

    char payload[256];
    const size_t n =
        formatPayload(payload, sizeof(payload), avgTempC, avgHumPct, pressureHpa, co2Ppm, lux);
    if (n == 0 || n >= sizeof(payload)) {
        LOG_WARN("Payload serialization failed");
        return;
    }
    LOG_DEBUG("Publishing: %s", payload);
    mqtt.publish(AURORA_MQTT_TOPIC_DATA, payload);
}
