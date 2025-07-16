/**
 * @file Helpers.h
 * @author Jeremy Noverraz
 * @version 1.0
 * @date 2025
 * @brief Module of reusable utility functions
 */
// Helpers.h
// Module of reusable utility functions

#ifndef HELPERS_H
#define HELPERS_H

#include <Arduino.h>
#include <WiFi.h>

/**
 * @class Helpers
 * @brief Provides static utility functions for string management, WiFi, memory, time, etc.
 *        Centralizes conversions, diagnostics and practical tools for the entire project.
 */
class Helpers
{
public:
    // === String management ===

    // Format an IP address to String
    static String ipToString(const IPAddress &ip);

    // Format a MAC address to String
    static String macToString(const uint8_t *mac);

    // Format a size in bytes to readable format (KB, MB, etc.)
    static String formatBytes(size_t bytes);

    // Format time in milliseconds to readable format
    static String formatUptime(unsigned long uptime);

    // === WiFi management ===

    // Get WiFi signal strength in dBm
    static int getWiFiRSSI();

    // Get WiFi signal quality as percentage
    static int getWiFiQuality();

    // Check if WiFi is connected and stable
    static bool isWiFiStable();

    // === Memory management ===

    // Get memory usage as percentage
    static int getMemoryUsage();

    // Get free memory in bytes
    static size_t getFreeMemory();

    // Get total memory size in bytes
    static size_t getTotalMemory();

    // === Time management ===

    // Check if a delay has elapsed (non-blocking)
    static bool isTimeElapsed(unsigned long startTime, unsigned long interval);

    // Get elapsed time since a timestamp
    static unsigned long getElapsedTime(unsigned long startTime);

    // === Conversions and utilities ===

    // Convert an integer to String with padding
    static String intToString(int value, int width = 0, char padChar = '0');

    // Limit a value between min and max
    static int clamp(int value, int min, int max);

    // Map a value from one range to another
    static int mapRange(int value, int fromLow, int fromHigh, int toLow, int toHigh);

    // Get the name of the current WiFi mode
    static String getWiFiModeString();

    // === Debug and diagnostics ===

    // Display system information
    static void printSystemInfo();

    // Display WiFi information
    static void printWiFiInfo();

    // Display memory information
    static void printMemoryInfo();
};

// Utility macros to facilitate usage
#define FORMAT_BYTES(bytes) Helpers::formatBytes(bytes)
#define FORMAT_UPTIME(uptime) Helpers::formatUptime(uptime)
#define IS_TIME_ELAPSED(start, interval) Helpers::isTimeElapsed(start, interval)
#define CLAMP(value, min, max) Helpers::clamp(value, min, max)

#endif // HELPERS_H