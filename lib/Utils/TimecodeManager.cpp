/**
 * @file TimecodeManager.cpp
 * @author Jeremy Noverraz
 * @version 1.0
 * @date 2025
 * @brief Implementation of timecode and metadata manager for RTSP/FFmpeg
 */
// TimecodeManager.cpp
#include "TimecodeManager.h"
#include "Logger.h"

TimecodeManager::TimecodeManager()
{
    clock_reference = 0;
    start_time_ms = 0;
    last_sync_time = 0;
    sync_status = RTSP_CLOCK_SYNC_ERROR;
    timecode_mode = RTSP_TIMECODE_MODE;
    frame_counter = 0;
    last_frame_timestamp = 0;
    ntp_timestamp = 0;
    ntp_synced = false;

    // Initialize reference clock immediately
    start_time_ms = millis();
    clock_reference = esp_timer_get_time() / 1000;
    last_sync_time = start_time_ms;

    LOG_DEBUG("TimecodeManager: Reference clock initialized");
}

TimecodeManager::~TimecodeManager()
{
    // Cleanup if needed
}

void TimecodeManager::begin()
{
    LOG_INFO("Initializing TimecodeManager");

    // Initialize reference clock
    initializeClock();

    // Sync with NTP if enabled
#ifdef RTSP_NTP_SERVER
        syncWithNTP();
#endif

    LOG_INFOF("TimecodeManager initialized - Mode: %d", timecode_mode);
}

void TimecodeManager::initializeClock()
{
    start_time_ms = millis();
    clock_reference = esp_timer_get_time() / 1000; // Microseconds to milliseconds
    last_sync_time = start_time_ms;

    LOG_DEBUG("Reference clock initialized");
}

void TimecodeManager::syncWithNTP()
{
    if (!WiFi.isConnected())
    {
        LOG_WARN("No WiFi connection for NTP synchronization");
        return;
    }

    LOG_INFO("Synchronizing with NTP server...");

    // Configure NTP server
    configTime(0, 0, RTSP_NTP_SERVER);

    // Wait for synchronization
    unsigned long start_wait = millis();
    while (!time(nullptr) && (millis() - start_wait) < RTSP_NTP_TIMEOUT)
    {
        delay(100);
        yield();
    }

    if (time(nullptr))
    {
        ntp_synced = true;
        ntp_timestamp = time(nullptr);
        sync_status = RTSP_CLOCK_SYNC_OK;
        last_sync_time = millis();

        LOG_INFO("NTP synchronization successful");
        LOG_INFOF("NTP timestamp: %lu", ntp_timestamp);
    }
    else
    {
        LOG_WARN("NTP synchronization failed");
        sync_status = RTSP_CLOCK_SYNC_ERROR;
    }
}

void TimecodeManager::updateClockReference()
{
    uint32_t current_time = millis();

    // Update clock reference
    clock_reference = esp_timer_get_time() / 1000;

    // Check if NTP resynchronization is needed
#ifdef RTSP_NTP_SERVER
    if ((current_time - last_sync_time) > (RTSP_NTP_SYNC_INTERVAL * 1000))
    {
        syncWithNTP();
    }
#endif
}

RTSPTimecode_t TimecodeManager::generateTimecode()
{
    RTSPTimecode_t timecode;

    updateClockReference();
    updateFrameCounter();

    // Ensure frame_counter is never 0 to avoid zero timestamps
    if (frame_counter == 0)
    {
        frame_counter = 1;
    }

    // Calculate timecodes according to mode
    switch (timecode_mode)
    {
    case 0: // Basic mode
        timecode.pts = getCurrentTimestamp();
        timecode.dts = timecode.pts;
        timecode.clock_reference = clock_reference;
        timecode.wall_clock = getWallClockMs();
        break;

    case 1: // Advanced mode
        timecode.pts = calculatePTS(frame_counter);
        timecode.dts = calculateDTS(frame_counter);
        timecode.clock_reference = clock_reference;
        timecode.wall_clock = getWallClockMs();
        break;

    case 2: // Expert mode
        timecode.pts = calculatePTS(frame_counter);
        timecode.dts = calculateDTS(frame_counter);
        timecode.clock_reference = clock_reference;
        timecode.wall_clock = getWallClockMs();

        // Add synchronization metadata
        if (ntp_synced)
        {
            timecode.clock_reference |= 0x80000000; // Synchronization bit
        }
        break;

    default:
        timecode.pts = getCurrentTimestamp();
        timecode.dts = timecode.pts;
        timecode.clock_reference = clock_reference;
        timecode.wall_clock = getWallClockMs();
        break;
    }

    // Ensure timecodes are never 0
    if (timecode.pts == 0)
    {
        timecode.pts = RTSP_CLOCK_RATE / RTSP_FPS; // 1 frame in RTP timestamps
    }
    if (timecode.dts == 0)
    {
        timecode.dts = timecode.pts;
    }

    // Force increasing timecodes if enabled
    if (RTSP_FORCE_INCREASING_TIMECODES)
    {
        if (timecode.pts <= last_frame_timestamp)
        {
            timecode.pts = last_frame_timestamp + (RTSP_CLOCK_RATE / RTSP_FPS);
        }
        if (timecode.dts <= last_frame_timestamp)
        {
            timecode.dts = timecode.pts;
        }
    }

    // Final timestamp consistency check
    if (timecode.dts > timecode.pts)
    {
        // DTS should never be greater than PTS for MJPEG
        timecode.dts = timecode.pts;
    }

    last_frame_timestamp = timecode.pts;

    LOG_DEBUGF("Timecode updated - PTS: %lu, DTS: %lu, Frame: %lu",
               timecode.pts, timecode.dts, frame_counter);

    return timecode;
}

