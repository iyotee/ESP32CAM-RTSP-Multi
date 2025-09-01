/**
 * @file OTAManager.cpp
 * @author Jeremy Noverraz
 * @version 1.0
 * @date 2025
 * @brief OTA (Over-The-Air) firmware update manager implementation
 */

#include "OTAManager.h"
#include "../Utils/Logger.h"
#include "../../src/config.h"
#include <esp_camera.h>

OTAManager::OTAManager() : otaServer(nullptr), isUpdateInProgress(false),
                           updateStartTime(0), updateTotalSize(0), updateCurrentSize(0)
{
}

OTAManager::~OTAManager()
{
    if (otaServer)
    {
        delete otaServer;
        otaServer = nullptr;
    }
}

bool OTAManager::begin(int port)
{
    if (otaServer)
    {
        delete otaServer;
    }

    otaServer = new WebServer(port);
    if (!otaServer)
    {
        LOG_ERROR("Failed to allocate OTA server");
        return false;
    }

    // Setup routes
    otaServer->on("/", HTTP_GET, [this]()
                  { handleRoot(); });
    otaServer->on("/update", HTTP_GET, [this]()
                  { handleUpdate(); });
    // Upload endpoint: completion handler + streaming upload handler
    otaServer->on("/upload", HTTP_POST, [this]()
                  {
                      // Completion callback after upload stream handled
                      if (Update.hasError())
                      {
                          LOG_ERRORF("OTA failed: %s", Update.errorString());
                          otaServer->send(500, "text/plain", String("Update failed: ") + Update.errorString());
                      }
                      else
                      {
                          LOG_INFO("OTA successful, restarting...");
                          otaServer->send(200, "text/plain", "Update successful");
                          delay(1000);
                          ESP.restart();
                      } }, [this]()
                  { handleUpload(); });
    otaServer->on("/progress", HTTP_GET, [this]()
                  { handleProgress(); });
    otaServer->onNotFound([this]()
                          { handleNotFound(); });

    otaServer->begin();
    LOG_INFOF("OTA server started on port %d", port);
    return true;
}

void OTAManager::handleClient()
{
    if (otaServer)
    {
        otaServer->handleClient();
    }
}

int OTAManager::getProgress() const
{
    if (!isUpdateInProgress || updateTotalSize == 0)
        return 0;

    return (updateCurrentSize * 100) / updateTotalSize;
}

String OTAManager::getStatus() const
{
    if (!isUpdateInProgress)
        return "Idle";

    if (updateTotalSize == 0)
        return "Preparing...";

    return "Updating: " + String(getProgress()) + "%";
}

