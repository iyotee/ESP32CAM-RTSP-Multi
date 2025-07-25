/**
 * @file RTSPClientSession.cpp
 * @author Jeremy Noverraz
 * @version 1.0
 * @date 2025
 * @brief Implementation of RTSP client session management and RTP/JPEG packet transmission
 */
// RTSPClientSession.cpp
#include "RTSPClientSession.h"
#include "../CameraManager/CameraManager.h"
#include "../../src/config.h"
#include "../Utils/Logger.h"
#include <stdlib.h> // For abs()

RTSPClientSession::RTSPClientSession(WiFiClient client) : client(client)
{
    LOG_INFO("New RTSP session created");
    sessionId = generateSessionId();
    lastFrameTime = DEFAULT_FRAME_TIME;
    frameInterval = 1000 / RTSP_FPS; // Interval between frames in ms

    // Initialize timecode manager
    timecodeManager.begin();
    LOG_INFO("TimecodeManager initialized for RTSP session");

    // UDP will be initialized on first use
    LOG_INFO("RTSP session ready for UDP/TCP");
}

RTSPClientSession::~RTSPClientSession()
{
    if (client.connected())
        client.stop();
}

void RTSPClientSession::handle()
{
    if (!client.connected())
    {
        return;
    }

    if (client.available())
    {
        processRequest();
    }

    // Send frames only if in playback mode and client is connected
    if (playing && isClientStillConnected())
    {
        unsigned long currentTime = millis();

        // Adjust framerate if necessary (adaptive framerate)
        if (RTSP_ADAPTIVE_FRAMERATE && (currentTime - lastFramerateAdjustment) > 5000)
        {
            lastFramerateAdjustment = currentTime;

            if (udpErrorCount >= RTSP_UDP_ERROR_THRESHOLD)
            {
                // Reduce framerate in case of UDP errors
                if (currentFramerate > RTSP_MIN_FRAMERATE)
                {
                    currentFramerate = max(RTSP_MIN_FRAMERATE, currentFramerate - 2);
                    frameInterval = 1000 / currentFramerate;
                    LOG_INFOF("Framerate reduced to %d FPS due to UDP errors", currentFramerate);
                }
            }
            else if (udpErrorCount == 0 && currentFramerate < RTSP_FPS)
            {
                // Increase framerate if no more errors
                currentFramerate = min(RTSP_FPS, currentFramerate + 1);
                frameInterval = 1000 / currentFramerate;
                LOG_INFOF("Framerate increased to %d FPS", currentFramerate);
            }
        }

        if (currentTime - lastFrameTime >= frameInterval)
        {
            LOG_DEBUGF("Frame interval check - current: %lu, last: %lu, interval: %lu, need: %lu",
                       currentTime, lastFrameTime, currentTime - lastFrameTime, frameInterval);

            // Validate timing - should be approximately 66.67ms for 15 FPS
            unsigned long actualInterval = currentTime - lastFrameTime;

// Only log timing warnings if enabled (debug mode)
#if RTSP_DISABLE_TIMING_WARNINGS == 0
            if (actualInterval < RTSP_TIMING_TOLERANCE_MIN || actualInterval > RTSP_TIMING_TOLERANCE_MAX)
            {
                Logger::warnf("Timing deviation detected - expected ~67ms, got %lu ms", actualInterval);
            }
#endif

// Timing compensation (if enabled)
#if RTSP_TIMING_COMPENSATION == 1
            if (currentTime - lastCompensationTime > 500) // Every 500ms for more responsive compensation
            {
                // Calculate drift and apply compensation
                int32_t expectedInterval = frameInterval;
                int32_t drift = actualInterval - expectedInterval;
                timingDrift += drift;

                // Apply compensation to next frame interval
                if (abs(timingDrift) > RTSP_COMPENSATION_FACTOR)
                {
                    int32_t newInterval = max(50, (int32_t)frameInterval - (timingDrift / 500));
                    if (newInterval != frameInterval)
                    {
                        frameInterval = newInterval;
                        LOG_DEBUGF("Timing compensation applied: new interval %lu ms", frameInterval);
                    }
                    timingDrift = 0; // Reset drift after compensation
                }

                lastCompensationTime = currentTime;
            }
#endif

            LOG_DEBUGF("About to send RTP frame - Interval: %lu ms, FPS: %d", actualInterval, currentFramerate);
            sendRTPFrame();
            LOG_DEBUG("RTP frame sent successfully");
            lastFrameTime = currentTime;
        }
    }
    else if (playing && !isClientStillConnected())
    {
        LOG_WARN("Client disconnected during playback - stopping stream");
        playing = false;
    }

    // Periodic UDP health check (every 10 seconds)
    static unsigned long lastUdpHealthCheck = 0;
    unsigned long currentTime = millis();
    if (playing && !useTcpInterleaved && (currentTime - lastUdpHealthCheck) > 10000)
    {
        lastUdpHealthCheck = currentTime;

        // If recent UDP errors, check connection
        if (udpErrorCount > 0 && (currentTime - lastUdpErrorTime) < 5000)
        {
            LOG_INFO("UDP health check - recent errors detected");
            // Reset will be done automatically on next error
        }
    }
}

