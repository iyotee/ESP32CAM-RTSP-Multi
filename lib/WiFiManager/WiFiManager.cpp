/**
 * @file WiFiManager.cpp
 * @author Jeremy Noverraz
 * @version 1.0
 * @date 2025
 * @brief Implementation of robust WiFi manager
 *
 * @modifications
 * 2025-08-25	JCZD	explicit BSSID selection stuff
 *
 * @TODO
 * - clean up redundancies when connection is established/lost
 */

#include "WiFiManager.h"
#include "../../src/config.h"
#include "../Utils/Logger.h"
#include "../Utils/Helpers.h"

// Static variables for monitoring
unsigned long WiFiManager::lastConnectionCheck = 0;
bool WiFiManager::connectionStable = false;

void WiFiManager::begin(const char *ssid, const char *password
#ifdef WIFI_USE_BSSID
    , const uint8_t channel, const uint8_t bssid[6]
#endif
)
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

    // Configure static IP if enabled
#ifdef WIFI_USE_STATIC_IP
    LOG_INFO("Configuring static IP...");
#ifndef WIFI_USE_BSSID
    LOG_INFOF("Static IP: %s", WIFI_STATIC_IP);
    LOG_INFOF("Gateway: %s", WIFI_STATIC_GATEWAY);
    LOG_INFOF("Subnet: %s", WIFI_STATIC_SUBNET);
    LOG_INFOF("DNS: %s", WIFI_STATIC_DNS);

    IPAddress staticIP, gateway, subnet, dns;

    if (staticIP.fromString(WIFI_STATIC_IP) &&
        gateway.fromString(WIFI_STATIC_GATEWAY) &&
        subnet.fromString(WIFI_STATIC_SUBNET) &&
        dns.fromString(WIFI_STATIC_DNS))
    {
        WiFi.config(staticIP, gateway, subnet, dns);
        LOG_DEBUG("Static IP configuration applied successfully");
    }
    else
    {
        LOG_ERROR("Invalid static IP configuration - falling back to DHCP");
    }
#else