uint32_t TimecodeManager::getCurrentTimestamp()
{
    uint32_t wall_clock = getWallClockMs();
    uint32_t rtp_timestamp = msToRTPTimestamp(wall_clock);

    // Ensure timestamp is never 0
    if (rtp_timestamp == 0)
    {
        rtp_timestamp = RTSP_CLOCK_RATE / RTSP_FPS;
    }

    return rtp_timestamp;
}

uint32_t TimecodeManager::getWallClockMs()
{
    uint32_t elapsed = millis() - start_time_ms;

    // Ensure elapsed time is never 0
    if (elapsed == 0)
    {
        elapsed = 1; // At least 1ms
    }

    return elapsed;
}

RTSPClockMetadata_t TimecodeManager::getClockMetadata()
{
    RTSPClockMetadata_t metadata;

    metadata.ntp_timestamp = ntp_timestamp;
    metadata.rtp_timestamp = getCurrentTimestamp();
    metadata.wall_clock_ms = getWallClockMs();
    metadata.clock_sync_status = sync_status;
    metadata.timecode_mode = timecode_mode;

    return metadata;
}

RTSPMJPEGMetadata_t TimecodeManager::getMJPEGMetadata(uint16_t width, uint16_t height)
{
    RTSPMJPEGMetadata_t metadata;

    metadata.quality_factor = RTSP_MJPEG_QUALITY_METADATA;
    metadata.width = width;
    metadata.height = height;
    metadata.precision = RTSP_TIMECODE_PRECISION_MEDIUM;
    metadata.fragmentation_info = RTSP_ENABLE_FRAGMENTATION_INFO;

    return metadata;
}

bool TimecodeManager::isClockSynchronized()
{
    return (sync_status == RTSP_CLOCK_SYNC_OK);
}

void TimecodeManager::setClockSyncStatus(uint8_t status)
{
    sync_status = status;
}

uint32_t TimecodeManager::calculatePTS(uint32_t frame_number)
{
    // PTS = Presentation Time Stamp
    // Precise calculation according to RTSP/RTP standards
    // Increment = RTSP_CLOCK_RATE / RTSP_FPS = 90000 / 15 = 6000 per frame

    // Calculate exact timestamp for this frame
    uint32_t frame_duration_rtp = RTSP_CLOCK_RATE / RTSP_FPS; // 6000 for 15 FPS at 90kHz
    uint32_t pts = frame_number * frame_duration_rtp;

    // Ensure timestamp is never 0 for first frame
    if (pts == 0 && frame_number > 0)
    {
        pts = frame_duration_rtp;
    }

    LOG_DEBUGF("PTS calculated - Frame: %lu, PTS: %lu, Increment: %lu (%.2f ms)",
               frame_number, pts, frame_duration_rtp, (float)frame_duration_rtp * 1000.0 / RTSP_CLOCK_RATE);

    return pts;
}

uint32_t TimecodeManager::calculateDTS(uint32_t frame_number)
{
    // DTS = Decoding Time Stamp
    // For MJPEG, DTS = PTS because each frame is independent
    return calculatePTS(frame_number);
}

uint32_t TimecodeManager::msToRTPTimestamp(uint32_t ms)
{
    // Convert milliseconds to RTP timestamp
    return (ms * RTSP_CLOCK_RATE) / 1000;
}

uint32_t TimecodeManager::rtpTimestampToMs(uint32_t rtp_ts)
{
    // Convert RTP timestamp to milliseconds
    return (rtp_ts * 1000) / RTSP_CLOCK_RATE;
}

uint32_t TimecodeManager::getNTPTimestamp()
{
    if (ntp_synced)
    {
        return ntp_timestamp;
    }
    return 0;
}

void TimecodeManager::updateFrameCounter()
{
    frame_counter++;
}

void TimecodeManager::resetFrameCounter()
{
    frame_counter = 0;
    last_frame_timestamp = 0;
    LOG_DEBUG("Frame counter reset");
}

uint32_t TimecodeManager::getFrameCounter()
{
    return frame_counter;
}
