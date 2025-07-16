/**
 * @file CameraManager.h
 * @author Jeremy Noverraz
 * @version 1.0
 * @date 2025
 * @brief ESP32-CAM camera manager with optimized configuration
 */

#ifndef CAMERA_MANAGER_H
#define CAMERA_MANAGER_H

#include <esp_camera.h>
#include <string>

/**
 * @brief ESP32-CAM camera manager class
 *
 * Provides centralized camera management with:
 * - Hardware configuration for ESP32-CAM AI-Thinker
 * - Optimized capture settings
 * - Memory management
 * - Framerate control
 * - Error handling
 */
class CameraManager
{
public:
    /**
     * @brief Initialize the camera with optimal settings
     *
     * Configures the ESP32-CAM with settings from config.h
     * including resolution, quality, framerate, and advanced parameters.
     *
     * @return true if initialization successful, false otherwise
     */
    static bool begin();

    /**
     * @brief Capture a single frame with framerate control
     *
     * Captures a JPEG frame from the camera with strict timing control
     * to maintain the configured framerate. Returns nullptr if it's too
     * early for the next frame (framerate control).
     *
     * @return Pointer to camera frame buffer, or nullptr if error or too early
     * @note Call releaseFrame() immediately after processing the frame
     */
    static camera_fb_t *capture();

    /**
     * @brief Capture a single frame without timing restrictions (for TCP mode)
     *
     * Captures a JPEG frame from the camera without framerate control.
     * Used when timing is not critical (e.g., TCP fallback mode).
     *
     * @return Pointer to camera frame buffer, or nullptr if error
     * @note Call releaseFrame() immediately after processing the frame
     */
    static camera_fb_t *captureForced();

    /**
     * @brief Release camera frame buffer - CRITICAL for memory management
     *
     * This function MUST be called after using a camera_fb_t to prevent
     * memory leaks and system crashes. Call this immediately after
     * processing the frame.
     *
     * @param fb Pointer to camera frame buffer to release
     */
    static void releaseFrame(camera_fb_t *fb);

    /**
     * @brief Check if camera is initialized
     *
     * @return true if camera is ready, false otherwise
     */
    static bool isInitialized();

    /**
     * @brief Get camera information as string
     *
     * Returns detailed information about camera configuration
     * including resolution, quality, framerate, etc.
     *
     * @return String with camera information
     */
    static std::string getCameraInfo();

private:
    static bool initialized;

    /**
     * @brief Configure advanced camera parameters
     *
     * Sets brightness, contrast, white balance, exposure,
     * and other advanced parameters for optimal image quality.
     *
     * @param sensor Pointer to camera sensor
     */
    static void configureAdvancedSettings(sensor_t *sensor);

    /**
     * @brief Add HLS metadata to JPEG frame for better compatibility
     *
     * Adds EXIF metadata to JPEG frames to improve HLS compatibility
     * with FFmpeg and video players.
     *
     * @param fb Pointer to camera frame buffer
     * @return true if metadata added successfully, false otherwise
     */
    static bool addHLSMetadataToJPEG(camera_fb_t *fb);
};

#endif // CAMERA_MANAGER_H
