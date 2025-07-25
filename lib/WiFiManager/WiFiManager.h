/**
 * @file WiFiManager.h
 * @author Jeremy Noverraz
 * @version 1.0
 * @date 2025
 * @brief WiFi manager with automatic reconnection and diagnostics
 */

#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WiFi.h>
#include "../../src/config.h"

/**
 * @class WiFiManager
 * @brief Robust WiFi manager with monitoring and automatic reconnection
 *
 * This class provides a unified interface for:
 * - Connecting to WiFi with error handling
 * - Monitoring signal quality and stability
 * - Managing automatic reconnection
 * - Providing detailed diagnostics
 *
 * @example
 * ```cpp
 * // WiFi connection
 * WiFiManager::begin("MySSID", "MyPassword");
 *
 * // Status check
 * if (WiFiManager::isConnected()) {
 *     LOG_INFO("WiFi connected");
 * }
 *
 * // Loop monitoring
 * if (WiFiManager::isStable()) {
 *     // WiFi stable, continue
 * }
 * ```
 */
class WiFiManager
{
public:
    /**
     * @brief Initializes WiFi connection with automatic retry
     *
     * Attempts to connect to WiFi with provided credentials.
     * Blocks until connection or definitive failure.
     *
     * @param ssid WiFi network SSID
     * @param password WiFi network password
     *
     * @note This method blocks until connection
     * @see config.h for delay and threshold parameters
     */
    static void begin(const char *ssid, const char *password);

    /**
     * @brief Checks if WiFi is connected
     *
     * @return true if connected, false otherwise
     */
    static bool isConnected();

    /**
     * @brief Checks if WiFi connection is stable
     *
     * Checks signal quality and connection stability
     * according to thresholds defined in config.h
     *
     * @return true if stable, false otherwise
     *
     * @see WIFI_QUALITY_THRESHOLD in config.h
     */
    static bool isStable();

    /**
     * @brief Gets WiFi signal quality as percentage
     *
     * @return Signal quality (0-100%)
     */
    static int getSignalQuality();

    /**
     * @brief Gets WiFi signal strength in dBm
     *
     * @return Signal strength in dBm (typically -100 to -30)
     */
    static int getSignalStrength();

    /**
     * @brief Gets detailed WiFi information
     *
     * @return String containing SSID, IP, quality, etc.
     */
    static String getWiFiInfo();

    /**
     * @brief Forces WiFi reconnection
     *
     * Disconnects and reconnects to WiFi. Useful in case of problems.
     *
     * @return true if reconnection succeeds, false otherwise
     */
    static bool reconnect();

    /**
     * @brief Gets local IP address
     *
     * @return IPAddress of ESP32, or IPAddress(0,0,0,0) if not connected
     */
    static IPAddress getLocalIP();

    /**
     * @brief Handles WiFi authentication errors
     *
     * Specialized method to handle AUTH_EXPIRE errors and other
     * common authentication problems
     *
     * @return true if error was resolved, false otherwise
     */
    static bool handleAuthError();

private:
    static unsigned long lastConnectionCheck;
    static bool connectionStable;
    static void monitorConnection();
    static void logConnectionStatus();
};

#endif // WIFI_MANAGER_H
