/**
 * @file CameraManager.cpp
 * @author Jeremy Noverraz
 * @version 1.0
 * @date 2025
 * @brief Implementation of ESP32-CAM camera manager
 */

#include "CameraManager.h"
#include "../../src/config.h"
#include "../Utils/Logger.h"

// Static variable to track initialization status
bool CameraManager::initialized = false;

// Static variables for framerate control
static unsigned long lastCaptureTime = 0;
static unsigned long frameInterval = 1000 / RTSP_FPS; // Calculate interval from config

bool CameraManager::begin()
{
    LOG_INFO("Initializing ESP32-CAM camera...");

    // Hardware configuration for ESP32-CAM AI-Thinker
    camera_config_t config;

    // Pin configuration (AI-Thinker ESP32-CAM)
    config.pin_d0 = 5;
    config.pin_d1 = 18;
    config.pin_d2 = 19;
    config.pin_d3 = 21;
    config.pin_d4 = 36;
    config.pin_d5 = 39;
    config.pin_d6 = 34;
    config.pin_d7 = 35;
    config.pin_xclk = 0;
    config.pin_pclk = 22;
    config.pin_vsync = 25;
    config.pin_href = 23;
    config.pin_sccb_sda = 26;
    config.pin_sccb_scl = 27;
    config.pin_pwdn = 32;
    config.pin_reset = -1;

    // LED configuration (for flash)
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;

    // Capture configuration (from config.h)
    config.xclk_freq_hz = CAMERA_XCLK_FREQ;
    config.pixel_format = PIXFORMAT_JPEG;
    config.frame_size = CAMERA_FRAME_SIZE;
    config.jpeg_quality = CAMERA_JPEG_QUALITY;
    config.fb_count = CAMERA_FB_COUNT;   // Use macro for buffer count
    config.grab_mode = CAMERA_GRAB_MODE; // Use macro for capture mode

    LOG_DEBUGF("Camera configuration: %dx%d, quality=%d, XCLK=%dMHz, FPS=%d",
               config.frame_size, CAMERA_JPEG_QUALITY, CAMERA_XCLK_FREQ / 1000000, RTSP_FPS);

    // Camera initialization
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK)
    {
        LOG_ERRORF("Camera initialization error: %s", esp_err_to_name(err));
        return false;
    }

    // Advanced parameters configuration
    sensor_t *s = esp_camera_sensor_get();
    if (s)
    {
        configureAdvancedSettings(s);
        LOG_INFO("Advanced parameters configured");
    }
    else
    {
        LOG_WARN("Unable to get sensor for advanced configuration");
    }

    // Initialize timing control
    lastCaptureTime = 0;
    frameInterval = 1000 / RTSP_FPS;

    initialized = true;
    LOG_INFOF("Camera initialized successfully - Target FPS: %d, Interval: %lu ms", RTSP_FPS, frameInterval);
    return true;
}

camera_fb_t *CameraManager::capture()
{
    if (!initialized)
    {
        LOG_ERROR("Attempt to capture without camera initialization");
        return nullptr;
    }

    // STRICT FRAMERATE CONTROL - Prevent excessive capture rate
    unsigned long currentTime = millis();
    if (currentTime - lastCaptureTime < frameInterval)
    {
        // Too early for next frame, return null to indicate "wait"
        LOG_DEBUGF("Framerate control: %lu ms since last capture, need %lu ms",
                   currentTime - lastCaptureTime, frameInterval);
        return nullptr;
    }

    // Update capture time
    lastCaptureTime = currentTime;

    // Capture with error handling
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb)
    {
        LOG_ERROR("Image capture failed");
        return nullptr;
    }

    // Validate captured frame
    if (fb->len == 0 || fb->width == 0 || fb->height == 0)
    {
        LOG_ERROR("Invalid frame captured - empty or corrupted");
        esp_camera_fb_return(fb);
        return nullptr;
    }

    // Validate JPEG markers (SOI and EOI)
    if (fb->len >= 2)
    {
        // Check SOI marker (Start of Image): 0xFF 0xD8
        if (fb->buf[0] != 0xFF || fb->buf[1] != 0xD8)
        {
            Logger::errorf("Invalid JPEG SOI marker - expected 0xFF 0xD8, got 0x%02X 0x%02X",
                           fb->buf[0], fb->buf[1]);
            esp_camera_fb_return(fb);
            return nullptr;
        }

        // Check EOI marker (End of Image): 0xFF 0xD9
        if (fb->len >= 4)
        {
            if (fb->buf[fb->len - 2] != 0xFF || fb->buf[fb->len - 1] != 0xD9)
            {
                Logger::errorf("Invalid JPEG EOI marker - expected 0xFF 0xD9, got 0x%02X 0x%02X",
                               fb->buf[fb->len - 2], fb->buf[fb->len - 1]);
                esp_camera_fb_return(fb);
                return nullptr;
            }
        }
    }

    LOG_DEBUGF("Frame captured successfully: %d bytes, %dx%d, timestamp: %lu, JPEG valid",
               fb->len, fb->width, fb->height, currentTime);

    return fb;
}

