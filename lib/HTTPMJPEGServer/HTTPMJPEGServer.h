/**
 * @file HTTPMJPEGServer.h
 * @author Jeremy Noverraz
 * @version 1.0
 * @date 2025
 * @brief Modular HTTP MJPEG server for ESP32-CAM
 */
// HTTPMJPEGServer.h
// Modular HTTP MJPEG server for ESP32-CAM

#ifndef HTTP_MJPEG_SERVER_H
#define HTTP_MJPEG_SERVER_H

#include "../../src/config.h"
#include <esp_camera.h>
#include <WebServer.h>
#include <functional>

class HTTPMJPEGServer
{
public:
    HTTPMJPEGServer(int port = HTTP_SERVER_PORT);
    void setCaptureCallback(CaptureCallback cb);
    void begin();
    void handleClient();

private:
    WebServer server;
    int listenPort;
    CaptureCallback captureCb;
    void handleMJPEG();
};

#endif // HTTP_MJPEG_SERVER_H