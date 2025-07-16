/**
 * @file NanoRTSPServer.h
 * @author Jeremy Noverraz
 * @version 1.0
 * @date 2025
 * @brief Multi-client RTSP MJPEG server, modular
 */
// NanoRTSPServer.h
// Multi-client RTSP MJPEG server, modular
#ifndef NANO_RTSP_SERVER_H
#define NANO_RTSP_SERVER_H

#include <WiFi.h>
#include <vector>
#include "RTSPClientSession.h"
#include "../../src/config.h"

/**
 * @class NanoRTSPServer
 * @brief Multi-client RTSP server for streaming MJPEG via RTP.
 *        Manages client acceptance, session creation/deletion and frame distribution.
 */
class NanoRTSPServer
{
public:
    NanoRTSPServer(int port = RTSP_PORT);
    void begin();
    void handleClients();
    bool hasActiveClients() const;

private:
    WiFiServer server;
    int listenPort;
    std::vector<RTSPClientSession *> clients;
    void acceptNewClients();
    void removeDisconnectedClients();
};

#endif // NANO_RTSP_SERVER_H
