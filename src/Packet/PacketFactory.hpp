#pragma once
#include <cstdint>
#include <vector>
#include <Packet/GameUpdatePacket.hpp>
#include <Packet/VariantList.hpp>
#include <Utils/BinaryWriter.hpp>

class IPacketType {
public:
    uint32_t m_packetType;
};

class ISPacket : public IPacketType {
public:
    std::vector<uint8_t>    m_packetData = {};
    std::size_t             m_packetLength = 0;
};

class STankPacket : public ISPacket {
public:
    STankPacket(TankPacketData tankData) : m_tankData(std::move(tankData)) {
        m_packetType = NET_MESSAGE_GAME_PACKET;
        this->Pack();
    }

public:
    void Pack() {
        m_packetData.resize(sizeof(TankPacketData) + m_tankData.m_dataLength);
        m_packetLength = sizeof(TankPacketData) + m_tankData.m_dataLength;

        std::memcpy(m_packetData.data(), &m_tankData, sizeof(TankPacketData));
        if (m_tankData.m_dataLength > 0)
            std::memcpy(m_packetData.data() + sizeof(TankPacketData), m_data.data(), m_tankData.m_dataLength);
    }

protected:
    TankPacketData m_tankData;

public:
    std::vector<uint8_t> m_data;
};

class SVariantPacket : public STankPacket {
public:
    SVariantPacket(VariantList vList) : STankPacket(TankPacketData()), m_vList(vList) {
        m_tankData.m_type = NET_GAME_PACKET_CALL_FUNCTION;
        m_tankData.m_flags.bExtended = true;
        m_tankData.m_netId = vList.netId;
        m_tankData.m_delay = vList.executionDelay;
        m_tankData.m_dataLength = vList.GetMemoryUsage();

        auto packedData = vList.Pack();
        m_data.resize(m_tankData.m_dataLength);

        std::memcpy(m_data.data(), packedData, m_tankData.m_dataLength);
        std::free(packedData);
        STankPacket::Pack();
    }

private:
    VariantList m_vList;
};

class SMapDataPacket : public STankPacket {
public:
    SMapDataPacket(std::vector<uint8_t> mapData) : STankPacket(TankPacketData()) {
        m_tankData.m_type = NET_GAME_PACKET_SEND_MAP_DATA;
        m_tankData.m_netId = -1;
        m_tankData.m_flags.bExtended = true;
        m_tankData.m_dataLength = static_cast<uint32_t>(mapData.size());
        m_data = std::move(mapData);
        STankPacket::Pack();
    }
};

class SExtendedTankPacket : public STankPacket {
public:
    SExtendedTankPacket(TankPacketData header, std::vector<uint8_t> data) : STankPacket(TankPacketData()) {
        m_tankData = header;
        m_tankData.m_flags.bExtended = true;
        m_tankData.m_dataLength = static_cast<uint32_t>(data.size());
        m_data = std::move(data);
        STankPacket::Pack();
    }
};

class SLoginInformationRequestPacket : public ISPacket {
public:
    SLoginInformationRequestPacket() {
        m_packetType = NET_MESSAGE_SERVER_HELLO;
    }
};

class STextPacket : public ISPacket {
public:
    STextPacket(std::string data) : m_data(std::move(data)) {
        m_packetType = NET_MESSAGE_GAME_MESSAGE;

        m_packetData.resize(m_data.size());
        m_packetLength = m_data.size();

        std::memcpy(m_packetData.data(), m_data.data(), m_data.size());
    }

private:
    std::string m_data;
};