bool RTSPClientSession::isConnected()
{
    return client.connected();
}

bool RTSPClientSession::isClientStillConnected()
{
    // More robust client connection check
    if (!client.connected())
    {
        return false;
    }

    // Check if client responds by sending a ping (optional)
    // For now, we use basic verification
    return true;
}

void RTSPClientSession::resetUDPConnection()
{
    LOG_INFO("Complete UDP connection reset");

    // Properly stop UDP connection
    udp.stop();
    delay(50); // Longer delay to allow connection to close

    // Reset UDP parameters
    udpErrorCount = 0;
    lastUdpErrorTime = 0;

    // Restart UDP on server port
    if (udp.begin(serverUdpPort))
    {
        LOG_INFOF("UDP reset successfully on port %d", serverUdpPort);
    }
    else
    {
        LOG_ERRORF("UDP reset failed on port %d", serverUdpPort);
    }

    // Wait a bit to stabilize the new connection
    delay(100);
}

void RTSPClientSession::processRequest()
{
    String request = "";
    String line;

    // Read complete header
    while (client.available())
    {
        line = client.readStringUntil('\n');
        request += line + "\n";
        if (line.length() <= 1)
            break; // Header ended
    }

    LOG_DEBUG("RTSP request received:");
    LOG_VERBOSE(request);

    // Extract first line to identify method
    int firstLineEnd = request.indexOf('\n');
    String firstLine = request.substring(0, firstLineEnd);

    // Check if path is supported
    bool validPath = false;
    if (firstLine.indexOf(HTTP_MJPEG_PATH) != -1 || firstLine.indexOf(RTSP_PATH) != -1)
    {
        validPath = true;
    }

    // Extract CSeq dynamically
    int cseq = DEFAULT_CSEQ; // Default value
    int cseqIndex = request.indexOf("CSeq:");
    if (cseqIndex != -1)
    {
        int cseqEnd = request.indexOf('\n', cseqIndex);
        String cseqLine = request.substring(cseqIndex, cseqEnd);
        cseqLine.trim();
        int sep = cseqLine.indexOf(':');
        if (sep != -1)
        {
            String cseqValue = cseqLine.substring(sep + 1);
            cseqValue.trim();
            cseq = cseqValue.toInt();
        }
    }

    char headers[HEADERS_BUFFER_SIZE];
    if (firstLine.startsWith("OPTIONS"))
    {
        snprintf(headers, sizeof(headers),
                 "CSeq: %d\r\n"
                 "Public: OPTIONS, DESCRIBE, SETUP, PLAY, PAUSE, TEARDOWN\r\n"
                 "Server: " RTSP_SERVER_NAME "\r\n",
                 cseq);
        sendRTSPResponse("200 OK", headers);
    }
    else if (firstLine.startsWith("DESCRIBE"))
    {
        if (!validPath)
        {
            snprintf(headers, sizeof(headers), "CSeq: %d\r\n", cseq);
            sendRTSPResponse("404 Not Found", headers);
            return;
        }

        // Advanced SDP with metadata for FFmpeg
        String sdp;
        generateAdvancedSDP(sdp);

        int sdpLength = sdp.length();

        snprintf(headers, sizeof(headers),
                 "CSeq: %d\r\n"
                 "Content-Type: application/sdp\r\n"
                 "Content-Length: %d\r\n"
                 "Server: " RTSP_SERVER_NAME "\r\n",
                 cseq, sdpLength);
        sendRTSPResponse("200 OK", headers);

        // Send SDP in response body
        client.print(sdp);
    }
    else if (firstLine.startsWith("SETUP"))
    {
        if (!validPath)
        {
            snprintf(headers, sizeof(headers), "CSeq: %d\r\n", cseq);
            sendRTSPResponse("404 Not Found", headers);
            return;
        }

        // Extract client UDP ports and other Transport parameters
        int transportIndex = request.indexOf("Transport:");
        if (transportIndex != -1)
        {
            // Extract complete Transport line
            int transportEnd = request.indexOf('\r', transportIndex);
            String transportLine = request.substring(transportIndex, transportEnd);

            LOG_DEBUG("Transport header received: " + transportLine);
            LOG_DEBUG("Analyzing requested transport...");

            // Check if client requests TCP interleaved or if we force TCP mode
            if (transportLine.indexOf("interleaved") != -1 || transportLine.indexOf("RTP/AVP/TCP") != -1 || RTSP_UDP_TCP_FALLBACK == 2)
            {
                useTcpInterleaved = true;
                if (RTSP_UDP_TCP_FALLBACK == 2)
                {
                    LOG_INFO("Forcing TCP interleaved mode (UDP disabled)");
                }
                else
                {
                    LOG_INFO("Client requests TCP interleaved");
                }

                // Extract interleaved channels
                int channelIndex = transportLine.indexOf("interleaved=");
                if (channelIndex != -1)
                {
                    int channelStart = channelIndex + 12;
                    int channelEnd = transportLine.indexOf(';', channelStart);
                    if (channelEnd == -1)
                        channelEnd = transportLine.length();

                    String channelStr = transportLine.substring(channelStart, channelEnd);
                    int dashPos = channelStr.indexOf('-');
                    if (dashPos != -1)
                    {
                        rtpChannel = channelStr.substring(0, dashPos).toInt();
                        rtcpChannel = channelStr.substring(dashPos + 1).toInt();
                        LOG_INFOF("Interleaved channels: RTP=%d, RTCP=%d", rtpChannel, rtcpChannel);
                    }
                    else
                    {
                        // If no range, use default channels
                        rtpChannel = 0;
                        rtcpChannel = 1;
                        LOG_INFO("Default interleaved channels: RTP=0, RTCP=1");
                    }
                }
                else
                {
                    // Default channels if not specified
                    rtpChannel = 0;
                    rtcpChannel = 1;
                    LOG_INFO("Default interleaved channels: RTP=0, RTCP=1");
                }
            }
            else
            {
                if (RTSP_UDP_TCP_FALLBACK == 2)
                {
                    useTcpInterleaved = true;
                    LOG_INFO("Client requested UDP but forcing TCP mode");
                }
                else
                {
                    useTcpInterleaved = false;
                    LOG_INFO("Client requests UDP");
                }

                // Extract client_port for UDP
                int clientPortIndex = transportLine.indexOf("client_port=");
                if (clientPortIndex != -1)
                {
                    int portStart = clientPortIndex + 12;
                    int portSep = transportLine.indexOf('-', portStart);
                    int portEnd = transportLine.indexOf(';', portStart);
                    if (portEnd == -1)
                        portEnd = transportLine.length();

                    String port1 = transportLine.substring(portStart, portSep);
                    String port2 = transportLine.substring(portSep + 1, portEnd);
                    clientRtpPort = port1.toInt();
                    clientRtcpPort = port2.toInt();
                    clientRtpIp = client.remoteIP();

                    // Port validation
                    if (clientRtpPort == 0 || clientRtcpPort == 0)
                    {
                        LOG_ERROR("Invalid client ports in SETUP");
                        snprintf(headers, sizeof(headers), "CSeq: %d\r\n", cseq);
                        sendRTSPResponse("400 Bad Request", headers);
                        return;
                    }

                    LOG_INFOF("SETUP: client RTP IP=%s, port=%d-%d", clientRtpIp.toString().c_str(), clientRtpPort, clientRtcpPort);
                }
                else
                {
                    LOG_ERROR("Invalid Transport header - client_port missing for UDP");
                    snprintf(headers, sizeof(headers), "CSeq: %d\r\n", cseq);
                    sendRTSPResponse("400 Bad Request", headers);
                    return;
                }
            }

            // Extract source (if present)
            String source = "";
            int sourceIndex = transportLine.indexOf("source=");
            if (sourceIndex != -1)
            {
                int sourceStart = sourceIndex + 7;
                int sourceEnd = transportLine.indexOf(';', sourceStart);
                if (sourceEnd == -1)
                    sourceEnd = transportLine.length();
                source = transportLine.substring(sourceStart, sourceEnd);
            }
        }
        else
        {
            LOG_ERROR("Transport header missing in SETUP request");
            snprintf(headers, sizeof(headers), "CSeq: %d\r\n", cseq);
            sendRTSPResponse("400 Bad Request", headers);
            return;
        }

        // Build response according to transport mode
        if (useTcpInterleaved)
        {
            // TCP interleaved mode - no UDP needed
            LOG_INFO("TCP interleaved configuration");

            snprintf(headers, sizeof(headers),
                     "CSeq: %d\r\n"
                     "Transport: RTP/AVP/TCP;unicast;interleaved=%d-%d\r\n"
                     "Session: %s\r\n"
                     "Server: " RTSP_SERVER_NAME "\r\n",
                     cseq, rtpChannel, rtcpChannel, sessionId.c_str());
        }
        else
        {
            // UDP mode - initialize UDP
            serverUdpPort = 20000 + (esp_random() % 10000);
            if (!udp.begin(serverUdpPort))
            {
                LOG_ERROR("UDP initialization error");
                snprintf(headers, sizeof(headers), "CSeq: %d\r\n", cseq);
                sendRTSPResponse("500 Internal Server Error", headers);
                return;
            }

            uint16_t serverRtpPort = serverUdpPort;
            uint16_t serverRtcpPort = serverUdpPort + 1;

            LOG_INFOF("SETUP: server RTP port=%d-%d", serverRtpPort, serverRtcpPort);

            // Build correct Transport header according to RFC 2326
            snprintf(headers, sizeof(headers),
                     "CSeq: %d\r\n"
                     "Transport: RTP/AVP;unicast;client_port=%d-%d;server_port=%d-%d\r\n"
                     "Session: %s\r\n"
                     "Server: " RTSP_SERVER_NAME "\r\n",
                     cseq, clientRtpPort, clientRtcpPort, serverRtpPort, serverRtcpPort, sessionId.c_str());
        }

        LOG_DEBUG("SETUP response:");
        LOG_DEBUG("RTSP/1.0 200 OK");
        LOG_DEBUG(String(headers));
        LOG_DEBUG("Transport mode configured: " + String(useTcpInterleaved ? "TCP interleaved" : "UDP"));

        sendRTSPResponse("200 OK", headers);
    }
    else if (firstLine.startsWith("PLAY"))
    {
        if (!validPath)
        {
            snprintf(headers, sizeof(headers), "CSeq: %d\r\n", cseq);
            sendRTSPResponse("404 Not Found", headers);
            return;
        }

        snprintf(headers, sizeof(headers),
                 "CSeq: %d\r\n"
                 "Session: %s\r\n"
                 "Range: npt=0.000-\r\n"
                 "Server: " RTSP_SERVER_NAME "\r\n",
                 cseq, sessionId.c_str());
        sendRTSPResponse("200 OK", headers);
        playing = true;
        lastFrameTime = DEFAULT_FRAME_TIME; // Reset timer

        // Reset parameters for new playback
        currentFramerate = RTSP_FPS;
        frameInterval = 1000 / RTSP_FPS;
        udpErrorCount = 0;
        lastUdpErrorTime = 0;

        // Reset frame counter and sequence number for new session
        timecodeManager.resetFrameCounter();
        sequenceNumber = 0; // Reset RTP sequence number for new session

        LOG_INFOF("RTSP playback started - FPS: %d", currentFramerate);
    }
    else if (firstLine.startsWith("PAUSE"))
    {
        snprintf(headers, sizeof(headers),
                 "CSeq: %d\r\n"
                 "Session: %s\r\n"
                 "Server: " RTSP_SERVER_NAME "\r\n",
                 cseq, sessionId.c_str());
        sendRTSPResponse("200 OK", headers);
        playing = false;
        LOG_INFO("RTSP playback paused");
    }
    else if (firstLine.startsWith("TEARDOWN"))
    {
        snprintf(headers, sizeof(headers),
                 "CSeq: %d\r\n"
                 "Session: %s\r\n"
                 "Server: " RTSP_SERVER_NAME "\r\n",
                 cseq, sessionId.c_str());
        sendRTSPResponse("200 OK", headers);
        playing = false;
        LOG_INFO("RTSP session closed");
    }
    else
    {
        snprintf(headers, sizeof(headers), "CSeq: %d\r\n", cseq);
        sendRTSPResponse("501 Not Implemented", headers);
    }
}

