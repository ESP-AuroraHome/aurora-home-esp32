#pragma once

/**
 * @file Config.h
 * @brief Firmware-wide configuration constants.
 *
 * Secret credentials will move to a gitignored file at step 8 (see CLAUDE.md).
 */

// ---- I2C ----
#define AURORA_I2C_SDA 21
#define AURORA_I2C_SCL 22

// ---- Wi-Fi Access Point ----
#define AURORA_WIFI_AP_SSID "ESP32-AP-Broker"
#define AURORA_WIFI_AP_PASSWORD "password123"

// ---- MQTT ----
#define AURORA_MQTT_PORT 1883
#define AURORA_MQTT_TOPIC "sensor/data"
