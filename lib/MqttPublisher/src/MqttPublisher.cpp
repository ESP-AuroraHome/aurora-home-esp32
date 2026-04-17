#include "MqttPublisher.h"

#include <Arduino.h>

MqttPublisher::MqttPublisher(uint16_t port, const char* topic)
    : mqtt_(wifiClient_), port_(port), topic_(topic) {}

bool MqttPublisher::connectScanning(const IPAddress* candidates, size_t count) {
    for (size_t i = 0; i < count; ++i) {
        mqtt_.setServer(candidates[i], port_);
        String clientId = "ESP32Client-" + String(random(0xffff), HEX);
        if (mqtt_.connect(clientId.c_str())) return true;
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