void RTSPClientSession::sendRTSPResponse(const char *status, const char *headers)
{
    client.print("RTSP/1.0 ");
    client.print(status);
    client.print("\r\n");
    client.print(headers);
    client.print("\r\n");
}

void RTSPClientSession::sendRTPFrame()
{
    // Check configuration according to transport mode
    if (useTcpInterleaved || RTSP_UDP_TCP_FALLBACK == 2)
    {
        sendRTPFrameTCP();
        return;
    }

    if (clientRtpPort == 0)
    {
        LOG_WARN("RTP port not configured, cannot send frame");
        return; // No UDP port configured
    }

    // Update timecodes for this frame
    updateTimecodeForFrame();

    // Optimized capture for RTSP timing
    camera_fb_t *fb = CameraManager::captureForced();
    if (!fb)
    {
        LOG_ERROR("Capture error for RTSP");
        return;
    }
    LOG_DEBUGF("Frame captured successfully - Size: %d bytes, %dx%d", fb->len, fb->width, fb->height);

    // Fragment image with optimized size for UDP (800 bytes max)
    const int MAX_PACKET_SIZE = RTSP_MAX_FRAGMENT_SIZE; // Use optimized config
    const int RTP_HEADER_SIZE = 12;
    const int JPEG_HEADER_SIZE = 8;
    const int MAX_PAYLOAD_SIZE = MAX_PACKET_SIZE - RTP_HEADER_SIZE - JPEG_HEADER_SIZE;

    int offset = 0;
    bool isFirstFragment = true;
    int fragments_sent = 0;
    bool frameSentSuccessfully = true;

    while (offset < fb->len)
    {
        int fragmentSize = fb->len - offset;
        if (fragmentSize > MAX_PAYLOAD_SIZE)
            fragmentSize = MAX_PAYLOAD_SIZE;

        bool isLastFragment = (offset + fragmentSize) >= fb->len;

        // Prepare RTP header
        uint8_t rtpHeader[RTP_HEADER_SIZE + JPEG_HEADER_SIZE];

        // RTP Header (12 bytes) compliant with RTP/JPEG standards
        rtpHeader[0] = 0x80; // Version 2, padding 0, extension 0, CSRC count 0

        // Payload type 26 (JPEG) + marker bit for last fragment
        uint8_t payload_flags = 0x1A; // Payload type 26 (JPEG) - CORRECT for MJPEG
        if (isLastFragment)
        {
            payload_flags |= 0x80; // Marker bit for last fragment
        }
        rtpHeader[1] = payload_flags;

        // Validate payload type
        if ((payload_flags & 0x7F) != 26)
        {
            Logger::errorf("Invalid RTP payload type - expected 26 (MJPEG), got %d", payload_flags & 0x7F);
        }
        rtpHeader[2] = (sequenceNumber >> 8) & 0xFF;
        rtpHeader[3] = sequenceNumber & 0xFF;
        // Use advanced PTS timecode
        uint32_t rtpTimestamp = currentTimecode.pts;
        rtpHeader[4] = (rtpTimestamp >> 24) & 0xFF;
        rtpHeader[5] = (rtpTimestamp >> 16) & 0xFF;
        rtpHeader[6] = (rtpTimestamp >> 8) & 0xFF;
        rtpHeader[7] = rtpTimestamp & 0xFF;
        // SSRC with synchronization metadata
        rtpHeader[8] = 0x13; // SSRC identifier
        rtpHeader[9] = 0xf9;
        rtpHeader[10] = 0x7e;
        rtpHeader[11] = 0x67;

        // JPEG Header (8 bytes) compliant with RTP/JPEG standards
        rtpHeader[12] = 0x00;                  // Type specific
        rtpHeader[13] = (offset >> 16) & 0xFF; // Fragment offset (3 bytes)
        rtpHeader[14] = (offset >> 8) & 0xFF;
        rtpHeader[15] = offset & 0xFF;
        rtpHeader[16] = 0x00;                             // Type (0 = 4:2:2, 1 = 4:2:0)
        rtpHeader[17] = RTSP_MJPEG_COMPATIBILITY_QUALITY; // Configured quality factor
        rtpHeader[18] = fb->width / 8;                    // Width in 8-pixel units
        rtpHeader[19] = fb->height / 8;                   // Height in 8-pixel units

        // Add keyframe information for HLS compatibility (UDP)
        if (isFirstFragment)
        {
            // Signal that this is a keyframe (every MJPEG frame is a keyframe)
            rtpHeader[12] |= 0x80; // Set keyframe bit in type specific field
        }

        // Send RTP packet with improved robust error handling
        bool packetSent = false;
        int retryCount = 0;

        while (!packetSent && retryCount < RTSP_UDP_MAX_RETRIES)
        {
            // Check UDP connection before sending
            if (!udp.beginPacket(clientRtpIp, clientRtpPort))
            {
                LOG_WARNF("UDP beginPacket error (attempt %d/%d) - buffer full or invalid port",
                          retryCount + 1, RTSP_UDP_MAX_RETRIES);
                retryCount++;
                delay(RTSP_UDP_RETRY_DELAY);

                // If too many beginPacket errors, try to reset UDP
                if (retryCount >= RTSP_UDP_MAX_RETRIES / 2)
                {
                    LOG_WARN("Too many beginPacket errors - UDP reset");
                    resetUDPConnection();
                }
                continue;
            }

            // Write data with verification
            size_t headerWritten = udp.write(rtpHeader, RTP_HEADER_SIZE + JPEG_HEADER_SIZE);
            size_t dataWritten = udp.write(fb->buf + offset, fragmentSize);

            if (headerWritten != (RTP_HEADER_SIZE + JPEG_HEADER_SIZE) || dataWritten != fragmentSize)
            {
                LOG_WARNF("UDP write error (attempt %d) - partial data", retryCount + 1);
                retryCount++;
                delay(RTSP_UDP_RETRY_DELAY);
                continue;
            }

            // Send packet with timeout
            if (!udp.endPacket())
            {
                LOG_WARNF("UDP endPacket error (attempt %d/%d) - packet lost or client disconnected",
                          retryCount + 1, RTSP_UDP_MAX_RETRIES);
                retryCount++;

                // If last attempt and UDP fails, try TCP
                if (retryCount >= RTSP_UDP_MAX_RETRIES && client.connected() && RTSP_UDP_TCP_FALLBACK >= 1)
                {
                    LOG_INFO("Fallback to TCP interleaved after repeated UDP errors");
                    useTcpInterleaved = true;
                    rtpChannel = 0;
                    rtcpChannel = 1;
                    frameSentSuccessfully = false;
                    break; // Exit retry loop
                }

                delay(RTSP_UDP_RETRY_DELAY * 2); // Longer delay before retry
                continue;
            }

            packetSent = true;
        }

        if (!packetSent)
        {
            LOG_ERROR("Unable to send RTP packet after all attempts");
            udpErrorCount++;
            lastUdpErrorTime = millis();
            frameSentSuccessfully = false;

            // If too many consecutive UDP errors, reset connection
            if (udpErrorCount >= RTSP_UDP_RESET_THRESHOLD && RTSP_UDP_AUTO_RESET)
            {
                LOG_WARN("Too many consecutive UDP errors - automatic reset");
                resetUDPConnection();
                udpErrorCount = 0;
                delay(RTSP_UDP_RESET_DELAY);
            }

            break; // Exit loop if all attempts fail
        }
        else
        {
            // Reset error counter if send succeeds
            if (udpErrorCount > 0)
            {
                udpErrorCount = (udpErrorCount - 1 > 0) ? (udpErrorCount - 1) : 0; // Decrement progressively
            }
        }

        fragments_sent++;
        offset += fragmentSize;
        // Sequence number is incremented per packet (RTP standard)
        sequenceNumber++;

        // Validate sequence number increment
        if (sequenceNumber == 0) // Overflow protection
        {
            LOG_WARN("RTP sequence number overflow - resetting to 1");
            sequenceNumber = 1;
        }

        // Buffer management and delay between fragments to avoid overload
        if (!isLastFragment)
        {
            // Minimal delay for better timing - only if errors detected
            if (udpErrorCount > 0 && fragments_sent % 3 == 0)
            {
                delay(RTSP_UDP_FRAGMENT_DELAY);
            }
            else
            {
                yield(); // Allow other tasks to execute
            }
        }
    }

    // CRITICAL: Release frame buffer to prevent memory leaks
    CameraManager::releaseFrame(fb);

    // If UDP failed and TCP fallback is enabled, send via TCP
    if (!frameSentSuccessfully && (useTcpInterleaved || RTSP_UDP_TCP_FALLBACK >= 1) && client.connected())
    {
        LOG_INFO("Sending frame via TCP after UDP failure");
        sendRTPFrameTCP();
    }
    else if (frameSentSuccessfully)
    {
        LOG_DEBUGF("UDP frame sent successfully - Fragments: %d, Sequence: %d, Timestamp: %lu, Frame: %lu",
                   fragments_sent, sequenceNumber, currentTimecode.pts, timecodeManager.getFrameCounter());
    }

    esp_camera_fb_return(fb);
    // Timestamp is managed by TimecodeManager, no manual increment needed
}

