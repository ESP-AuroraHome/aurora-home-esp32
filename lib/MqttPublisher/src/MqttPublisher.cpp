#include "MqttPublisher.h"

#include <Arduino.h>
#include <ESPmDNS.h>

namespace {
constexpr uint8_t kWillQos = 1;
constexpr bool kWillRetain = true;
}  // namespace

MqttPublisher::MqttPublisher(uint16_t port, const char* topic, const char* clientId,
                             const char* statusTopic, const char* onlinePayload,
                             const char* offlinePayload)
    : mqtt_(wifiClient_),
      port_(port),
      topic_(topic),
      clientId_(clientId),
      statusTopic_(statusTopic),
      onlinePayload_(onlinePayload),
      offlinePayload_(offlinePayload) {}

bool MqttPublisher::connectWithLwt() {
    if (!mqtt_.connect(clientId_, nullptr, nullptr, statusTopic_, kWillQos, kWillRetain,
                       offlinePayload_)) {
        return false;
    }
    mqtt_.publish(statusTopic_, onlinePayload_, kWillRetain);
    return true;
}

bool MqttPublisher::connectScanning(const IPAddress* candidates, size_t count) {
    for (size_t i = 0; i < count; ++i) {
        mqtt_.setServer(candidates[i], port_);
        if (connectWithLwt()) return true;
    }
    return false;
}

bool MqttPublisher::connectMdns(const char* service, const char* proto) {
    const int found = MDNS.queryService(service, proto);
    for (int i = 0; i < found; ++i) {
        const IPAddress ip = MDNS.IP(i);
        const uint16_t port = MDNS.port(i);
        mqtt_.setServer(ip, port);
        if (connectWithLwt()) return true;
    }
    return false;
}

void MqttPublisher::loop() {
    mqtt_.loop();
}

bool MqttPublisher::connected() {
    return mqtt_.connected();
}

bool MqttPublisher::publish(const char* payload) {
    return mqtt_.publish(topic_, payload);
}
