#include <ArduinoJson.h>
#include <unity.h>

#include <cstring>

#include "telemetry.h"

void setUp() {}
void tearDown() {}

/* JSON produced by telemetryFormat for the reference inputs used in several tests:
 * {"temperature":"21.50","humidity":"48.00","pressure":"1013.20","co2":"650.00","light":"123.40"}
 * = 95 bytes (without null terminator)
 */
static constexpr float kTemp = 21.5f;
static constexpr float kHum = 48.0f;
static constexpr float kPressure = 1013.2f;
static constexpr float kCo2 = 650.0f;
static constexpr float kLux = 123.4f;

void testTelemetryFormatProducesValidJson() {
    char payload[256];
    const size_t written =
        telemetryFormat(payload, sizeof(payload), kTemp, kHum, kPressure, kCo2, kLux);

    TEST_ASSERT_GREATER_THAN_UINT32(0, static_cast<uint32_t>(written));
    TEST_ASSERT_LESS_THAN_UINT32(sizeof(payload), static_cast<uint32_t>(written));

    JsonDocument doc;
    TEST_ASSERT_TRUE(deserializeJson(doc, payload) == DeserializationError::Ok);
    TEST_ASSERT_EQUAL_size_t(5, doc.size());
}

void testTelemetryFormatKeyNames() {
    char payload[256];
    telemetryFormat(payload, sizeof(payload), kTemp, kHum, kPressure, kCo2, kLux);

    JsonDocument doc;
    deserializeJson(doc, payload);

    TEST_ASSERT_TRUE(doc["temperature"].is<const char*>());
    TEST_ASSERT_TRUE(doc["humidity"].is<const char*>());
    TEST_ASSERT_TRUE(doc["pressure"].is<const char*>());
    TEST_ASSERT_TRUE(doc["co2"].is<const char*>());
    TEST_ASSERT_TRUE(doc["light"].is<const char*>());
}

void testTelemetryFormatStringValues() {
    char payload[256];
    telemetryFormat(payload, sizeof(payload), kTemp, kHum, kPressure, kCo2, kLux);

    JsonDocument doc;
    deserializeJson(doc, payload);

    TEST_ASSERT_EQUAL_STRING("21.50", doc["temperature"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("48.00", doc["humidity"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("1013.20", doc["pressure"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("650.00", doc["co2"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("123.40", doc["light"].as<const char*>());
}

void testTelemetryFormatNegativeTemperature() {
    char payload[256];
    const size_t written =
        telemetryFormat(payload, sizeof(payload), -5.3f, kHum, kPressure, kCo2, kLux);

    TEST_ASSERT_GREATER_THAN_UINT32(0, static_cast<uint32_t>(written));
    JsonDocument doc;
    TEST_ASSERT_TRUE(deserializeJson(doc, payload) == DeserializationError::Ok);
    TEST_ASSERT_EQUAL_STRING("-5.30", doc["temperature"].as<const char*>());
}

void testTelemetryFormatZeroValues() {
    char payload[256];
    const size_t written = telemetryFormat(payload, sizeof(payload), 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);

    TEST_ASSERT_GREATER_THAN_UINT32(0, static_cast<uint32_t>(written));
    JsonDocument doc;
    TEST_ASSERT_TRUE(deserializeJson(doc, payload) == DeserializationError::Ok);
    TEST_ASSERT_EQUAL_STRING("0.00", doc["temperature"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("0.00", doc["co2"].as<const char*>());
}

void testTelemetryFormatReturnsZeroWhenBufferTooSmall() {
    char payload[8];
    const size_t written =
        telemetryFormat(payload, sizeof(payload), kTemp, kHum, kPressure, kCo2, kLux);
    TEST_ASSERT_EQUAL_size_t(0, written);
}

void testTelemetryFormatReturnsZeroForNullBuffer() {
    const size_t written = telemetryFormat(nullptr, 256, kTemp, kHum, kPressure, kCo2, kLux);
    TEST_ASSERT_EQUAL_size_t(0, written);
}

void testTelemetryFormatReturnsZeroForZeroSize() {
    char payload[256];
    const size_t written = telemetryFormat(payload, 0, kTemp, kHum, kPressure, kCo2, kLux);
    TEST_ASSERT_EQUAL_size_t(0, written);
}

void testTelemetryFormatExactBoundaryBuffer() {
    /* Reference JSON is 95 bytes; a 96-byte buffer (95 + '\0') must succeed. */
    char payload[96];
    const size_t written =
        telemetryFormat(payload, sizeof(payload), kTemp, kHum, kPressure, kCo2, kLux);
    TEST_ASSERT_EQUAL_size_t(95, written);
}

void testTelemetryFormatOneByteTooSmall() {
    /* 95-byte buffer cannot hold the 95-byte JSON plus the null terminator. */
    char payload[95];
    const size_t written =
        telemetryFormat(payload, sizeof(payload), kTemp, kHum, kPressure, kCo2, kLux);
    TEST_ASSERT_EQUAL_size_t(0, written);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(testTelemetryFormatProducesValidJson);
    RUN_TEST(testTelemetryFormatKeyNames);
    RUN_TEST(testTelemetryFormatStringValues);
    RUN_TEST(testTelemetryFormatNegativeTemperature);
    RUN_TEST(testTelemetryFormatZeroValues);
    RUN_TEST(testTelemetryFormatReturnsZeroWhenBufferTooSmall);
    RUN_TEST(testTelemetryFormatReturnsZeroForNullBuffer);
    RUN_TEST(testTelemetryFormatReturnsZeroForZeroSize);
    RUN_TEST(testTelemetryFormatExactBoundaryBuffer);
    RUN_TEST(testTelemetryFormatOneByteTooSmall);
    return UNITY_END();
}
