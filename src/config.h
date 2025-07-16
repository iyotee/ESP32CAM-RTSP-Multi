/**
 * @file config.h
 * @author Jeremy Noverraz
 * @version 1.0
 * @date 2025
 * @brief Central configuration file for ESP32-CAM RTSP MJPEG multi-client firmware
 */
// config.h
// Configuration file for sensitive information and system parameters

#ifndef CONFIG_H
#define CONFIG_H

#include <esp_camera.h>
#include <functional>
// Callback type for image capture
typedef std::function<camera_fb_t *()> CaptureCallback;

// ===== WIFI CONFIGURATION =====
// ⚠️  MODIFY THESE VALUES BEFORE USE !
// Replace with your own WiFi credentials
#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"

// ===== CAMERA CONFIGURATION =====
// Camera resolution - Choose according to your needs:
// FRAMESIZE_QQVGA  = 160x120   (very low resolution, very fast)
// FRAMESIZE_QVGA   = 320x240   (low resolution, fast)
// FRAMESIZE_VGA    = 640x480   (standard resolution, good compromise)
// FRAMESIZE_SVGA   = 800x600   (high resolution, slower)
// FRAMESIZE_XGA    = 1024x768  (very high resolution, slow)
// FRAMESIZE_HD     = 1280x720  (HD, very slow)
// FRAMESIZE_SXGA   = 1280x1024 (very high resolution, very slow)
// FRAMESIZE_UXGA   = 1600x1200 (ultra high resolution, extremely slow)
#define CAMERA_FRAME_SIZE FRAMESIZE_VGA // 640x480

// JPEG Quality (0-100)
// 0-10   = Maximum quality, very large files
// 10-30  = Very high quality
// 30-50  = High quality
// 50-70  = Medium quality (recommended for RTSP)
// 70-90  = Low quality
// 90-100 = Very low quality, small files
#define CAMERA_JPEG_QUALITY 20 // 20 = OPTIMIZED for stability and FFmpeg compatibility

// XCLK frequency in Hz (10-20MHz recommended)
// 10MHz = Low frequency, energy saving
// 15MHz = Medium frequency, good compromise
// 20MHz = High frequency, better quality
#define CAMERA_XCLK_FREQ 15000000 // 15 MHz - OPTIMIZED for stability (reduced from 20MHz)

// ===== RTSP CONFIGURATION =====
// RTSP server port (554 = standard RTSP port, prefer port 8554 as routers or ISPs sometimes block port 554)
#define RTSP_PORT 8554 // Current port: 8554
// RTSP stream path (must start with /)
#define RTSP_PATH "/stream=0"

// HTTP MJPEG server port
#define HTTP_SERVER_PORT 80 // Current port: 80
// HTTP MJPEG stream path (must start with /)
#define HTTP_MJPEG_PATH "/mjpeg"

// RTSP server name in headers
#define RTSP_SERVER_NAME "ESP32CAM-RTSP-Multi/1.0"

// RTP ports for transport (if needed)
#define RTSP_CLIENT_PORT_RANGE "8000-8001"
#define RTSP_SERVER_PORT_RANGE "8002-8003"

// Framerate in SDP - MUST MATCH REAL FRAMERATE
#define RTSP_SDP_FRAMERATE 15

// RTSP frame startup delay (ms)
#define RTSP_STARTUP_DELAY 100

// Frames per second (FPS) - FIXED AND REALISTIC
// 5-10   = Very slow, energy saving
// 10-15  = Slow, good for surveillance (RECOMMENDED)
// 15-20  = Medium, good compromise
// 20-25  = Fast, smooth
// 25-30  = Very fast, may overload network
#define RTSP_FPS 15 // 15 FPS FIXED - matches SDP and ensures 66.67ms interval

// ===== RTSP TIMECODE AND METADATA CONFIGURATION =====
// Timecode mode for FFmpeg
// 0 = Basic mode (simple timestamp)
// 1 = Advanced mode (PTS/DTS with metadata)
// 2 = Expert mode (complete clock synchronization)
#define RTSP_TIMECODE_MODE 1 // Advanced mode recommended for FFmpeg

// Reference clock for timecodes (Hz)
// 90000 = RTP standard for video
// 48000 = Audio standard
// 44100 = CD audio
#define RTSP_CLOCK_RATE 90000

