#include <unity.h>

/* Pull stub state globals before net.h so headers resolve correctly. */
#include "Arduino.h"
#include "ESPmDNS.h"
#include "PubSubClient.h"
#include "WiFi.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "net.h"

void setUp() {
    g_wifi_station_num = 0;
    g_mqtt_connected = false;
    g_mqtt_connect_calls = 0;
    g_mqtt_connect_fail_n = 0;
    g_mqtt_connect_result = true;
    g_mqtt_publish_calls = 0;
    g_mqtt_set_server_calls = 0;
    g_mdns_service_count = 0;
    g_esp_wifi_ok = true;
    g_esp_netif_ok = true;
    g_esp_netif_num = 0;
    g_esp_netif_ip = 0;
}
void tearDown() {}

/* ---- netHasClient ---- */

void testHasClientFalseWhenNoStation() {
    g_wifi_station_num = 0;
    TEST_ASSERT_FALSE(netHasClient());
}

void testHasClientTrueWithOneStation() {
    g_wifi_station_num = 1;
    TEST_ASSERT_TRUE(netHasClient());
}

/* ---- netMqttTryConnect: mDNS path ---- */

void testTryConnectSucceedsViaMdns() {
    g_mdns_service_count = 1;
    TEST_ASSERT_TRUE(netMqttTryConnect());
    /* connectMdns calls setServer + connect once, scan path not reached */
    TEST_ASSERT_EQUAL_INT(1, g_mqtt_connect_calls);
    TEST_ASSERT_EQUAL_INT(1, g_mqtt_set_server_calls);
}

void testTryConnectMdnsEmptyFallsBackToScan() {
    g_mdns_service_count = 0; /* mDNS finds nothing */
    TEST_ASSERT_TRUE(netMqttTryConnect());
    /* connect() must have been called at least once via the scan path */
    TEST_ASSERT_GREATER_THAN_INT(0, g_mqtt_connect_calls);
}

void testTryConnectMdnsFailsFallsBackToScan() {
    g_mdns_service_count = 1;
    g_mqtt_connect_fail_n = 1; /* mDNS call fails, scan first call succeeds */
    TEST_ASSERT_TRUE(netMqttTryConnect());
    TEST_ASSERT_EQUAL_INT(2, g_mqtt_connect_calls); /* 1 mDNS + 1 scan */
}

/* ---- netMqttTryConnect: scan path ---- */

void testTryConnectScanSucceedsOnFirstCandidate() {
    /* mDNS empty, first scan candidate succeeds */
    TEST_ASSERT_TRUE(netMqttTryConnect());
    TEST_ASSERT_EQUAL_INT(1, g_mqtt_connect_calls);
    TEST_ASSERT_EQUAL_INT(1, g_mqtt_set_server_calls);
}

void testTryConnectScanSucceedsOnSecondCandidate() {
    g_mqtt_connect_fail_n = 1; /* fail first, succeed second */
    TEST_ASSERT_TRUE(netMqttTryConnect());
    TEST_ASSERT_EQUAL_INT(2, g_mqtt_connect_calls);
    TEST_ASSERT_EQUAL_INT(2, g_mqtt_set_server_calls);
}

void testTryConnectReturnsFalseWhenAllFail() {
    g_mqtt_connect_result = false; /* all calls fail */
    TEST_ASSERT_FALSE(netMqttTryConnect());
    /* 4 scan candidates + 0 mDNS */
    TEST_ASSERT_EQUAL_INT(4, g_mqtt_connect_calls);
}

/* ---- LWT: "online" published on successful connect ---- */

void testConnectPublishesOnlineStatus() {
    TEST_ASSERT_TRUE(netMqttTryConnect());
    /* connectWithLwt() calls mqtt.publish(STATUS_TOPIC, "online", retain) once */
    TEST_ASSERT_EQUAL_INT(1, g_mqtt_publish_calls);
}

void testConnectDoesNotPublishOnFailure() {
    g_mqtt_connect_result = false;
    netMqttTryConnect();
    TEST_ASSERT_EQUAL_INT(0, g_mqtt_publish_calls);
}

/* ---- netMqttConnected / netMqttPublish ---- */

void testMqttConnectedReflectsStubState() {
    g_mqtt_connected = false;
    TEST_ASSERT_FALSE(netMqttConnected());
    g_mqtt_connected = true;
    TEST_ASSERT_TRUE(netMqttConnected());
}

void testMqttPublishDelegates() {
    netMqttPublish("topic", "payload");
    TEST_ASSERT_EQUAL_INT(1, g_mqtt_publish_calls);
}

/* ---- netClientIP ---- */

void testClientIpReturnsZeroWhenWifiFails() {
    g_esp_wifi_ok = false;
    TEST_ASSERT_TRUE(netClientIP() == IPAddress());
}

void testClientIpReturnsZeroWhenNetifFails() {
    g_esp_netif_ok = false;
    TEST_ASSERT_TRUE(netClientIP() == IPAddress());
}

void testClientIpReturnsZeroWhenNoStation() {
    g_esp_netif_num = 0;
    TEST_ASSERT_TRUE(netClientIP() == IPAddress());
}

void testClientIpReturnsFirstStation() {
    g_esp_netif_num = 1;
    g_esp_netif_ip = (10U) | (4U << 8) | (168U << 16) | (192U << 24);
    TEST_ASSERT_TRUE(netClientIP() != IPAddress());
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(testHasClientFalseWhenNoStation);
    RUN_TEST(testHasClientTrueWithOneStation);
    RUN_TEST(testTryConnectSucceedsViaMdns);
    RUN_TEST(testTryConnectMdnsEmptyFallsBackToScan);
    RUN_TEST(testTryConnectMdnsFailsFallsBackToScan);
    RUN_TEST(testTryConnectScanSucceedsOnFirstCandidate);
    RUN_TEST(testTryConnectScanSucceedsOnSecondCandidate);
    RUN_TEST(testTryConnectReturnsFalseWhenAllFail);
    RUN_TEST(testConnectPublishesOnlineStatus);
    RUN_TEST(testConnectDoesNotPublishOnFailure);
    RUN_TEST(testMqttConnectedReflectsStubState);
    RUN_TEST(testMqttPublishDelegates);
    RUN_TEST(testClientIpReturnsZeroWhenWifiFails);
    RUN_TEST(testClientIpReturnsZeroWhenNetifFails);
    RUN_TEST(testClientIpReturnsZeroWhenNoStation);
    RUN_TEST(testClientIpReturnsFirstStation);
    return UNITY_END();
}