void RTSPClientSession::sendRTPFrameTCP()
{
    // Update timecodes for this frame (CRITICAL for TCP mode)
    updateTimecodeForFrame();

    // In TCP mode, use forced capture without timing restrictions
    camera_fb_t *fb = CameraManager::captureForced();
    if (!fb)
    {
        LOG_ERROR("Forced capture error for RTSP TCP");
        return;
    }

    LOG_DEBUGF("TCP frame captured - Size: %d bytes, Width: %d, Height: %d", fb->len, fb->width, fb->height);

    // Fragment image if necessary (max ~1400 bytes per TCP packet)
    const int MAX_PACKET_SIZE = 1400;
    const int RTP_HEADER_SIZE = 12;
    const int JPEG_HEADER_SIZE = 8;
    const int MAX_PAYLOAD_SIZE = MAX_PACKET_SIZE - RTP_HEADER_SIZE - JPEG_HEADER_SIZE;

    int offset = 0;
    bool isFirstFragment = true;
    int fragments_sent = 0;

    while (offset < fb->len)
    {
        int fragmentSize = fb->len - offset;
        if (fragmentSize > MAX_PAYLOAD_SIZE)
            fragmentSize = MAX_PAYLOAD_SIZE;

        bool isLastFragment = (offset + fragmentSize) >= fb->len;

        // Prepare RTP header
        uint8_t rtpHeader[RTP_HEADER_SIZE + JPEG_HEADER_SIZE];

        // RTP Header (12 bytes) compliant with RTP/JPEG standards
        rtpHeader[0] = 0x80; // Version 2, padding 0, extension 0, CSRC count 0

        // Payload type 26 (JPEG) + marker bit for last fragment
        uint8_t payload_flags = 0x1A; // Payload type 26 (JPEG) - CORRECT for MJPEG
        if (isLastFragment)
        {
            payload_flags |= 0x80; // Marker bit for last fragment only
        }
        rtpHeader[1] = payload_flags;

        // Validate payload type for TCP mode
        if ((payload_flags & 0x7F) != 26)
        {
            Logger::errorf("Invalid RTP payload type (TCP) - expected 26 (MJPEG), got %d", payload_flags & 0x7F);
        }
        rtpHeader[2] = (sequenceNumber >> 8) & 0xFF;
        rtpHeader[3] = sequenceNumber & 0xFF;
        // Use advanced PTS timecode
        uint32_t rtpTimestamp = currentTimecode.pts;
        rtpHeader[4] = (rtpTimestamp >> 24) & 0xFF;
        rtpHeader[5] = (rtpTimestamp >> 16) & 0xFF;
        rtpHeader[6] = (rtpTimestamp >> 8) & 0xFF;
        rtpHeader[7] = rtpTimestamp & 0xFF;
        // SSRC with synchronization metadata
        rtpHeader[8] = 0x13; // SSRC identifier
        rtpHeader[9] = 0xf9;
        rtpHeader[10] = 0x7e;
        rtpHeader[11] = 0x67;

        // JPEG Header (8 bytes) compliant with RTP/JPEG standards
        rtpHeader[12] = 0x00;                  // Type specific
        rtpHeader[13] = (offset >> 16) & 0xFF; // Fragment offset (3 bytes)
        rtpHeader[14] = (offset >> 8) & 0xFF;
        rtpHeader[15] = offset & 0xFF;
        rtpHeader[16] = 0x00;                             // Type (0 = 4:2:2, 1 = 4:2:0)
        rtpHeader[17] = RTSP_MJPEG_COMPATIBILITY_QUALITY; // Configured quality factor
        rtpHeader[18] = fb->width / 8;                    // Width in 8-pixel units
        rtpHeader[19] = fb->height / 8;                   // Height in 8-pixel units

        // Add keyframe information for HLS compatibility (TCP)
        if (isFirstFragment)
        {
            // Signal that this is a keyframe (every MJPEG frame is a keyframe)
            rtpHeader[12] |= 0x80; // Set keyframe bit in type specific field
        }

        // Send RTP packet via TCP interleaved
        // Format: '$' + channel + length + RTP data
        uint16_t packetLength = RTP_HEADER_SIZE + JPEG_HEADER_SIZE + fragmentSize;

        // TCP interleaved header (4 bytes)
        uint8_t tcpHeader[4];
        tcpHeader[0] = '$';                        // TCP interleaved marker
        tcpHeader[1] = rtpChannel;                 // RTP channel
        tcpHeader[2] = (packetLength >> 8) & 0xFF; // High length
        tcpHeader[3] = packetLength & 0xFF;        // Low length

        // Send TCP header
        if (client.write(tcpHeader, 4) != 4)
        {
            LOG_WARN("TCP interleaved header send error");
            break;
        }

        // Send RTP data
        if (client.write(rtpHeader, RTP_HEADER_SIZE + JPEG_HEADER_SIZE) != RTP_HEADER_SIZE + JPEG_HEADER_SIZE)
        {
            LOG_WARN("RTP header send error via TCP");
            break;
        }

        if (client.write(fb->buf + offset, fragmentSize) != fragmentSize)
        {
            LOG_WARN("RTP data send error via TCP");
            // Don't break immediately, try to continue
            if (!client.connected())
            {
                LOG_ERROR("TCP client disconnected during send");
                break;
            }
            // Small delay before retry
            delay(1);
            continue;
        }

        fragments_sent++;
        offset += fragmentSize;
        sequenceNumber++; // Increment sequence number by 1 per packet (RTP standard)

        // Validate sequence number increment for TCP mode
        if (sequenceNumber == 0) // Overflow protection
        {
            LOG_WARN("RTP sequence number overflow (TCP) - resetting to 1");
            sequenceNumber = 1;
        }

        // Small delay between fragments to avoid overload (non-blocking)
        if (!isLastFragment)
        {
            yield(); // Allow other tasks to execute
        }
    }

    LOG_DEBUGF("TCP frame sent - Fragments: %d, Sequence: %d, Timestamp: %lu, Frame: %lu",
               fragments_sent, sequenceNumber, currentTimecode.pts, timecodeManager.getFrameCounter());

    // CRITICAL: Release frame buffer to prevent memory leaks
    CameraManager::releaseFrame(fb);
    // Timestamp is managed by TimecodeManager, no manual increment needed
}

