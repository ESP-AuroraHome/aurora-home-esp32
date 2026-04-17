#include "net.h"

#include <ESPmDNS.h>
#include <IPAddress.h>
#include <PubSubClient.h>
#include <WiFi.h>

#include "Config.h"
#include "Logger.h"

namespace {

constexpr uint8_t kWillQos = 1;
constexpr bool kWillRetain = true;

/// IPs candidates pour le broker si la resolution mDNS echoue.
const IPAddress kBrokerCandidates[] = {
    IPAddress(192, 168, 4, 2),
    IPAddress(192, 168, 4, 3),
    IPAddress(192, 168, 4, 4),
    IPAddress(192, 168, 4, 5),
};
constexpr size_t kBrokerCandidateCount = sizeof(kBrokerCandidates) / sizeof(kBrokerCandidates[0]);

WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

bool connectWithLwt() {
    if (!mqtt.connect(AURORA_MQTT_CLIENT_ID, nullptr, nullptr, AURORA_MQTT_TOPIC_STATUS, kWillQos,
                      kWillRetain, AURORA_MQTT_STATUS_OFFLINE)) {
        return false;
    }
    mqtt.publish(AURORA_MQTT_TOPIC_STATUS, AURORA_MQTT_STATUS_ONLINE, kWillRetain);
    return true;
}

bool connectMdns() {
    const int found = MDNS.queryService(AURORA_MQTT_SERVICE, AURORA_MQTT_PROTO);
    for (int i = 0; i < found; ++i) {
        mqtt.setServer(MDNS.IP(i), MDNS.port(i));
        if (connectWithLwt()) return true;
    }
    return false;
}

bool connectScan() {
    for (size_t i = 0; i < kBrokerCandidateCount; ++i) {
        mqtt.setServer(kBrokerCandidates[i], AURORA_MQTT_PORT);
        if (connectWithLwt()) return true;
    }
    return false;
}

}  // namespace

void netBegin() {
    WiFi.softAP(AURORA_WIFI_AP_SSID, AURORA_WIFI_AP_PASSWORD);
    LOG_INFO("AP SSID: %s", AURORA_WIFI_AP_SSID);
    LOG_INFO("ESP32 IP: %s", WiFi.softAPIP().toString().c_str());

    if (!MDNS.begin(AURORA_MDNS_HOSTNAME)) {
        LOG_WARN("mDNS responder failed to start");
    } else {
        LOG_INFO("mDNS responder up as %s.local", AURORA_MDNS_HOSTNAME);
    }
}

bool netHasClient() {
    return WiFi.softAPgetStationNum() > 0;
}

bool netMqttConnected() {
    return mqtt.connected();
}

void netMqttLoop() {
    mqtt.loop();
}

bool netMqttTryConnect() {
    return connectMdns() || connectScan();
}

bool netMqttPublish(const char* topic, const char* payload) {
    return mqtt.publish(topic, payload);
}
