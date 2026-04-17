#pragma once

#include <IPAddress.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <stddef.h>
#include <stdint.h>

/**
 * @brief MQTT publisher using PubSubClient over Wi-Fi.
 */
class MqttPublisher {
 public:
    /**
     * @brief Build the publisher.
     * @param port  TCP port of the MQTT broker.
     * @param topic Topic used for every publish() call.
     */
    MqttPublisher(uint16_t port, const char* topic);

    /**
     * @brief Try to connect to the broker by scanning a list of candidate IPs.
     * @param candidates Array of IPs to try in order.
     * @param count      Number of entries in @p candidates.
     * @return true when connected to one of the candidates.
     */
    bool connectScanning(const IPAddress* candidates, size_t count);

    /**
     * @brief Pump the MQTT client; must be called regularly from loop().
     */
    void loop();

    /**
     * @brief Whether the MQTT session is currently established.
     */
    bool connected();

    /**
     * @brief Publish a raw payload on the configured topic.
     * @param payload Null-terminated UTF-8 payload.
     * @return true if the broker acknowledged the publish.
     */
    bool publish(const char* payload);

 private:
    WiFiClient   wifiClient_;
    PubSubClient mqtt_;
    uint16_t     port_;
    const char*  topic_;
};
