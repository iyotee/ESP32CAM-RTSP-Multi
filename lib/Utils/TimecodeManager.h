/**
 * @file TimecodeManager.h
 * @author Jeremy Noverraz
 * @version 1.0
 * @date 2025
 * @brief Timecode and metadata manager for RTSP/FFmpeg
 */
// TimecodeManager.h
// Timecode and metadata manager for RTSP/FFmpeg

#ifndef TIMECODE_MANAGER_H
#define TIMECODE_MANAGER_H

#include <WiFi.h>
#include <time.h>
#include "../../src/config.h"

/**
 * @class TimecodeManager
 * @brief Manages PTS/DTS timecodes and temporal metadata for optimal FFmpeg compatibility
 */
class TimecodeManager
{
public:
    TimecodeManager();
    ~TimecodeManager();

    // Initialization and configuration
    void begin();
    void syncWithNTP();
    void updateClockReference();

    // Timecode generation
    RTSPTimecode_t generateTimecode();
    uint32_t getCurrentTimestamp();
    uint32_t getWallClockMs();

    // Metadata for FFmpeg
    RTSPClockMetadata_t getClockMetadata();
    RTSPMJPEGMetadata_t getMJPEGMetadata(uint16_t width, uint16_t height);

    // Clock synchronization
    bool isClockSynchronized();
    void setClockSyncStatus(uint8_t status);

    // Utilities
    uint32_t calculatePTS(uint32_t frame_number);
    uint32_t calculateDTS(uint32_t frame_number);
    uint32_t msToRTPTimestamp(uint32_t ms);
    uint32_t rtpTimestampToMs(uint32_t rtp_ts);
    void resetFrameCounter();   // New method to reset counter
    uint32_t getFrameCounter(); // Access frame counter

private:
    // Reference clock
    uint32_t clock_reference;
    uint32_t start_time_ms;
    uint32_t last_sync_time;

    // Synchronization status
    uint8_t sync_status;
    uint8_t timecode_mode;

    // Frame counters
    uint32_t frame_counter;
    uint32_t last_frame_timestamp;

    // NTP metadata
    uint32_t ntp_timestamp;
    bool ntp_synced;

    // Private methods
    void initializeClock();
    uint32_t getNTPTimestamp();
    void updateFrameCounter();
};

#endif // TIMECODE_MANAGER_H