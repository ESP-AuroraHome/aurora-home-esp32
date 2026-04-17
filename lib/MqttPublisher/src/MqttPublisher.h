#pragma once

#include <IPAddress.h>
#include <PubSubClient.h>
#include <WiFi.h>

#include <cstddef>
#include <cstdint>

/**
 * @brief MQTT publisher using PubSubClient over Wi-Fi.
 *
 * Supporte:
 *  - Decouverte broker par liste d'IP candidates (fallback).
 *  - Decouverte mDNS via MDNS.queryService().
 *  - Last Will and Testament retained sur un topic de statut dedie,
 *    avec publication "online" retained a chaque (re)connexion.
 */
class MqttPublisher {
 public:
    /**
     * @brief Construit le publisher.
     * @param port             Port TCP du broker MQTT.
     * @param topic            Topic pour chaque publish().
     * @param clientId         Identifiant client MQTT (unique par device).
     * @param statusTopic      Topic retained pour LWT + heartbeat connexion.
     * @param onlinePayload    Payload publie retained en (re)connexion.
     * @param offlinePayload   Payload LWT publie par le broker a la deconnexion.
     */
    MqttPublisher(uint16_t port, const char* topic, const char* clientId, const char* statusTopic,
                  const char* onlinePayload, const char* offlinePayload);

    /**
     * @brief Tente la connexion en scannant une liste d'IPs candidates.
     * @param candidates Tableau d'IPs a essayer dans l'ordre.
     * @param count      Nombre d'entrees dans @p candidates.
     * @return true si connecte a l'un des candidats.
     */
    bool connectScanning(const IPAddress* candidates, size_t count);

    /**
     * @brief Tente la connexion via decouverte mDNS (_service._proto.local).
     * @param service "mqtt" par defaut.
     * @param proto   "tcp" par defaut.
     * @return true si le broker est resolu et la connexion reussit.
     */
    bool connectMdns(const char* service, const char* proto);

    /** @brief Pompe le client MQTT; a appeler regulierement depuis loop(). */
    void loop();

    /** @brief Session MQTT etablie ? */
    bool connected();

    /**
     * @brief Publie un payload brut sur le topic configure.
     * @param payload Payload UTF-8 null-termine.
     * @return true si le broker a acquitte le publish.
     */
    bool publish(const char* payload);

 private:
    bool connectWithLwt();

    WiFiClient wifiClient_;
    PubSubClient mqtt_;
    uint16_t port_;
    const char* topic_;
    const char* clientId_;
    const char* statusTopic_;
    const char* onlinePayload_;
    const char* offlinePayload_;
};