String RTSPClientSession::generateSessionId()
{
    static int sessionCounter = 0;
    sessionCounter++;
    return String(sessionCounter) + String(millis());
}

// ===== NEW METHODS FOR ADVANCED TIMECODES =====

void RTSPClientSession::generateAdvancedSDP(String &sdp)
{
    // Complete SDP compliant with RTSP standards
    sdp = "v=0\r\n";
    sdp += "o=- " + String(timecodeManager.getWallClockMs()) + " " + String(timecodeManager.getWallClockMs()) + " IN IP4 " + WiFi.localIP().toString() + "\r\n";
    sdp += "s=ESP32CAM-RTSP-Multi Stream\r\n";
    sdp += "i=ESP32CAM MJPEG Stream compliant with RTSP\r\n";
    sdp += "c=IN IP4 " + WiFi.localIP().toString() + "\r\n";
    sdp += "t=0 0\r\n";
    sdp += "a=control:*\r\n";

    // RTSP session metadata
    sdp += "a=type:broadcast\r\n";
    sdp += "a=range:npt=0-\r\n";

    // Video stream information with CORRECT framerate
    sdp += "m=video 0 RTP/AVP 26\r\n";
    sdp += "a=rtpmap:26 JPEG/" + String(RTSP_CLOCK_RATE) + "\r\n";
    sdp += "a=control:" RTSP_PATH "\r\n";
    sdp += "a=framerate:" + String(RTSP_SDP_FRAMERATE) + "\r\n";
    sdp += "a=framerate:15.0\r\n"; // Explicit framerate for compatibility

    // Add clock metadata if enabled
    if (RTSP_ENABLE_CLOCK_METADATA)
    {
        addClockMetadataToSDP(sdp);
    }

    // Add MJPEG metadata if enabled
    if (RTSP_ENABLE_MJPEG_METADATA)
    {
        // Use default dimensions for SDP
        addMJPEGMetadataToSDP(sdp, 800, 600);
    }

    LOG_DEBUG("Complete RTSP-compliant SDP generated");
}

