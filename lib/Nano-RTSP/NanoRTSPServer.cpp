/**
 * @file NanoRTSPServer.cpp
 * @author Jeremy Noverraz
 * @version 1.0
 * @date 2025
 * @brief Implementation of multi-client RTSP MJPEG server
 */
// NanoRTSPServer.cpp
#include "NanoRTSPServer.h"
#include "RTSPClientSession.h"
#include "../Utils/Logger.h"

NanoRTSPServer::NanoRTSPServer(int port) : server(port), listenPort(port) {}

void NanoRTSPServer::begin()
{
    server.begin();
    LOG_INFOF("RTSP server started on port %d", listenPort);
    LOG_INFO("Waiting for RTSP connections...");
}

void NanoRTSPServer::handleClients()
{
    acceptNewClients();
    removeDisconnectedClients();

    // Handle active clients
    for (auto it = clients.begin(); it != clients.end(); ++it)
    {
        if ((*it)->isConnected())
        {
            (*it)->handle();
        }
    }
}

void NanoRTSPServer::acceptNewClients()
{
    WiFiClient client = server.available();
    if (client)
    {
        LOG_INFO("=== NEW RTSP CONNECTION ===");
        LOG_INFOF("Client connected from: %s", client.remoteIP().toString().c_str());
        LOG_INFOF("Client port: %d", client.remotePort());
        LOG_DEBUG("Creating RTSP session...");

        // Limit number of simultaneous clients
        if (clients.size() >= 5)
        {
            LOG_WARN("Maximum number of clients reached, connection refused");
            client.stop();
            return;
        }

        clients.push_back(new RTSPClientSession(client));
        LOG_INFOF("Total clients: %d", clients.size());
    }
}

void NanoRTSPServer::removeDisconnectedClients()
{
    for (auto it = clients.begin(); it != clients.end();)
    {
        if (!(*it)->isConnected())
        {
            LOG_INFO("RTSP client disconnected");
            delete *it;
            it = clients.erase(it);
            LOG_INFOF("Remaining clients: %d", clients.size());
        }
        else
        {
            ++it;
        }
    }
}

bool NanoRTSPServer::hasActiveClients() const
{
    for (const auto &client : clients)
    {
        if (client && client->isConnected())
        {
            return true;
        }
    }
    return false;
}
