/**
 * @file HTTPMJPEGServer.cpp
 * @author Jeremy Noverraz
 * @version 1.0
 * @date 2025
 * @brief Modular HTTP MJPEG server for ESP32-CAM
 */
// HTTPMJPEGServer.cpp
// Modular HTTP MJPEG server for ESP32-CAM

#include <esp_camera.h>
#include "HTTPMJPEGServer.h"
#include <Arduino.h>
#include "../Utils/Logger.h"
#include "../CameraManager/CameraManager.h"

HTTPMJPEGServer::HTTPMJPEGServer(int port)
    : server(port), listenPort(port), captureCb(nullptr) {}

void HTTPMJPEGServer::setCaptureCallback(CaptureCallback cb)
{
    captureCb = cb;
}

void HTTPMJPEGServer::begin()
{
    server.on(HTTP_MJPEG_PATH, HTTP_GET, [this]()
              { handleMJPEG(); });
    server.begin();
    LOG_INFOF("HTTP MJPEG server started on port %d", listenPort);
}

void HTTPMJPEGServer::handleClient()
{
    server.handleClient();
}

void HTTPMJPEGServer::handleMJPEG()
{
    if (!captureCb)
    {
        server.send(500, "text/plain", "Error: capture callback not defined");
        LOG_ERROR("Capture callback not defined for MJPEG");
        return;
    }
    server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    server.send(200, "multipart/x-mixed-replace; boundary=frame");
    while (server.client().connected())
    {
        camera_fb_t *fb = captureCb();
        if (!fb)
        {
            LOG_ERROR("Capture error for HTTP MJPEG");
            continue;
        }
        server.sendContent("--frame\r\n");
        server.sendContent("Content-Type: image/jpeg\r\n");
        server.sendContent("Content-Length: " + String(fb->len) + "\r\n\r\n");
        server.sendContent_P((const char *)fb->buf, fb->len);
        server.sendContent("\r\n");

        // CRITICAL: Release frame buffer to prevent memory leaks
        CameraManager::releaseFrame(fb);

        yield(); // Allow other tasks to execute (non-blocking)
    }
}