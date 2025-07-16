/**
 * @file Types.h
 * @author Jeremy Noverraz
 * @version 1.0
 * @date 2025
 * @brief Centralized types for ESP32CAM-RTSP-Multi firmware
 */
// Types.h
// Centralized types for ESP32CAM-RTSP-Multi firmware

#ifndef TYPES_H
#define TYPES_H

#include <esp_camera.h>
#include <functional>

// Callback types for image capture
typedef std::function<camera_fb_t *()> CaptureCallback;

// Types for configuration parameters
typedef struct
{
    int width;
    int height;
    int quality;
    int fps;
} CameraConfig;

typedef struct
{
    int port;
    const char *path;
    const char *server_name;
} ServerConfig;

#endif // TYPES_H