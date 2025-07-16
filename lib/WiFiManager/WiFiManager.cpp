/**
 * @file WiFiManager.cpp
 * @author Jeremy Noverraz
 * @version 1.0
 * @date 2025
 * @brief Implementation of robust WiFi manager
 */

#include "WiFiManager.h"
#include "../../src/config.h"
#include "../Utils/Logger.h"
#include "../Utils/Helpers.h"

// Static variables for monitoring
unsigned long WiFiManager::lastConnectionCheck = 0;
bool WiFiManager::connectionStable = false;

void WiFiManager::begin(const char *ssid, const char *password)
{
    LOG_INFO("Initializing WiFi connection...");
    LOG_INFOF("SSID: %s", ssid);

    // WiFi mode configuration with optimized parameters
    WiFi.mode(WIFI_STA);

    // Advanced WiFi configuration to avoid AUTH_EXPIRE
    WiFi.setSleep(false);        // Disable WiFi sleep mode
    WiFi.setAutoReconnect(true); // Enable automatic reconnection

    // WiFi power configuration to avoid disconnections
    WiFi.setTxPower(WIFI_POWER_19_5dBm); // Maximum power

    // Completely clean previous WiFi parameters
    WiFi.disconnect(true, true);
    delay(WIFI_CLEANUP_DELAY);

    // Connection attempt with retry and improved error handling
    int attempts = 0;
    const int maxAttempts = WIFI_MAX_ATTEMPTS;
    bool connectionSuccess = false;

    LOG_INFO("Connecting...");

    while (!connectionSuccess && attempts < maxAttempts)
    {
        // Clean before each attempt if not the first
        if (attempts > 0)
        {
            WiFi.disconnect(true, true);
            delay(WIFI_CLEANUP_DELAY);
        }

        WiFi.begin(ssid, password);

        // Wait for connection with optimized timeout
        int waitAttempts = 0;
        const int waitMaxAttempts = 24; // 6 seconds max per attempt (250ms * 24)

        while (WiFi.status() != WL_CONNECTED && waitAttempts < waitMaxAttempts)
        {
            delay(WIFI_DELAY_MS);
            waitAttempts++;

            // Check specific errors with improved handling
            if (WiFi.status() == WL_CONNECT_FAILED || WiFi.status() == WL_NO_SSID_AVAIL)
            {
                LOG_WARNF("WiFi connection error (attempt %d): %d", attempts + 1, WiFi.status());
                break; // Exit wait loop
            }

            // Specific authentication error handling
            if (WIFI_HANDLE_AUTH_ERRORS && WiFi.status() == WL_CONNECT_FAILED)
            {
                LOG_WARN("Authentication error detected - attempting resolution...");
                handleAuthError();
                break; // Exit and restart
            }
        }

        if (WiFi.status() == WL_CONNECTED)
        {
            connectionSuccess = true;
            break;
        }
        else
        {
            attempts++;
            LOG_WARNF("Connection attempt %d/%d failed (status: %d)", attempts, maxAttempts, WiFi.status());

            // Progressive delay between attempts with exponential backoff
            int delayTime = 500 + (attempts * 200);
            if (delayTime > 3000)
                delayTime = 3000; // Max 3 seconds
            delay(delayTime);
        }
    }

    if (connectionSuccess)
    {
        connectionStable = true;
        lastConnectionCheck = millis();

        LOG_INFO("WiFi connected successfully!");
        logConnectionStatus();

        // Wait longer to stabilize connection
        delay(WIFI_STABILIZATION_DELAY);

        // Final stability verification
        if (WiFi.status() == WL_CONNECTED)
        {
            LOG_INFO("WiFi connection stabilized and ready");
        }
        else
        {
            LOG_WARN("WiFi connection unstable after stabilization");
        }
    }
    else
    {
        LOG_ERROR("WiFi connection failed after all attempts");
        LOG_ERRORF("Last status: %d", WiFi.status());
    }
}

bool WiFiManager::isConnected()
{
    bool connected = (WiFi.status() == WL_CONNECTED);

    // Connection monitoring
    if (connected != connectionStable)
    {
        if (connected)
        {
            LOG_INFO("WiFi reconnected");
            connectionStable = true;
        }
        else
        {
            LOG_WARN("WiFi disconnected");
            connectionStable = false;
        }
        lastConnectionCheck = millis();
    }

    return connected;
}

