#pragma once
#include <Event/EventType.hpp>
#include <Event/EventPool.hpp>
#include <Packet/PacketFactory.hpp>
#include <ENetWrapper/ENetWrapper.hpp>
#include <Manager/Item/ItemManager.hpp>
#include <Logger/Logger.hpp>

ACTION_EVENT("refresh_item_data", OnRefreshItemData) {
    const std::vector<uint8_t>& raw = GetItemManager()->GetClientItemsData();
    if (raw.empty()) {
        Logger::Print(WARNING, "OnRefreshItemData: items.dat not loaded");
        return;
    }

    TankPacketData t{};
    t.m_type = NET_GAME_PACKET_SEND_ITEM_DATABASE_DATA;

    t.m_netId = -1;

    SExtendedTankPacket packet(t, raw);
    ENetWrapper::SendPacket(pAvatar->Get(), packet);
    Logger::Print(INFO, "Sent items.dat to player {} ({} bytes, raw/uncompressed)", pAvatar->GetRawName(), raw.size());
}
