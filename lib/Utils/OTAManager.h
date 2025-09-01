/**
 * @file OTAManager.h
 * @author Jeremy Noverraz
 * @version 1.0
 * @date 2025
 * @brief OTA (Over-The-Air) firmware update manager for ESP32-CAM
 *
 * Provides simple OTA functionality for firmware updates via HTTP
 */

#ifndef OTA_MANAGER_H
#define OTA_MANAGER_H

#include <Arduino.h>
#include <WebServer.h>
#include <Update.h>
#include <WiFi.h>
#include "../../src/config.h"

class OTAManager
{
private:
    WebServer *otaServer;
    bool isUpdateInProgress;
    unsigned long updateStartTime;
    size_t updateTotalSize;
    size_t updateCurrentSize;

public:
    OTAManager();
    ~OTAManager();

    /**
     * @brief Initialize OTA server
     * @param port Port for OTA server (default: OTA_SERVER_PORT from config.h)
     * @return true if initialization successful
     */
    bool begin(int port = OTA_SERVER_PORT);

    /**
     * @brief Handle OTA client requests
     */
    void handleClient();

    /**
     * @brief Check if OTA update is in progress
     * @return true if update is active
     */
    bool isUpdating() const { return isUpdateInProgress; }

    /**
     * @brief Get update progress percentage
     * @return Progress percentage (0-100)
     */
    int getProgress() const;

    /**
     * @brief Get update status string
     * @return Status message
     */
    String getStatus() const;

private:
    void handleRoot();
    void handleUpdate();
    void handleUpload();
    void handleNotFound();
    void handleProgress();
};

#endif // OTA_MANAGER_H