void RTSPClientSession::addClockMetadataToSDP(String &sdp)
{
    RTSPClockMetadata_t clockMeta = timecodeManager.getClockMetadata();

    // Add clock metadata
    sdp += "a=clock:" + String(clockMeta.rtp_timestamp) + "\r\n";
    sdp += "a=wallclock:" + String(clockMeta.wall_clock_ms) + "\r\n";

    if (clockMeta.clock_sync_status == RTSP_CLOCK_SYNC_OK)
    {
        sdp += "a=ntp:" + String(clockMeta.ntp_timestamp) + "\r\n";
        sdp += "a=clock-sync:1\r\n";
    }
    else
    {
        sdp += "a=clock-sync:0\r\n";
    }

    sdp += "a=timecode-mode:" + String(clockMeta.timecode_mode) + "\r\n";
}

void RTSPClientSession::addMJPEGMetadataToSDP(String &sdp, uint16_t width, uint16_t height)
{
    RTSPMJPEGMetadata_t mjpegMeta = timecodeManager.getMJPEGMetadata(width, height);

    // Add MJPEG metadata
    sdp += "a=quality:" + String(mjpegMeta.quality_factor) + "\r\n";
    sdp += "a=width:" + String(mjpegMeta.width) + "\r\n";
    sdp += "a=height:" + String(mjpegMeta.height) + "\r\n";
    sdp += "a=precision:" + String(mjpegMeta.precision) + "\r\n";

    if (mjpegMeta.fragmentation_info)
    {
        sdp += "a=fragmentation:1\r\n";
        sdp += "a=max-fragment-size:" + String(RTSP_MAX_FRAGMENT_SIZE) + "\r\n";
    }

    // MJPEG specific information
    sdp += "a=mjpeg:1\r\n";
    sdp += "a=keyframe-only:1\r\n"; // MJPEG = 100% keyframes

    // Keyframe signaling according to RTSP standards
    if (RTSP_SIGNAL_KEYFRAMES_IN_SDP)
    {
        sdp += "a=keyframe-interval:" + String(RTSP_KEYFRAME_INTERVAL) + "\r\n";
    }

    // HLS compatibility metadata
    if (RTSP_ENABLE_HLS_COMPATIBILITY)
    {
        sdp += "a=segment-duration:" + String(RTSP_HLS_SEGMENT_DURATION) + "\r\n"; // Configurable segment duration
        sdp += "a=segment-type:keyframe\r\n";                                      // Keyframe-based segmentation
        sdp += "a=gop-size:" + String(RTSP_HLS_GOP_SIZE) + "\r\n";                 // Configurable GOP size
        sdp += "a=closed-gop:" + String(RTSP_HLS_CLOSED_GOP) + "\r\n";             // Configurable closed GOP
    }

    // Video compatibility metadata
    if (RTSP_ENABLE_VIDEO_COMPATIBILITY_METADATA)
    {
        sdp += "a=video-compatibility:1\r\n";
        sdp += "a=mjpeg-quality:" + String(RTSP_MJPEG_COMPATIBILITY_QUALITY) + "\r\n";

        if (RTSP_MJPEG_PROFILE_BASELINE)
        {
            sdp += "a=mjpeg-profile:baseline\r\n";
        }
    }

    // Detailed codec information
    if (RTSP_ENABLE_CODEC_INFO)
    {
        sdp += "a=codec:mjpeg\r\n";
        sdp += "a=codec-version:1.0\r\n";
        sdp += "a=codec-profile:baseline\r\n";
        sdp += "a=codec-level:1\r\n";
    }

    // Timing information for compatibility
    sdp += "a=frame-duration:" + String(1000 / RTSP_FPS) + "ms\r\n";
    sdp += "a=clock-rate:" + String(RTSP_CLOCK_RATE) + "\r\n";

    // HLS-specific metadata for better compatibility
    addHLSMetadataToSDP(sdp);
}

