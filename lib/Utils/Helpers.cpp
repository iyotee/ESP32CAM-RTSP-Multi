/**
 * @file Helpers.cpp
 * @author Jeremy Noverraz
 * @version 1.0
 * @date 2025
 * @brief Implementation of reusable utility functions
 */
// Helpers.cpp
// Implementation of reusable utility functions

#include "Helpers.h"
#include "Logger.h"
#include "../../src/config.h"
#include <esp_system.h>
#include <esp_heap_caps.h>
#include <esp_wifi.h>

// === String management ===

String Helpers::ipToString(const IPAddress &ip)
{
    return ip.toString();
}

String Helpers::macToString(const uint8_t *mac)
{
    char macStr[MAC_STRING_SIZE];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return String(macStr);
}

String Helpers::formatBytes(size_t bytes)
{
    if (bytes < BYTES_KB)
    {
        return String(bytes) + " B";
    }
    else if (bytes < BYTES_MB)
    {
        return String(bytes / 1024.0, 1) + " KB";
    }
    else if (bytes < BYTES_GB)
    {
        return String(bytes / (1024.0 * 1024.0), 1) + " MB";
    }
    else
    {
        return String(bytes / (1024.0 * 1024.0 * 1024.0), 1) + " GB";
    }
}

String Helpers::formatUptime(unsigned long uptime)
{
    unsigned long seconds = uptime / 1000;
    unsigned long minutes = seconds / SECONDS_PER_MINUTE;
    unsigned long hours = minutes / MINUTES_PER_HOUR;
    unsigned long days = hours / HOURS_PER_DAY;

    if (days > 0)
    {
        return String(days) + "d " + String(hours % HOURS_PER_DAY) + "h " + String(minutes % MINUTES_PER_HOUR) + "m";
    }
    else if (hours > 0)
    {
        return String(hours) + "h " + String(minutes % MINUTES_PER_HOUR) + "m " + String(seconds % SECONDS_PER_MINUTE) + "s";
    }
    else if (minutes > 0)
    {
        return String(minutes) + "m " + String(seconds % SECONDS_PER_MINUTE) + "s";
    }
    else
    {
        return String(seconds) + "s";
    }
}

// === WiFi management ===

int Helpers::getWiFiRSSI()
{
    return WiFi.RSSI();
}

int Helpers::getWiFiQuality()
{
    int rssi = WiFi.RSSI();
    if (rssi <= WIFI_RSSI_MIN)
        return 0;
    if (rssi >= WIFI_RSSI_MAX)
        return 100;
    return WIFI_QUALITY_MULTIPLIER * (rssi + WIFI_RSSI_OFFSET);
}

bool Helpers::isWiFiStable()
{
    return WiFi.status() == WL_CONNECTED && getWiFiQuality() > WIFI_QUALITY_THRESHOLD;
}

// === Memory management ===

int Helpers::getMemoryUsage()
{
    size_t total = getTotalMemory();
    size_t free = getFreeMemory();
    if (total == 0)
        return 0;
    return (int)((total - free) * 100 / total);
}

size_t Helpers::getFreeMemory()
{
    return ESP.getFreeHeap();
}

size_t Helpers::getTotalMemory()
{
    return ESP.getHeapSize();
}

// === Time management ===

bool Helpers::isTimeElapsed(unsigned long startTime, unsigned long interval)
{
    return (millis() - startTime) >= interval;
}

unsigned long Helpers::getElapsedTime(unsigned long startTime)
{
    return millis() - startTime;
}

// === Conversions and utilities ===

String Helpers::intToString(int value, int width, char padChar)
{
    String result = String(value);
    while (result.length() < width)
    {
        result = String(padChar) + result;
    }
    return result;
}

int Helpers::clamp(int value, int min, int max)
{
    if (value < min)
        return min;
    if (value > max)
        return max;
    return value;
}

int Helpers::mapRange(int value, int fromLow, int fromHigh, int toLow, int toHigh)
{
    return (value - fromLow) * (toHigh - toLow) / (fromHigh - fromLow) + toLow;
}

String Helpers::getWiFiModeString()
{
    // Use WiFi.mode() instead of esp_wifi_get_mode()
    wifi_mode_t mode = WiFi.getMode();
    switch (mode)
    {
    case WIFI_MODE_NULL:
        return "NULL";
    case WIFI_MODE_STA:
        return "STATION";
    case WIFI_MODE_AP:
        return "ACCESS_POINT";
    case WIFI_MODE_APSTA:
        return "AP_STA";
    default:
        return "UNKNOWN";
    }
}

// === Debug and diagnostics ===

void Helpers::printSystemInfo()
{
    LOG_INFO("=== System Information ===");
    LOG_INFOF("Chip: %s", ESP.getChipModel());
    LOG_INFOF("CPU Freq: %d MHz", ESP.getCpuFreqMHz());
    LOG_INFOF("Flash Size: %s", FORMAT_BYTES(ESP.getFlashChipSize()).c_str());
    LOG_INFOF("SDK Version: %s", ESP.getSdkVersion());
    LOG_INFOF("Uptime: %s", FORMAT_UPTIME(millis()).c_str());
    LOG_INFO("==========================");
}

void Helpers::printWiFiInfo()
{
    LOG_INFO("=== WiFi Information ===");
    LOG_INFOF("SSID: %s", WiFi.SSID().c_str());
    LOG_INFOF("IP: %s", ipToString(WiFi.localIP()).c_str());
    LOG_INFOF("Gateway: %s", ipToString(WiFi.gatewayIP()).c_str());
    LOG_INFOF("DNS: %s", ipToString(WiFi.dnsIP()).c_str());
    LOG_INFOF("MAC: %s", WiFi.macAddress().c_str());
    LOG_INFOF("RSSI: %d dBm", getWiFiRSSI());
    LOG_INFOF("Quality: %d%%", getWiFiQuality());
    LOG_INFOF("Mode: %s", getWiFiModeString().c_str());
    LOG_INFO("========================");
}

void Helpers::printMemoryInfo()
{
    LOG_INFO("=== Memory Information ===");
    LOG_INFOF("Total: %s", FORMAT_BYTES(getTotalMemory()).c_str());
    LOG_INFOF("Free: %s", FORMAT_BYTES(getFreeMemory()).c_str());
    LOG_INFOF("Used: %d%%", getMemoryUsage());
    LOG_INFOF("PSRAM Total: %s", FORMAT_BYTES(ESP.getPsramSize()).c_str());
    LOG_INFOF("PSRAM Free: %s", FORMAT_BYTES(ESP.getFreePsram()).c_str());
    LOG_INFO("===========================");
}