void OTAManager::handleRoot()
{
    String html = "<!DOCTYPE html><html><head>";
    html += "<title>ESP32-CAM OTA Update</title>";
    html += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
    html += "<style>";
    html += "body { font-family: Arial, sans-serif; margin: 20px; background: #f0f0f0; }";
    html += ".container { max-width: 600px; margin: 0 auto; background: white; padding: 20px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }";
    html += "h1 { color: #333; text-align: center; }";
    html += ".upload-area { border: 2px dashed #ccc; padding: 40px; text-align: center; margin: 20px 0; border-radius: 5px; }";
    html += ".upload-area:hover { border-color: #007bff; }";
    html += "input[type=\"file\"] { display: none; }";
    html += ".btn { background: #007bff; color: white; padding: 12px 24px; border: none; border-radius: 5px; cursor: pointer; font-size: 16px; }";
    html += ".btn:hover { background: #0056b3; }";
    html += ".btn:disabled { background: #ccc; cursor: not-allowed; }";
    html += ".progress { width: 100%; height: 20px; background: #f0f0f0; border-radius: 10px; overflow: hidden; margin: 20px 0; }";
    html += ".progress-bar { height: 100%; background: #28a745; width: 0%; transition: width 0.3s; }";
    html += ".status { text-align: center; margin: 10px 0; font-weight: bold; }";
    html += ".info { background: #e9ecef; padding: 15px; border-radius: 5px; margin: 20px 0; }";
    html += "</style></head><body>";
    html += "<div class=\"container\">";
    html += "<h1>ESP32-CAM Firmware Update</h1>";
    html += "<div class=\"info\">";
    html += "<strong>Instructions:</strong><br>";
    html += "1. Select your firmware file (.bin)<br>";
    html += "2. Click \"Upload Firmware\"<br>";
    html += "3. Wait for the upload to complete<br>";
    html += "4. The device will restart automatically";
    html += "</div>";
    html += "<div class=\"upload-area\" onclick=\"document.getElementById('firmware').click()\">";
    html += "<p>Click here to select firmware file</p>";
    html += "<input type=\"file\" id=\"firmware\" accept=\".bin\" onchange=\"updateFileName()\">";
    html += "</div>";
    html += "<div style=\"text-align: center;\">";
    html += "<button class=\"btn\" onclick=\"uploadFirmware()\" id=\"uploadBtn\" disabled>Upload Firmware</button>";
    html += "</div>";
    html += "<div class=\"progress\" id=\"progress\" style=\"display: none;\">";
    html += "<div class=\"progress-bar\" id=\"progressBar\"></div>";
    html += "</div>";
    html += "<div class=\"status\" id=\"status\"></div>";
    html += "</div>";
    html += "<script>";
    html += "function updateFileName() {";
    html += "    const file = document.getElementById('firmware').files[0];";
    html += "    if (file) {";
    html += "        document.querySelector('.upload-area p').textContent = 'Selected: ' + file.name;";
    html += "        document.getElementById('uploadBtn').disabled = false;";
    html += "    }";
    html += "}";
    html += "function uploadFirmware() {";
    html += "    const file = document.getElementById('firmware').files[0];";
    html += "    if (!file) return;";
    html += "    const formData = new FormData();";
    html += "    formData.append('firmware', file);";
    html += "    document.getElementById('uploadBtn').disabled = true;";
    html += "    document.getElementById('progress').style.display = 'block';";
    html += "    document.getElementById('status').textContent = 'Uploading...';";
    html += "    const xhr = new XMLHttpRequest();";
    html += "    xhr.upload.addEventListener('progress', function(e) {";
    html += "        if (e.lengthComputable) {";
    html += "            const percentComplete = (e.loaded / e.total) * 100;";
    html += "            document.getElementById('progressBar').style.width = percentComplete + '%';";
    html += "            document.getElementById('status').textContent = 'Uploading: ' + Math.round(percentComplete) + '%';";
    html += "        }";
    html += "    });";
    html += "    xhr.addEventListener('load', function() {";
    html += "        if (xhr.status === 200) {";
    html += "            document.getElementById('status').textContent = 'Upload successful! Device will restart...';";
    html += "            document.getElementById('progressBar').style.background = '#28a745';";
    html += "            setTimeout(() => { window.location.reload(); }, 3000);";
    html += "        } else {";
    html += "            document.getElementById('status').textContent = 'Upload failed: ' + xhr.responseText;";
    html += "            document.getElementById('uploadBtn').disabled = false;";
    html += "        }";
    html += "    });";
    html += "    xhr.addEventListener('error', function() {";
    html += "        document.getElementById('status').textContent = 'Upload failed!';";
    html += "        document.getElementById('uploadBtn').disabled = false;";
    html += "    });";
    html += "    xhr.open('POST', '/upload');";
    html += "    xhr.send(formData);";
    html += "}";
    html += "</script></body></html>";

    otaServer->send(200, "text/html", html);
}

void OTAManager::handleUpdate()
{
    String html = "<!DOCTYPE html><html><head>";
    html += "<title>ESP32-CAM OTA Update</title>";
    html += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
    html += "<style>";
    html += "body { font-family: Arial, sans-serif; margin: 20px; background: #f0f0f0; }";
    html += ".container { max-width: 600px; margin: 0 auto; background: white; padding: 20px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }";
    html += "h1 { color: #333; text-align: center; }";
    html += ".upload-area { border: 2px dashed #ccc; padding: 40px; text-align: center; margin: 20px 0; border-radius: 5px; }";
    html += ".upload-area:hover { border-color: #007bff; }";
    html += "input[type=\"file\"] { display: none; }";
    html += ".btn { background: #007bff; color: white; padding: 12px 24px; border: none; border-radius: 5px; cursor: pointer; font-size: 16px; }";
    html += ".btn:hover { background: #0056b3; }";
    html += ".btn:disabled { background: #ccc; cursor: not-allowed; }";
    html += ".progress { width: 100%; height: 20px; background: #f0f0f0; border-radius: 10px; overflow: hidden; margin: 20px 0; }";
    html += ".progress-bar { height: 100%; background: #28a745; width: 0%; transition: width 0.3s; }";
    html += ".status { text-align: center; margin: 10px 0; font-weight: bold; }";
    html += ".info { background: #e9ecef; padding: 15px; border-radius: 5px; margin: 20px 0; }";
    html += "</style></head><body>";
    html += "<div class=\"container\">";
    html += "<h1>ESP32-CAM Firmware Update</h1>";
    html += "<div class=\"info\">";
    html += "<strong>Instructions:</strong><br>";
    html += "1. Select your firmware file (.bin)<br>";
    html += "2. Click \"Upload Firmware\"<br>";
    html += "3. Wait for the upload to complete<br>";
    html += "4. The device will restart automatically";
    html += "</div>";
    html += "<div class=\"upload-area\" onclick=\"document.getElementById('firmware').click()\">";
    html += "<p>Click here to select firmware file</p>";
    html += "<input type=\"file\" id=\"firmware\" accept=\".bin\" onchange=\"updateFileName()\">";
    html += "</div>";
    html += "<div style=\"text-align: center;\">";
    html += "<button class=\"btn\" onclick=\"uploadFirmware()\" id=\"uploadBtn\" disabled>Upload Firmware</button>";
    html += "</div>";
    html += "<div class=\"progress\" id=\"progress\" style=\"display: none;\">";
    html += "<div class=\"progress-bar\" id=\"progressBar\"></div>";
    html += "</div>";
    html += "<div class=\"status\" id=\"status\"></div>";
    html += "</div>";
    html += "<script>";
    html += "function updateFileName() {";
    html += "    const file = document.getElementById('firmware').files[0];";
    html += "    if (file) {";
    html += "        document.querySelector('.upload-area p').textContent = 'Selected: ' + file.name;";
    html += "        document.getElementById('uploadBtn').disabled = false;";
    html += "    }";
    html += "}";
    html += "function uploadFirmware() {";
    html += "    const file = document.getElementById('firmware').files[0];";
    html += "    if (!file) return;";
    html += "    const formData = new FormData();";
    html += "    formData.append('firmware', file);";
    html += "    document.getElementById('uploadBtn').disabled = true;";
    html += "    document.getElementById('progress').style.display = 'block';";
    html += "    document.getElementById('status').textContent = 'Uploading...';";
    html += "    const xhr = new XMLHttpRequest();";
    html += "    xhr.upload.addEventListener('progress', function(e) {";
    html += "        if (e.lengthComputable) {";
    html += "            const percentComplete = (e.loaded / e.total) * 100;";
    html += "            document.getElementById('progressBar').style.width = percentComplete + '%';";
    html += "            document.getElementById('status').textContent = 'Uploading: ' + Math.round(percentComplete) + '%';";
    html += "        }";
    html += "    });";
    html += "    xhr.addEventListener('load', function() {";
    html += "        if (xhr.status === 200) {";
    html += "            document.getElementById('status').textContent = 'Upload successful! Device will restart...';";
    html += "            document.getElementById('progressBar').style.background = '#28a745';";
    html += "            setTimeout(() => { window.location.reload(); }, 3000);";
    html += "        } else {";
    html += "            document.getElementById('status').textContent = 'Upload failed: ' + xhr.responseText;";
    html += "            document.getElementById('uploadBtn').disabled = false;";
    html += "        }";
    html += "    });";
    html += "    xhr.addEventListener('error', function() {";
    html += "        document.getElementById('status').textContent = 'Upload failed!';";
    html += "        document.getElementById('uploadBtn').disabled = false;";
    html += "    });";
    html += "    xhr.open('POST', '/upload');";
    html += "    xhr.send(formData);";
    html += "}";
    html += "</script></body></html>";

    otaServer->send(200, "text/html", html);
}

