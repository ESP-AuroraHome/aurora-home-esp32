#include <unity.h>

#include <cstddef>
#include <type_traits>

#include "fusion.h"
#include "net.h"
#include "sensors.h"
#include "telemetry.h"

void setUp() {}
void tearDown() {}

void testNetApiFunctionSignatures() {
    TEST_ASSERT_TRUE((std::is_same_v<decltype(&netBegin), void (*)()>));
    TEST_ASSERT_TRUE((std::is_same_v<decltype(&netHasClient), bool (*)()>));
    TEST_ASSERT_TRUE((std::is_same_v<decltype(&netClientIP), IPAddress (*)()>));
    TEST_ASSERT_TRUE((std::is_same_v<decltype(&netMqttConnected), bool (*)()>));
    TEST_ASSERT_TRUE((std::is_same_v<decltype(&netMqttLoop), void (*)()>));
    TEST_ASSERT_TRUE((std::is_same_v<decltype(&netMqttTryConnect), bool (*)()>));
    TEST_ASSERT_TRUE(
        (std::is_same_v<decltype(&netMqttPublish), bool (*)(const char*, const char*)>));
}

void testSensorsApiFunctionSignatures() {
    TEST_ASSERT_TRUE((std::is_same_v<decltype(&sensorsInitBh1750), bool (*)()>));
    TEST_ASSERT_TRUE((std::is_same_v<decltype(&sensorsInitBme280), bool (*)()>));
    TEST_ASSERT_TRUE((std::is_same_v<decltype(&sensorsInitScd30), bool (*)()>));
    TEST_ASSERT_TRUE(
        (std::is_same_v<decltype(&sensorsReadScd30), bool (*)(float&, float&, float&)>));
    TEST_ASSERT_TRUE(
        (std::is_same_v<decltype(&sensorsReadBme280), bool (*)(float&, float&, float&)>));
    TEST_ASSERT_TRUE((std::is_same_v<decltype(&sensorsReadBh1750), bool (*)(float&)>));
}

void testTelemetryApiFunctionSignatures() {
    TEST_ASSERT_TRUE(
        (std::is_same_v<decltype(&telemetryFormat),
                        size_t (*)(char*, size_t, float, float, float, float, float)>));
}

void testFusionApiFunctionSignatures() {
    TEST_ASSERT_TRUE((std::is_same_v<decltype(&fusionAverage), float (*)(float, float)>));
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(testNetApiFunctionSignatures);
    RUN_TEST(testSensorsApiFunctionSignatures);
    RUN_TEST(testTelemetryApiFunctionSignatures);
    RUN_TEST(testFusionApiFunctionSignatures);
    return UNITY_END();
}
