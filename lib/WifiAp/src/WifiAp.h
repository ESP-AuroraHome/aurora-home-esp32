#pragma once

#include <IPAddress.h>
#include <stddef.h>

/**
 * @brief Wrapper around the ESP32 soft-AP APIs used by AuroraHome.
 */
class WifiAp {
 public:
    /**
     * @brief Build the AP configuration.
     * @param ssid     SSID broadcast by the ESP32.
     * @param password WPA2 password (must be >= 8 chars).
     */
    WifiAp(const char* ssid, const char* password);

    /**
     * @brief Start the soft access point.
     * @return true if the AP started successfully.
     */
    bool begin();

    /**
     * @brief Number of clients currently connected to the AP.
     */
    size_t clientCount() const;

    /**
     * @brief Whether at least one client is currently connected.
     */
    bool hasClient() const { return clientCount() > 0; }

    /**
     * @brief Local IP address assigned to the ESP32 on the AP.
     */
    IPAddress ip() const;

 private:
    const char* ssid_;
    const char* password_;
};