bool WiFiManager::isStable()
{
    if (!isConnected())
    {
        return false;
    }

    int quality = getSignalQuality();
    bool stable = (quality >= WIFI_QUALITY_THRESHOLD);

    // Stability monitoring
    static unsigned long lastStabilityCheck = 0;
    if (millis() - lastStabilityCheck > 30000) // Check every 30 seconds
    {
        if (stable != connectionStable)
        {
            if (stable)
            {
                LOG_INFO("WiFi connection stabilized");
            }
            else
            {
                LOG_WARN("WiFi connection unstable");
            }
            connectionStable = stable;
        }
        lastStabilityCheck = millis();
    }

    return stable;
}

int WiFiManager::getSignalQuality()
{
    if (!isConnected())
    {
        return 0;
    }

    int rssi = WiFi.RSSI();
    int quality = WIFI_QUALITY_MULTIPLIER * (rssi + WIFI_RSSI_OFFSET);

    // Limit between 0 and 100
    quality = Helpers::clamp(quality, 0, 100);

    return quality;
}

int WiFiManager::getSignalStrength()
{
    if (!isConnected())
    {
        return -100;
    }

    return WiFi.RSSI();
}

String WiFiManager::getWiFiInfo()
{
    if (!isConnected())
    {
        return "WiFi not connected";
    }

    String info = "WiFi Info:\n";
    info += "SSID: " + WiFi.SSID() + "\n";
    info += "IP: " + WiFi.localIP().toString() + "\n";
    info += "Gateway: " + WiFi.gatewayIP().toString() + "\n";
    info += "DNS: " + WiFi.dnsIP().toString() + "\n";
    info += "Signal: " + String(getSignalStrength()) + " dBm\n";
    info += "Quality: " + String(getSignalQuality()) + "%\n";
    info += "Stable: " + String(isStable() ? "Yes" : "No") + "\n";

    return info;
}

bool WiFiManager::reconnect()
{
    LOG_INFO("Forced WiFi reconnection...");

    // Completely clean connection
    WiFi.disconnect(true, true);
    delay(500);

    // Optimized configuration for reconnection
    WiFi.setSleep(false);
    WiFi.setAutoReconnect(true);

    int attempts = 0;
    const int maxAttempts = 5;
    bool reconnectionSuccess = false;

    while (!reconnectionSuccess && attempts < maxAttempts)
    {
        WiFi.begin();

        // Wait for reconnection with timeout
        int waitAttempts = 0;
        const int waitMaxAttempts = 15; // 7.5 seconds max per attempt

        while (WiFi.status() != WL_CONNECTED && waitAttempts < waitMaxAttempts)
        {
            delay(WIFI_DELAY_MS);
            waitAttempts++;

            // Check specific errors
            if (WiFi.status() == WL_CONNECT_FAILED || WiFi.status() == WL_NO_SSID_AVAIL)
            {
                LOG_WARNF("WiFi reconnection error (attempt %d): %d", attempts + 1, WiFi.status());
                break;
            }
        }

        if (WiFi.status() == WL_CONNECTED)
        {
            reconnectionSuccess = true;
            break;
        }
        else
        {
            attempts++;
            LOG_WARNF("Reconnection attempt %d/%d failed", attempts, maxAttempts);

            // Progressive delay between attempts
            delay(1000 + (attempts * 200));

            // Clean and restart
            WiFi.disconnect(true, true);
            delay(300);
        }
    }

    if (reconnectionSuccess)
    {
        LOG_INFO("WiFi reconnection successful");
        connectionStable = true;
        lastConnectionCheck = millis();

        // Wait a bit to stabilize
        delay(300);
        return true;
    }
    else
    {
        LOG_ERROR("WiFi reconnection failed after all attempts");
        return false;
    }
}

IPAddress WiFiManager::getLocalIP()
{
    if (isConnected())
    {
        return WiFi.localIP();
    }
    return IPAddress(0, 0, 0, 0);
}

void WiFiManager::logConnectionStatus()
{
    LOG_INFOF("SSID: %s", WiFi.SSID().c_str());
    LOG_INFOF("IP: %s", WiFi.localIP().toString().c_str());
    LOG_INFOF("Signal: %d dBm (%d%%)", getSignalStrength(), getSignalQuality());
    LOG_INFOF("Stable: %s", isStable() ? "Yes" : "No");
}

bool WiFiManager::handleAuthError()
{
    LOG_INFO("Handling WiFi authentication error...");

    // Completely clean WiFi parameters
    WiFi.disconnect(true, true);
    delay(1000);

    // Reset WiFi parameters with optimized configuration
    WiFi.setSleep(false);
    WiFi.setAutoReconnect(true);
    WiFi.setTxPower(WIFI_POWER_19_5dBm);

    // Wait longer before restarting
    delay(3000);

    LOG_INFO("WiFi parameters reset to resolve authentication error");
    return true;
}
