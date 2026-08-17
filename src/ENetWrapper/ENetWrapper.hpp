#pragma once
#if defined(_WIN32) || defined(WIN32)
#pragma comment(lib, "winmm.lib")
#endif
#include <thread>
#include <mutex>
#include <string>
#include <enet/enet.h>
#include <fmt/core.h>
#include <ENetWrapper/Peer.hpp>
#include <ENetWrapper/ENetServer.hpp>
#include <Packet/PacketFactory.hpp>
#include <Packet/VariantList.hpp>
#include <Player/Player.hpp>
#include <Logger/Logger.hpp>
namespace ENetWrapper {

    inline std::mutex g_enetMutex;

    inline void SendPacket(ENetPeer* peer, ISPacket& data, bool verbose = true) {
        if (!peer) {
            Logger::Print(WARNING, "SendPacket: peer is null, aborting send");
            return;
        }
        std::lock_guard<std::mutex> lock(g_enetMutex);
        auto packetData = data.m_packetData.data();
        auto dataLength = data.m_packetLength;
        ENetPacket* packet = enet_packet_create(nullptr, 5 + dataLength, ENET_PACKET_FLAG_RELIABLE);
        if (!packet) {
            Logger::Print(WARNING, "SendPacket: enet_packet_create returned null for type={}, dataLength={}", data.m_packetType, dataLength);
            return;
        }
        std::memcpy(packet->data, &data.m_packetType, 4);
        packet->data[dataLength + 4] = 0;

        if (data.m_packetType != NET_MESSAGE_SERVER_HELLO && dataLength > 0)
            std::memcpy(packet->data + 4, packetData, dataLength);

        if (verbose) {
            std::string hex;
            size_t dumpLen = std::min<size_t>(400, packet->dataLength);
            for (size_t i = 0; i < dumpLen; i++)
                hex += fmt::format("{:02X} ", packet->data[i]);
            Logger::Print(INFO, "SendPacket: outgoing bytes ({}/{} shown): {}", dumpLen, packet->dataLength, hex);
        }

        int sendResult = enet_peer_send(peer, 0, packet);
        if (verbose)
            Logger::Print(INFO, "SendPacket: type={}, dataLength={}, enet_peer_send result={}, peer state={}", data.m_packetType, dataLength, sendResult, static_cast<int>(peer->state));
        if (sendResult != 0) {
            Logger::Print(WARNING, "SendPacket: enet_peer_send FAILED with code {}", sendResult);
            enet_packet_destroy(packet);
        }
    }
    inline void SendPacket(ENetPeer* peer, eNetMessageType packetType, const void* pData, uintmax_t dataLength) {
        if (!peer) {
            Logger::Print(WARNING, "SendPacket(raw): peer is null, aborting send");
            return;
        }
        std::lock_guard<std::mutex> lock(g_enetMutex);
        ENetPacket* packet = enet_packet_create(nullptr, 5 + dataLength, ENET_PACKET_FLAG_RELIABLE);
        if (!packet) {
            Logger::Print(WARNING, "SendPacket(raw): enet_packet_create returned null for type={}, dataLength={}", static_cast<int>(packetType), dataLength);
            return;
        }
        std::memcpy(packet->data, &packetType, 4);
        packet->data[dataLength + 4] = 0;
        if (pData)
            std::memcpy(packet->data + 4, pData, dataLength);
        int sendResult = enet_peer_send(peer, 0, packet);
        Logger::Print(INFO, "SendPacket(raw): type={}, dataLength={}, enet_peer_send result={}, peer state={}", static_cast<int>(packetType), dataLength, sendResult, static_cast<int>(peer->state));
        if (sendResult != 0) {
            Logger::Print(WARNING, "SendPacket(raw): enet_peer_send FAILED with code {}", sendResult);
            enet_packet_destroy(packet);
        }
    }
    inline void SendPacket(ENetPeer* peer, std::string data) {
        auto vPacket = STextPacket(data);
        ENetWrapper::SendPacket(peer, vPacket);
    }
    inline void SendVariantList(ENetPeer* peer, VariantList vList) {
        auto vPacket = SVariantPacket(vList);
        ENetWrapper::SendPacket(peer, vPacket);
    }
}
