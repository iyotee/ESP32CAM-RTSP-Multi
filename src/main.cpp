/**
 * @file main.cpp
 * @brief Main entry point for ESP32-CAM RTSP MJPEG multi-client firmware
 * @author Jeremy Noverraz
 * @version 1.0
 * @date 2025
 *
 * This firmware transforms an ESP32-CAM into a multi-client RTSP/MJPEG video server
 * with modular architecture and centralized configuration.
 *
 * Main features:
 * - Multi-client RTSP MJPEG server (up to 5 simultaneous clients)
 * - HTTP MJPEG server for browser access
 * - Robust WiFi management with monitoring
 * - 100% centralized configuration via config.h
 * - Professional logger with verbosity levels
 *
 * @note All parameters are configurable via config.h
 * @see config.h for complete configuration
 */

#include "config.h"
#include "../lib/WiFiManager/WiFiManager.h"
#include "../lib/CameraManager/CameraManager.h"
#include "../lib/Nano-RTSP/NanoRTSPServer.h"
#include "../lib/Utils/Logger.h"
#include "../lib/Utils/Helpers.h"
#include "../lib/HTTPMJPEGServer/HTTPMJPEGServer.h"
#include <WebServer.h>

// === GLOBAL INSTANCES ===

// Modular HTTP MJPEG server
HTTPMJPEGServer httpMJPEGServer(HTTP_SERVER_PORT);

// Global RTSP server instance (dynamic allocation)
NanoRTSPServer *rtspServer = nullptr;

// Monitoring variables
unsigned long startupTime = 0;
unsigned long lastHealthCheck = 0;

// === MAIN FUNCTIONS ===

/**
 * @brief Initial system configuration
 *
 * Initializes all modules in order:
 * 1. Logger and serial communication
 * 2. WiFi connection
 * 3. Camera initialization
 * 4. RTSP and HTTP server startup
 *
 * @note This function only returns on critical error
 */
void setup()
{
    // === BASIC INITIALIZATION ===
    Serial.begin(SERIAL_BAUD_RATE);
    Logger::setLogLevel(LOG_LEVEL);

    startupTime = millis();

    LOG_INFO("==========================================");
    LOG_INFO("ESP32-CAM RTSP MJPEG Multi-Clients v1.0");
    LOG_INFO("==========================================");

    // Display system information
    Helpers::printSystemInfo();

    // === WIFI CONNECTION ===
    LOG_INFO("Initializing WiFi connection...");
    WiFiManager::begin(WIFI_SSID, WIFI_PASSWORD);

    if (!WiFiManager::isConnected())
    {
        LOG_ERROR("WiFi connection failed - Restarting...");
        ESP.restart();
    }

    // Display WiFi information (removed double initialization)
    Helpers::printWiFiInfo();

    // === OTA (Over-The-Air) ===
    // (Removed)

    // === CAMERA INITIALIZATION ===
    LOG_INFO("Initializing ESP32-CAM camera...");
    LOG_INFO("Starting camera initialization process...");
    
    bool cameraInitResult = CameraManager::begin();
    LOG_INFOF("Camera initialization result: %s", cameraInitResult ? "SUCCESS" : "FAILED");
    
    if (!cameraInitResult)
    {
        LOG_ERROR("Camera initialization failed - Restarting...");
        ESP.restart();
    }

    LOG_INFO("Camera initialized successfully");
    LOG_INFO("Getting camera information...");
    LOG_INFO(CameraManager::getCameraInfo().c_str());

    // === SERVER STARTUP ===

    // RTSP Server
    LOG_INFO("Starting RTSP server...");
    rtspServer = new NanoRTSPServer(RTSP_PORT);
    if (!rtspServer)
    {
        LOG_ERROR("RTSP server allocation failed");
        ESP.restart();
    }

    LOG_INFOF("RTSP server created on port %d", RTSP_PORT);
    rtspServer->begin();
    LOG_INFO("RTSP server started successfully");

    // HTTP MJPEG Server
    LOG_INFO("Starting HTTP MJPEG server...");
    httpMJPEGServer.setCaptureCallback([]() -> camera_fb_t *
                                       { return CameraManager::capture(); });
    httpMJPEGServer.begin();
    LOG_INFO("HTTP MJPEG server started successfully");

    // === CONFIGURATION COMPLETE ===
    LOG_INFO("==========================================");
    LOG_INFO("Configuration completed successfully!");
    LOG_INFO("==========================================");
    LOG_INFO("System ready - entering main loop...");

    // Display access URLs
    String localIP = WiFiManager::getLocalIP().toString();
    LOG_INFOF("RTSP Stream: rtsp://%s:%d%s", localIP.c_str(), RTSP_PORT, RTSP_PATH);
    LOG_INFOF("HTTP Stream: http://%s%s", localIP.c_str(), HTTP_MJPEG_PATH);
    LOG_INFO("Compatible clients: VLC, FFmpeg, web browsers");
    LOG_INFO("Limit: 5 simultaneous RTSP clients");

    // Final system information
    Helpers::printMemoryInfo();
    LOG_INFO("==========================================");
}

// === UTILITY FUNCTIONS ===

/**
 * @brief Performs a system health check
 *
 * Displays debug information and checks the status
 * of different system modules.
 */
void performHealthCheck()
{
    LOG_DEBUG("=== Health Check ===");
    LOG_DEBUGF("Uptime: %s", FORMAT_UPTIME(millis() - startupTime).c_str());
    LOG_DEBUGF("IP: %s", WiFiManager::getLocalIP().toString().c_str());
    LOG_DEBUGF("WiFi quality: %d%%", WiFiManager::getSignalQuality());
    LOG_DEBUGF("WiFi stable: %s", WiFiManager::isStable() ? "Yes" : "No");
    LOG_DEBUGF("Free memory: %s", FORMAT_BYTES(Helpers::getFreeMemory()).c_str());
    LOG_DEBUGF("Memory used: %d%%", Helpers::getMemoryUsage());
    LOG_DEBUGF("Camera initialized: %s", CameraManager::isInitialized() ? "Yes" : "No");
    LOG_DEBUG("===================");
}

/**
 * @brief Main system loop
 *
 * Continuously manages:
 * - RTSP and HTTP clients
 * - System health monitoring
 * - Periodic debug logs
 * - WiFi stability verification
 *
 * @note Non-blocking function with yield() for stability
 */
void loop()
{
    // === CLIENT MANAGEMENT ===

    // RTSP client management
    if (rtspServer)
    {
        rtspServer->handleClients();
    }
    else
    {
        LOG_ERROR("RTSP server is null!");
    }

    // HTTP MJPEG client management
    httpMJPEGServer.handleClient();

    // === PERIODIC MONITORING ===

    // System health check
    if (IS_TIME_ELAPSED(lastHealthCheck, DEBUG_INTERVAL_MS))
    {
        performHealthCheck();
        lastHealthCheck = millis();
    }

    // WiFi stability verification with improved error handling
    static unsigned long lastWiFiCheck = 0;
    if (IS_TIME_ELAPSED(lastWiFiCheck, 30000)) // Every 30 seconds
    {
        if (!WiFiManager::isConnected())
        {
            LOG_WARN("WiFi disconnected - attempting reconnection");
            WiFiManager::reconnect();
        }
        else if (!WiFiManager::isStable())
        {
            LOG_WARN("WiFi connection unstable - continuous monitoring");
            // Don't force reconnection if connected but unstable
        }
        lastWiFiCheck = millis();
    }

    // Adaptive delay for main loop
    delay(MAIN_LOOP_DELAY);

    // Allow other tasks to execute
    yield();
}