// Temporal metadata for FFmpeg
// 0 = Disabled (maximum compatibility)
// 1 = Enabled (better FFmpeg compatibility)
#define RTSP_ENABLE_CLOCK_METADATA 1

// NTP clock synchronization
// 0 = Disabled
// 1 = Enabled (synchronization with NTP server)
#define RTSP_ENABLE_NTP_SYNC 0

// NTP server for synchronization (if enabled)
#define RTSP_NTP_SERVER "pool.ntp.org"

// NTP synchronization interval (seconds)
#define RTSP_NTP_SYNC_INTERVAL 3600 // 1 hour

// Advanced MJPEG metadata
// 0 = Disabled
// 1 = Enabled (better compatibility with MJPEG players)
#define RTSP_ENABLE_MJPEG_METADATA 1

// MJPEG quality in metadata (0-100)
#define RTSP_MJPEG_QUALITY_METADATA 85

// MJPEG fragmentation information
// 0 = Disabled
// 1 = Enabled (fragmentation information in SDP)
#define RTSP_ENABLE_FRAGMENTATION_INFO 1

// Force increasing timecodes (avoids "Non-increasing DTS" errors)
// 0 = Disabled (normal mode)
// 1 = Enabled (forces always increasing timecodes)
#define RTSP_FORCE_INCREASING_TIMECODES 1

// ===== MJPEG VIDEO COMPATIBILITY CONFIGURATION =====
// Keyframe interval (in frames)
// MJPEG = 100% keyframes, but we can force "special" keyframes
// 1 = Each frame is a keyframe (recommended for MJPEG)
// 30 = Keyframe every 30 frames (about 1 second at 30fps)
#define RTSP_KEYFRAME_INTERVAL 1

// MJPEG quality optimized for compatibility
// 10-20 = High quality, larger files
// 20-40 = Medium quality, good compromise (recommended)
// 40-60 = Low quality, smaller files
#define RTSP_MJPEG_COMPATIBILITY_QUALITY 25

// MJPEG profile for maximum compatibility
// 0 = Standard profile
// 1 = Baseline profile (maximum compatibility)
#define RTSP_MJPEG_PROFILE_BASELINE 1

// Video compatibility metadata
// 0 = Disabled
// 1 = Enabled (adds metadata for video players)
#define RTSP_ENABLE_VIDEO_COMPATIBILITY_METADATA 1

// Codec information in SDP
// 0 = Disabled
// 1 = Enabled (adds detailed codec info)
#define RTSP_ENABLE_CODEC_INFO 1

// Keyframe signaling in SDP (RTSP compliant)
// 0 = Disabled
// 1 = Enabled (adds a=keyframe-interval in SDP)
#define RTSP_SIGNAL_KEYFRAMES_IN_SDP 1

// Advanced optimization: number of frame buffers and capture mode
#define CAMERA_FB_COUNT 2                       // 2 buffers - SAFE MODE
#define CAMERA_GRAB_MODE CAMERA_GRAB_WHEN_EMPTY // More stable mode

// WiFi quality threshold to consider connection as stable
#define WIFI_QUALITY_THRESHOLD 20

// ===== SYSTEM CONFIGURATION =====
// Serial port speed for debug messages
#define SERIAL_BAUD_RATE 115200 // Current speed: 115200 bauds

// Logger configuration
// Available levels: LOG_ERROR, LOG_WARN, LOG_INFO, LOG_DEBUG, LOG_VERBOSE
#define LOG_LEVEL LOG_INFO // Niveau INFO pour éviter le spam des logs

// ===== OPTIMIZED WIFI CONFIGURATION =====
// Delay between WiFi connection attempts (ms)
#define WIFI_DELAY_MS 200 // Balanced for stability and speed

// Maximum number of WiFi connection attempts
#define WIFI_MAX_ATTEMPTS 15 // Balanced for reliability

// Stabilization delay after WiFi connection (ms)
#define WIFI_STABILIZATION_DELAY 500 // Balanced for stability

// WiFi authentication error handling
// 0 = Disabled
// 1 = Enabled (automatic AUTH_EXPIRE error handling)
#define WIFI_HANDLE_AUTH_ERRORS 1

// Advanced WiFi configuration to avoid AUTH_EXPIRE
#define WIFI_POWER_SAVE_DISABLE 1 // Disable WiFi power saving
#define WIFI_AUTO_RECONNECT 1     // Automatic reconnection
#define WIFI_CLEAN_SESSION 1      // Clean session at each connection

