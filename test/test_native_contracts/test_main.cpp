#include <unity.h>

#include <type_traits>

#include "net.h"
#include "sensors.h"

void setUp() {}
void tearDown() {}

void testNetApiFunctionSignatures() {
    TEST_ASSERT_TRUE((std::is_same_v<decltype(&netBegin), void (*)()>));
    TEST_ASSERT_TRUE((std::is_same_v<decltype(&netHasClient), bool (*)()>));
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

int main() {
    UNITY_BEGIN();
    RUN_TEST(testNetApiFunctionSignatures);
    RUN_TEST(testSensorsApiFunctionSignatures);
    return UNITY_END();
}