#ifdef HAS_SECONDARY_DNS
  LOG_DEBUG("Both DNS servers are set");
  if ( !WiFi.config(staticIP, gateway, subnet, primaryDNS, secondaryDNS) ) {
#else
#ifdef HAS_PRIMARY_DNS
  LOG_DEBUG("Only primary DNS server is set");
  if ( !WiFi.config(staticIP, gateway, subnet, primaryDNS) ) {
#else
  LOG_DEBUG("No DNS servers are set!");
  if ( !WiFi.config(staticIP, gateway, subnet) ) {
#endif
#endif
        LOG_ERROR("Failed to configure static IP - falling back to DHCP");
  } else {
        LOG_INFO("Static IP configuration applied successfully");
  }
#endif

#else
    LOG_INFO("Using DHCP for automatic IP assignment");
#endif

    // Completely clean previous WiFi parameters
    WiFi.disconnect(true, true);
    delay(WIFI_CLEANUP_DELAY);

    // Optimized connection attempt with faster error handling
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

#ifndef WIFI_USE_BSSID
        WiFi.begin(ssid, password);
#else
        WiFi.begin(ssid, password, channel, bssid);
#endif

        // Balanced connection wait with stable timeout
        int waitAttempts = 0;
        const int waitMaxAttempts = 20; // 4 seconds max per attempt (200ms * 20)

        while (WiFi.status() != WL_CONNECTED && waitAttempts < waitMaxAttempts)
        {
            delay(WIFI_DELAY_MS);
            waitAttempts++;

            // Stable error detection
            if (WiFi.status() == WL_CONNECT_FAILED || WiFi.status() == WL_NO_SSID_AVAIL)
            {
                LOG_VERBOSEF("WiFi error detected (attempt %d): %d", attempts + 1, WiFi.status());
                break; // Exit wait loop
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

            // Balanced delay between attempts
            delay(300 + (attempts * 150)); // Max 1.65 seconds
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
    info += "BSSID: " + WiFi.BSSIDstr() + "\n";
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

    // Configure static IP if enabled (same as in begin method)
#ifdef WIFI_USE_STATIC_IP
    LOG_INFO("Reapplying static IP configuration...");
#ifndef WIFI_USE_BSSID
    IPAddress staticIP, gateway, subnet, dns;

    if (staticIP.fromString(WIFI_STATIC_IP) &&
        gateway.fromString(WIFI_STATIC_GATEWAY) &&
        subnet.fromString(WIFI_STATIC_SUBNET) &&
        dns.fromString(WIFI_STATIC_DNS))
    {
        WiFi.config(staticIP, gateway, subnet, dns);
        LOG_INFO("Static IP configuration reapplied successfully");
    }
    else
    {
        LOG_ERROR("Invalid static IP configuration during reconnection - falling back to DHCP");
    }
#else

#ifdef HAS_SECONDARY_DNS
  LOG_INFO("Both DNS servers are set");
  if ( !WiFi.config(staticIP, gateway, subnet, primaryDNS, secondaryDNS) ) {
#else
#ifdef HAS_PRIMARY_DNS
  LOG_INFO("Only primary DNS server is set");
  if ( !WiFi.config(staticIP, gateway, subnet, primaryDNS) ) {
#else
  LOG_INFO("No DNS servers are set!");
  if ( !WiFi.config(staticIP, gateway, subnet) ) {
#endif
#endif
        LOG_ERROR("Failed to configure static IP - falling back to DHCP");
  } else {
        LOG_INFO("Static IP configuration applied successfully");
  }
#endif

#endif

    int attempts = 0;
    const int maxAttempts = 5;
    bool reconnectionSuccess = false;

    while (!reconnectionSuccess && attempts < maxAttempts)
    {
        WiFi.begin();

        // Faster reconnection wait
        int waitAttempts = 0;
        const int waitMaxAttempts = 10; // 1 second max per attempt

        while (WiFi.status() != WL_CONNECTED && waitAttempts < waitMaxAttempts)
        {
            delay(WIFI_DELAY_MS);
            waitAttempts++;

            // Quick error detection
            if (WiFi.status() == WL_CONNECT_FAILED || WiFi.status() == WL_NO_SSID_AVAIL)
            {
                LOG_VERBOSEF("WiFi reconnection error (attempt %d): %d", attempts + 1, WiFi.status());
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

            // Shorter delay between attempts
            delay(300 + (attempts * 100));

            // Clean and restart
            WiFi.disconnect(true, true);
            delay(100);
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
    LOG_INFOF("SSID:   %s", WiFi.SSID().c_str());
    LOG_INFOF("BSSID:  %s", WiFi.BSSIDstr());
    LOG_INFOF("IP:     %s", WiFi.localIP().toString().c_str());
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

    // Configure static IP if enabled (same as in begin method)
#ifdef WIFI_USE_STATIC_IP
        LOG_INFO("Reapplying static IP configuration after auth error...");
#ifndef WIFI_USE_BSSID
        IPAddress staticIP, gateway, subnet, dns;

        if (staticIP.fromString(WIFI_STATIC_IP) &&
            gateway.fromString(WIFI_STATIC_GATEWAY) &&
            subnet.fromString(WIFI_STATIC_SUBNET) &&
            dns.fromString(WIFI_STATIC_DNS))
        {
            WiFi.config(staticIP, gateway, subnet, dns);
            LOG_INFO("Static IP configuration reapplied after auth error");
        }
        else
        {
            LOG_ERROR("Invalid static IP configuration after auth error - falling back to DHCP");
        }
#else

#ifdef HAS_SECONDARY_DNS
  LOG_INFO("Both DNS servers are set");
  if ( !WiFi.config(staticIP, gateway, subnet, primaryDNS, secondaryDNS) ) {
#else
#ifdef HAS_PRIMARY_DNS
  LOG_INFO("Only primary DNS server is set");
  if ( !WiFi.config(staticIP, gateway, subnet, primaryDNS) ) {
#else
  LOG_INFO("No DNS servers are set!");
  if ( !WiFi.config(staticIP, gateway, subnet) ) {
#endif
#endif
        LOG_ERROR("Failed to configure static IP - falling back to DHCP");
  } else {
        LOG_INFO("Static IP configuration applied successfully");
  }
#endif

    // Wait longer before restarting
    delay(3000);

    LOG_INFO("WiFi parameters reset to resolve authentication error");
    return true;
#endif
}