void OTAManager::handleUpload()
{
    HTTPUpload &upload = otaServer->upload();

    if (upload.status == UPLOAD_FILE_START)
    {
        isUpdateInProgress = true;
        updateStartTime = millis();
        updateTotalSize = upload.totalSize;
        updateCurrentSize = 0;

        LOG_INFOF("Starting OTA update. Size: %d bytes", updateTotalSize);

        // Free as much memory as possible for OTA (deinit camera)
        esp_camera_deinit();
        LOG_DEBUG("Camera deinitialized for OTA");

        // Do not validate size at start; browsers may not send totalSize reliably

        // Start update with unknown size to allow streaming uploads
        if (Update.begin(UPDATE_SIZE_UNKNOWN))
        {
            LOG_INFO("Update.begin() successful");
        }
        else
        {
            LOG_ERROR("Update.begin() failed");
            LOG_ERRORF("Update error: %s", Update.errorString());
            Update.abort();
            isUpdateInProgress = false;
            return;
        }
    }
    else if (upload.status == UPLOAD_FILE_WRITE)
    {
        // Stream chunk; defer size validation to Update.end(true)
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize)
        {
            LOG_ERROR("Update.write() failed");
            Update.abort();
            isUpdateInProgress = false;
            return;
        }

        updateCurrentSize += upload.currentSize;
        LOG_DEBUGF("Update progress: %d/%d bytes (%d%%)",
                   updateCurrentSize, updateTotalSize, getProgress());
    }
    else if (upload.status == UPLOAD_FILE_END)
    {
        LOG_INFOF("Upload completed. Total bytes: %d, Expected: %d", updateCurrentSize, updateTotalSize);

        // Finalize update (true = accept and validate even if size differs slightly)
        if (Update.end(true))
        {
            LOG_INFO("Update completed successfully");
        }
        else
        {
            LOG_ERROR("Update.end() failed");
            LOG_ERRORF("Update error: %s", Update.errorString());
            LOG_ERRORF("Update error code: %d", Update.getError());
            LOG_ERRORF("Update size: %d bytes", Update.size());
            LOG_ERRORF("Update progress: %d bytes", Update.progress());
        }

        isUpdateInProgress = false;
    }
    else
    {
        LOG_ERRORF("Upload error: %d", upload.status);
        otaServer->send(500, "text/plain", "Upload error");
        isUpdateInProgress = false;
    }
}

void OTAManager::handleProgress()
{
    String json = "{\"progress\":" + String(getProgress()) +
                  ",\"status\":\"" + getStatus() + "\"}";
    otaServer->send(200, "application/json", json);
}

void OTAManager::handleNotFound()
{
    otaServer->send(404, "text/plain", "Not found");
}
