#include <ArduinoJson.h>
#include <unity.h>

#include "telemetry.h"

void setUp() {}
void tearDown() {}

void testTelemetryFormatProducesValidJsonWithExpectedKeys() {
    char payload[256];
    const size_t written =
        telemetryFormat(payload, sizeof(payload), 21.5f, 48.0f, 1013.2f, 650.0f, 123.4f);

    TEST_ASSERT_GREATER_THAN_UINT32(0, static_cast<uint32_t>(written));
    TEST_ASSERT_LESS_THAN_UINT32(sizeof(payload), static_cast<uint32_t>(written));

    JsonDocument doc;
    const DeserializationError err = deserializeJson(doc, payload);
    TEST_ASSERT_TRUE(err == DeserializationError::Ok);

    TEST_ASSERT_TRUE(doc["temperature_c"].is<float>());
    TEST_ASSERT_TRUE(doc["humidity_pct"].is<float>());
    TEST_ASSERT_TRUE(doc["pressure_hpa"].is<float>());
    TEST_ASSERT_TRUE(doc["co2_ppm"].is<float>());
    TEST_ASSERT_TRUE(doc["light_lx"].is<float>());
    TEST_ASSERT_EQUAL_size_t(5, doc.size());
}

void testTelemetryFormatReturnsZeroWhenBufferTooSmall() {
    char payload[8];
    const size_t written =
        telemetryFormat(payload, sizeof(payload), 21.5f, 48.0f, 1013.2f, 650.0f, 123.4f);
    TEST_ASSERT_EQUAL_size_t(0, written);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(testTelemetryFormatProducesValidJsonWithExpectedKeys);
    RUN_TEST(testTelemetryFormatReturnsZeroWhenBufferTooSmall);
    return UNITY_END();
}
