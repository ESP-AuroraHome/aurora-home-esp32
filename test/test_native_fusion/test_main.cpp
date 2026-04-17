#include <unity.h>

#include <cmath>

#include "fusion.h"

/**
 * @file test/test_native_fusion/test_main.cpp
 * @brief Tests natifs (Unity) de la logique pure fusion.h.
 *
 * Tourne sous l'env PlatformIO `native` : aucune dependance Arduino.
 */

void setUp() {}
void tearDown() {}

void testAverageSimple() {
    TEST_ASSERT_EQUAL_FLOAT(21.0f, fusionAverage(20.0f, 22.0f));
}

void testAverageEqualInputs() {
    TEST_ASSERT_EQUAL_FLOAT(25.5f, fusionAverage(25.5f, 25.5f));
}

void testAverageNegatives() {
    TEST_ASSERT_EQUAL_FLOAT(-10.0f, fusionAverage(-15.0f, -5.0f));
}

void testAverageZero() {
    TEST_ASSERT_EQUAL_FLOAT(0.0f, fusionAverage(0.0f, 0.0f));
}

void testAverageCommutative() {
    TEST_ASSERT_EQUAL_FLOAT(fusionAverage(3.0f, 7.0f), fusionAverage(7.0f, 3.0f));
}

void testAverageMixedSigns() {
    TEST_ASSERT_EQUAL_FLOAT(0.0f, fusionAverage(-42.5f, 42.5f));
}

void testAverageFractionalValues() {
    TEST_ASSERT_FLOAT_WITHIN(1e-6f, 2.8f, fusionAverage(1.2f, 4.4f));
}

void testAverageLargeValues() {
    TEST_ASSERT_FLOAT_WITHIN(1.0f, 1000000.0f, fusionAverage(999999.0f, 1000001.0f));
}

void testAverageWithPositiveInfinity() {
    const float result = fusionAverage(INFINITY, 10.0f);
    TEST_ASSERT_TRUE(std::isinf(result));
    TEST_ASSERT_TRUE(result > 0.0f);
}

void testAverageWithNaN() {
    const float result = fusionAverage(NAN, 10.0f);
    TEST_ASSERT_TRUE(std::isnan(result));
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(testAverageSimple);
    RUN_TEST(testAverageEqualInputs);
    RUN_TEST(testAverageNegatives);
    RUN_TEST(testAverageZero);
    RUN_TEST(testAverageCommutative);
    RUN_TEST(testAverageMixedSigns);
    RUN_TEST(testAverageFractionalValues);
    RUN_TEST(testAverageLargeValues);
    RUN_TEST(testAverageWithPositiveInfinity);
    RUN_TEST(testAverageWithNaN);
    return UNITY_END();
}