// WiFi cleanup delay (ms)
#define WIFI_CLEANUP_DELAY 100 // Balanced for stability

// WiFi reconnection delay in milliseconds
#define WIFI_RECONNECT_DELAY 1000 // Balanced for reliability

// ===== OPTIMIZED UDP CONFIGURATION =====
// Maximum number of UDP send attempts before TCP fallback
#define RTSP_UDP_MAX_RETRIES 2 // Reduced to 2 attempts for faster TCP fallback

// Delay between UDP attempts (ms)
#define RTSP_UDP_RETRY_DELAY 10 // Increased to 10ms for better stability

// Automatic fallback to TCP if UDP fails
// 0 = Disabled (UDP only)
// 1 = Enabled (automatic fallback to TCP)
// 2 = Force TCP mode (UDP disabled) - RECOMMENDED for stability
#define RTSP_UDP_TCP_FALLBACK 1 // Fallback automatique UDP->TCP

// Adaptive delay between UDP fragments (ms)
#define RTSP_UDP_FRAGMENT_DELAY 2 // Increased to 2ms

// Adaptive framerate in case of UDP problems
// 0 = Disabled (fixed framerate)
// 1 = Enabled (automatic framerate reduction in case of errors)
#define RTSP_ADAPTIVE_FRAMERATE 1

// UDP error threshold to reduce framerate
#define RTSP_UDP_ERROR_THRESHOLD 5 // Increased to 5 errors

// Minimum framerate in case of UDP problems
#define RTSP_MIN_FRAMERATE 10 // Increased to 10 FPS minimum

// Maximum RTP fragment size (bytes) - optimized for UDP
#define RTSP_MAX_FRAGMENT_SIZE 600 // Reduced to 600 bytes for more reliable UDP

// UDP timeout to detect packet loss (ms)
#define RTSP_UDP_TIMEOUT 100

// Automatic UDP connection reset after repeated errors
#define RTSP_UDP_AUTO_RESET 1

// Error threshold for UDP reset
#define RTSP_UDP_RESET_THRESHOLD 10

// Delay before UDP reset (ms)
#define RTSP_UDP_RESET_DELAY 5000 // 5 seconds

// Main loop delay in milliseconds
// Shorter delay = more responsive system
// Longer delay = CPU saving
#define MAIN_LOOP_DELAY 10 // 10ms - OPTIMIZED for 15 FPS timing control

// ===== ADVANCED CAMERA CONFIGURATION =====
// OV2640 sensor parameters

// Brightness (-2 to 2)
// -2 = Very dark, -1 = Dark, 0 = Normal, 1 = Bright, 2 = Very bright
#define CAMERA_BRIGHTNESS 1 // Slightly brighter

// Contrast (-2 to 2)
// -2 = Very low, -1 = Low, 0 = Normal, 1 = High, 2 = Very high
#define CAMERA_CONTRAST 1 // A bit more contrast

// Saturation (-2 to 2)
// -2 = Very low, -1 = Low, 0 = Normal, 1 = High, 2 = Very high
#define CAMERA_SATURATION 1 // A bit more color

// Special effect (0-6)
// 0 = No effect, 1 = Negative, 2 = Grayscale, 3 = Red tint
// 4 = Green tint, 5 = Blue tint, 6 = Sepia
#define CAMERA_SPECIAL_EFFECT 0 // Current effect: 0 (none)

// Automatic white balance (0 = disabled, 1 = enabled)
#define CAMERA_WHITEBAL 1 // White balance: enabled

// AWB gain (0 = disabled, 1 = enabled)
#define CAMERA_AWB_GAIN 1 // AWB gain: enabled

// White balance mode (0-4) - if AWB enabled
// 0 = Auto, 1 = Sunny, 2 = Cloudy, 3 = Office, 4 = Home
#define CAMERA_WB_MODE 0 // Current mode: 0 (auto)

// Automatic exposure control (0 = disabled, 1 = enabled)
#define CAMERA_EXPOSURE_CTRL 1 // Exposure control: enabled

// AEC2 (0 = disabled, 1 = enabled)
#define CAMERA_AEC2 0 // AEC2: disabled

// Automatic gain control (0 = disabled, 1 = enabled)
#define CAMERA_GAIN_CTRL 1 // Gain control: enabled

// AGC gain (0-30)
#define CAMERA_AGC_GAIN 0 // Current AGC gain: 0

