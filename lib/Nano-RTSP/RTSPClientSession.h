/**
 * @file RTSPClientSession.h
 * @author Jeremy Noverraz
 * @version 1.0
 * @date 2025
 * @brief Manages an individual RTSP session (SETUP, PLAY, PAUSE, TEARDOWN) and RTP/JPEG packet transmission.
 */
// RTSPClientSession.h
#ifndef RTSP_CLIENT_SESSION_H
#define RTSP_CLIENT_SESSION_H

#include <WiFi.h>
#include <WiFiUdp.h>
#include "../Utils/TimecodeManager.h"

/**
 * @class RTSPClientSession
 * @brief Manages an individual RTSP session (SETUP, PLAY, PAUSE, TEARDOWN) and RTP/JPEG packet transmission.
 *        Each connected client has its own instance of this class.
 */
class RTSPClientSession
{
public:
    RTSPClientSession(WiFiClient client);
    ~RTSPClientSession();
    void handle();
    bool isConnected();

private:
    WiFiClient client;
    bool playing = false;
    String sessionId;
    WiFiUDP udp;
    IPAddress clientRtpIp;
    uint16_t clientRtpPort = 0;
    uint16_t clientRtcpPort = 0;
    unsigned long lastFrameTime = 0;
    unsigned long frameInterval = 50; // Interval between frames in ms
    uint16_t sequenceNumber = 0;      // Unique RTP sequence number per session
    uint32_t timestamp = 0;           // Unique RTP timestamp per session

    // TCP interleaved support
    bool useTcpInterleaved = false;
    uint8_t rtpChannel = 0;  // RTP channel for TCP interleaved
    uint8_t rtcpChannel = 1; // RTCP channel for TCP interleaved

    // Local UDP port used for RTP
    uint16_t serverUdpPort = 0;

    // UDP error counters for problem detection
    uint32_t udpErrorCount = 0;
    uint32_t lastUdpErrorTime = 0;

    // Adaptive framerate
    uint8_t currentFramerate = RTSP_FPS;
    uint32_t lastFramerateAdjustment = 0;

    // Advanced timecode manager
    TimecodeManager timecodeManager;
    RTSPTimecode_t currentTimecode;

    void processRequest();
    void sendRTPFrame();
    void sendRTPFrameTCP(); // New method for TCP interleaved
    void sendRTSPResponse(const char *status, const char *headers);
    String generateSessionId();
    bool isClientStillConnected(); // New method to detect disconnection
    void resetUDPConnection();     // New method to reset UDP

    // New methods for advanced timecodes
    void generateAdvancedSDP(String &sdp);
    void addClockMetadataToSDP(String &sdp);
    void addMJPEGMetadataToSDP(String &sdp, uint16_t width, uint16_t height);
    void updateTimecodeForFrame();
};

#endif // RTSP_CLIENT_SESSION_H