camera_fb_t *CameraManager::captureForced()
{
    if (!initialized)
    {
        LOG_ERROR("Attempt to capture without camera initialization");
        return nullptr;
    }

    // FORCED CAPTURE - No timing restrictions, optimized for RTSP
    unsigned long currentTime = millis();

    // Capture with error handling - optimized for speed
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb)
    {
        LOG_ERROR("Forced image capture failed");
        return nullptr;
    }

    // Quick validation - only check essential parameters
    if (fb->len == 0 || fb->width == 0 || fb->height == 0)
    {
        LOG_ERROR("Invalid frame captured in forced mode - empty or corrupted");
        esp_camera_fb_return(fb);
        return nullptr;
    }

    // Minimal JPEG validation for speed
    if (fb->len >= 4 && (fb->buf[0] != 0xFF || fb->buf[1] != 0xD8 ||
                         fb->buf[fb->len - 2] != 0xFF || fb->buf[fb->len - 1] != 0xD9))
    {
        LOG_ERROR("Invalid JPEG markers in forced mode");
        esp_camera_fb_return(fb);
        return nullptr;
    }

    LOG_DEBUGF("Forced frame captured: %d bytes, %dx%d, timestamp: %lu, JPEG valid",
               fb->len, fb->width, fb->height, currentTime);

    return fb;
}

/**
 * @brief Release camera frame buffer - CRITICAL for memory management
 *
 * This function MUST be called after using a camera_fb_t to prevent memory leaks
 * and system crashes. Call this immediately after processing the frame.
 *
 * @param fb Pointer to camera frame buffer to release
 */
void CameraManager::releaseFrame(camera_fb_t *fb)
{
    if (fb)
    {
        esp_camera_fb_return(fb);
        LOG_DEBUG("Frame buffer released");
    }
}

bool CameraManager::isInitialized()
{
    return initialized;
}

std::string CameraManager::getCameraInfo()
{
    if (!initialized)
    {
        return "Camera not initialized";
    }

    sensor_t *s = esp_camera_sensor_get();
    if (!s)
    {
        return "Unable to get sensor information";
    }

    std::string info = "ESP32-CAM Camera\n";
    info += "Resolution: " + std::to_string(s->status.framesize) + "\n";
    info += "JPEG Quality: " + std::to_string(s->status.quality) + "\n";
    info += "XCLK Frequency: " + std::to_string(CAMERA_XCLK_FREQ / 1000000) + "MHz\n";
    info += "Target FPS: " + std::to_string(RTSP_FPS) + "\n";
    info += "Frame Interval: " + std::to_string(frameInterval) + "ms\n";
    info += "Pixel Format: JPEG\n";
    info += "Frame Buffers: " + std::to_string(CAMERA_FB_COUNT) + "\n";

    return info;
}

void CameraManager::configureAdvancedSettings(sensor_t *sensor)
{
    if (!sensor)
    {
        LOG_ERROR("Invalid sensor for advanced configuration");
        return;
    }

    // Brightness and contrast configuration
    sensor->set_brightness(sensor, CAMERA_BRIGHTNESS);
    sensor->set_contrast(sensor, CAMERA_CONTRAST);
    sensor->set_saturation(sensor, CAMERA_SATURATION);

    // White balance configuration
    sensor->set_whitebal(sensor, CAMERA_WHITEBAL);
    sensor->set_awb_gain(sensor, CAMERA_AWB_GAIN);
    sensor->set_wb_mode(sensor, CAMERA_WB_MODE);

    // Exposure and gain configuration
    sensor->set_exposure_ctrl(sensor, CAMERA_EXPOSURE_CTRL);
    sensor->set_aec2(sensor, CAMERA_AEC2);
    sensor->set_gain_ctrl(sensor, CAMERA_GAIN_CTRL);
    sensor->set_agc_gain(sensor, CAMERA_AGC_GAIN);
    sensor->set_gainceiling(sensor, (gainceiling_t)CAMERA_GAINCEILING);

    // Image correction configuration
    sensor->set_bpc(sensor, CAMERA_BPC);
    sensor->set_wpc(sensor, CAMERA_WPC);
    sensor->set_raw_gma(sensor, CAMERA_RAW_GMA);
    sensor->set_lenc(sensor, CAMERA_LENC);

    // Orientation configuration
    sensor->set_hmirror(sensor, CAMERA_HMIRROR);
    sensor->set_vflip(sensor, CAMERA_VFLIP);

    // Downsize and effects configuration
    sensor->set_dcw(sensor, CAMERA_DCW);
    sensor->set_colorbar(sensor, CAMERA_COLORBAR);
    sensor->set_special_effect(sensor, CAMERA_SPECIAL_EFFECT);

    LOG_DEBUG("Advanced camera configuration completed");
}