void RTSPClientSession::addHLSMetadataToSDP(String &sdp)
{
    // Only add HLS metadata if enabled
    if (!RTSP_ENABLE_HLS_COMPATIBILITY)
    {
        return;
    }

    // HLS-specific metadata for better compatibility with FFmpeg
    sdp += "a=hls-version:3\r\n";                                                  // HLS version 3
    sdp += "a=hls-segment-duration:" + String(RTSP_HLS_SEGMENT_DURATION) + "\r\n"; // Configurable segment duration
    sdp += "a=hls-playlist-type:VOD\r\n";                                          // Video on Demand
    sdp += "a=hls-target-duration:" + String(RTSP_HLS_SEGMENT_DURATION) + "\r\n";  // Target segment duration
    sdp += "a=hls-allow-cache:1\r\n";                                              // Allow caching

    // Keyframe information for HLS
    sdp += "a=hls-keyframe-interval:" + String(RTSP_KEYFRAME_INTERVAL) + "\r\n"; // Configurable keyframe interval
    sdp += "a=hls-gop-size:" + String(RTSP_HLS_GOP_SIZE) + "\r\n";               // Configurable GOP size
    sdp += "a=hls-closed-gop:" + String(RTSP_HLS_CLOSED_GOP) + "\r\n";           // Configurable closed GOP

    // Stream information
    sdp += "a=hls-stream-type:video\r\n";
    sdp += "a=hls-codec:mjpeg\r\n";
    sdp += "a=hls-framerate:" + String(RTSP_FPS) + "\r\n"; // Use configured framerate
    sdp += "a=hls-resolution:800x600\r\n";

    // FFmpeg compatibility
    sdp += "a=ffmpeg-compatible:1\r\n";
    sdp += "a=ffmpeg-keyframe-mode:all\r\n"; // All frames are keyframes
    sdp += "a=ffmpeg-gop-mode:closed\r\n";   // Closed GOP mode
}

void RTSPClientSession::updateTimecodeForFrame()
{
    // Generate new timecode for current frame
    currentTimecode = timecodeManager.generateTimecode();

    // Update RTP timestamp - use PTS directly
    timestamp = currentTimecode.pts;

    LOG_DEBUGF("Timecode updated - PTS: %lu, DTS: %lu, Frame: %lu", currentTimecode.pts, currentTimecode.dts, timecodeManager.getFrameCounter());
}