// Gain ceiling (0-6)
// 0 = x2, 1 = x4, 2 = x8, 3 = x16, 4 = x32, 5 = x64, 6 = x128
#define CAMERA_GAINCEILING 0 // Current ceiling: x2

// Bad pixel correction (0 = disabled, 1 = enabled)
#define CAMERA_BPC 0 // Pixel correction: disabled

// White pixel correction (0 = disabled, 1 = enabled)
#define CAMERA_WPC 1 // White pixel correction: enabled

// Raw gamma (0 = disabled, 1 = enabled)
#define CAMERA_RAW_GMA 1 // Raw gamma: enabled

// Lens correction (0 = disabled, 1 = enabled)
#define CAMERA_LENC 1 // Lens correction: enabled

// Horizontal mirror (0 = disabled, 1 = enabled)
#define CAMERA_HMIRROR 0 // Horizontal mirror: disabled

// Vertical flip (0 = disabled, 1 = enabled)
#define CAMERA_VFLIP 0 // Vertical flip: disabled

// Downsize (0 = disabled, 1 = enabled)
#define CAMERA_DCW 1 // Downsize: enabled

// Color bar (test) (0 = disabled, 1 = enabled)
#define CAMERA_COLORBAR 0 // Color bar: disabled

// HTTP response codes
#define HTTP_OK 200
#define HTTP_NOT_FOUND 404
#define HTTP_INTERNAL_ERROR 500

// Delays and thresholds
#define DEBUG_INTERVAL_MS 10000
#define MAC_STRING_SIZE 18
#define HEADERS_BUFFER_SIZE 512
#define LOG_BUFFER_SIZE 256

// WiFi thresholds
#define WIFI_RSSI_MIN -100
#define WIFI_RSSI_MAX -50
#define WIFI_RSSI_OFFSET 100
#define WIFI_QUALITY_MULTIPLIER 2

// Byte formatting thresholds
#define BYTES_KB 1024
#define BYTES_MB (1024 * 1024)
#define BYTES_GB (1024 * 1024 * 1024)

// Time thresholds
#define SECONDS_PER_MINUTE 60
#define MINUTES_PER_HOUR 60
#define HOURS_PER_DAY 24

// Default values
#define DEFAULT_CSEQ 1
#define DEFAULT_FRAME_TIME 0

// ===== ADVANCED TIMECODE CONFIGURATION =====
// Structure for PTS/DTS timecodes
typedef struct
{
    uint32_t pts;             // Presentation Time Stamp
    uint32_t dts;             // Decoding Time Stamp
    uint32_t clock_reference; // Clock reference
    uint32_t wall_clock;      // Wall clock (ms since startup)
} RTSPTimecode_t;

// Temporal metadata for FFmpeg
typedef struct
{
    uint32_t ntp_timestamp;    // NTP timestamp (if synchronized)
    uint32_t rtp_timestamp;    // RTP timestamp
    uint32_t wall_clock_ms;    // Wall clock in milliseconds
    uint8_t clock_sync_status; // Synchronization status (0=not sync, 1=sync)
    uint8_t timecode_mode;     // Timecode mode used
} RTSPClockMetadata_t;

// Advanced MJPEG metadata
typedef struct
{
    uint8_t quality_factor;     // Quality factor (0-100)
    uint16_t width;             // Image width
    uint16_t height;            // Image height
    uint8_t precision;          // Timecode precision
    uint8_t fragmentation_info; // Fragmentation information
} RTSPMJPEGMetadata_t;

// Constants for timecodes
#define RTSP_TIMECODE_PRECISION_HIGH 1   // High precision
#define RTSP_TIMECODE_PRECISION_MEDIUM 2 // Medium precision
#define RTSP_TIMECODE_PRECISION_LOW 3    // Low precision

// Metadata thresholds
#define RTSP_METADATA_BUFFER_SIZE 256
#define RTSP_TIMECODE_BUFFER_SIZE 64

// Synchronization delays
#define RTSP_CLOCK_SYNC_TIMEOUT 5000 // 5 seconds
#define RTSP_NTP_TIMEOUT 3000        // 3 seconds

// Status codes for metadata
#define RTSP_CLOCK_SYNC_OK 1
#define RTSP_CLOCK_SYNC_ERROR 0
#define RTSP_CLOCK_SYNC_PENDING 2

#endif // CONFIG_H
