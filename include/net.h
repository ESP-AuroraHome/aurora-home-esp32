#pragma once

/**
 * @file net.h
 * @brief Soft-AP Wi-Fi + MQTT (decouverte mDNS, fallback scan IP, LWT).
 *
 * Les clients PubSubClient/WiFiClient sont internes a net.cpp. On expose
 * uniquement des fonctions libres consommees par main.cpp.
 */

/** @brief Demarre le soft-AP et le responder mDNS. */
void netBegin();

/** @brief Vrai si au moins un client Wi-Fi est associe a l'AP. */
bool netHasClient();

/** @brief Session MQTT etablie ? */
bool netMqttConnected();

/** @brief Pompe MQTT; a appeler regulierement depuis loop(). */
void netMqttLoop();

/**
 * @brief Tente une (re)connexion : mDNS d'abord, sinon scan IPs candidates.
 *        Arme un LWT retained et publie "online" retained sur succes.
 */
bool netMqttTryConnect();

/** @brief Publie un payload sur le topic donne. */
bool netMqttPublish(const char* topic, const char* payload);
