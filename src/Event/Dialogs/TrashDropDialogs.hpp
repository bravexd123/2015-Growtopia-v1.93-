#pragma once
#include <algorithm>
#include <cstdlib>
#include <fmt/core.h>
#include <Event/EventType.hpp>
#include <Player/Player.hpp>
#include <World/World.hpp>
#include <Manager/Item/ItemManager.hpp>
#include <Manager/Item/ItemInfo.hpp>
#include <Manager/Database/Database.hpp>
#include <Packet/PacketFactory.hpp>
#include <Packet/TextFunction.hpp>
#include <Packet/VariantFunction.hpp>
#include <ENetWrapper/ENetWrapper.hpp>
#include <Logger/Logger.hpp>

namespace {
    uint8_t ResolveTrashCount(TextParse& eventParser, uint8_t held) {
        std::string raw = eventParser.Get("count", 1);
        int parsed = 0;
        try { parsed = raw.empty() ? 0 : std::stoi(raw); } catch (...) { return 0; }
        if (parsed <= 0)
            return 0;
        return static_cast<uint8_t>(std::min<int>(parsed, held));
    }
    uint8_t ResolveDropCount(TextParse& eventParser, uint8_t held) {
        std::string raw = eventParser.Get("count", 1);
        if (raw.empty())
            return held;
        int parsed = 0;
        try { parsed = std::stoi(raw); } catch (...) { return held; }
        if (parsed <= 0)
            return 0;
        return static_cast<uint8_t>(std::min<int>(parsed, held));
    }

    void SendInventoryLoss(Player* pAvatar, uint16_t itemId, uint8_t lostCount) {
        TankPacketData t{};
        t.m_type = NET_GAME_PACKET_MODIFY_ITEM_INVENTORY;
        t.m_lostItemCount = lostCount;
        t.m_mainData = itemId;
        STankPacket packet(t);
        ENetWrapper::SendPacket(pAvatar->Get(), packet);
    }
}

DIALOG_EVENT("trash_item", OnTrashItemDialog) {
    uint16_t itemId = eventParser.Get<uint16_t>("itemID", 1);
    {

        auto* pCheck = GetItemManager()->GetItem(itemId);
        if (!pCheck || pCheck->IsUntradable())
            return;
    }
    auto* pItems = pAvatar->GetItems();
    auto it = pItems->m_bpItems.find(itemId);
    if (it == pItems->m_bpItems.end())
        return;

    uint8_t count = ResolveTrashCount(eventParser, it->second);
    if (count == 0 || !pItems->RemoveItem(itemId, count))
        return;

    SendInventoryLoss(pAvatar, itemId, count);
    CAction::PlaySFX(pAvatar->Get(), "trash", 0);

    auto* pItem = GetItemManager()->GetItem(itemId);

    if (pAvatar->GetAccountTier() != AccountTier::Default) {
        uint32_t gemsWon = 0;
        for (uint8_t i = 0; i < count; i++)
            gemsWon += static_cast<uint32_t>(std::rand() % 2);
        if (gemsWon > 0) {
            pAvatar->GetItems()->SetGems(pAvatar->GetItems()->GetGems() + static_cast<int32_t>(gemsWon));
            GetDatabase()->GetPlayerTable()->Save(pAvatar);
            VarList::OnSetBux(pAvatar->Get(), pAvatar->GetItems()->GetGems());
        }
        if (pItem)

            VarList::OnConsoleMessage(pAvatar->Get(), fmt::format("{} `w{}`` recycled, `w{}`` gems earned.", count, pItem->m_name, gemsWon));
    } else if (pItem) {
        VarList::OnConsoleMessage(pAvatar->Get(), fmt::format("{} `w{}`` trashed.", count, pItem->m_name));
    }

    Logger::Print(INFO, "Player {} trashed {}x {} (itemId {})", pAvatar->GetRawName(), count, pItem ? pItem->m_name : "?", itemId);
}

DIALOG_EVENT("drop_item", OnDropItemDialog) {
    uint16_t itemId = eventParser.Get<uint16_t>("itemID", 1);
    {

        auto* pCheck = GetItemManager()->GetItem(itemId);
        if (!pCheck || pCheck->IsUntradable())
            return;
    }
    auto* pItems = pAvatar->GetItems();
    auto it = pItems->m_bpItems.find(itemId);
    if (it == pItems->m_bpItems.end())
        return;

    uint8_t count = ResolveDropCount(eventParser, it->second);
    if (count == 0 || !pItems->RemoveItem(itemId, count))
        return;

    SendInventoryLoss(pAvatar, itemId, count);

    auto pWorld = pAvatar->GetWorld();
    if (pWorld) {
        float aheadX = pAvatar->GetX() + (pAvatar->IsFacingLeft() ? -32.0f : 32.0f);
        float x = aheadX + static_cast<float>((std::rand() % 5) - 2);
        float y = pAvatar->GetY() + static_cast<float>((std::rand() % 5) - 2);
        pWorld->SpawnDrop(itemId, count, x, y);

        TankPacketData drop{};
        drop.m_type = NET_GAME_PACKET_ITEM_CHANGE_OBJECT;
        drop.m_netId = -1;
        drop.m_item = -1;
        drop.m_floatVariable = static_cast<float>(count);
        drop.m_mainData = itemId;
        drop.m_vectorX = x;
        drop.m_vectorY = y;
        STankPacket dropPacket(drop);
        pWorld->BroadcastPacket(dropPacket);
    }

    auto* pItem = GetItemManager()->GetItem(itemId);
    Logger::Print(INFO, "Player {} dropped {}x {} (itemId {})", pAvatar->GetRawName(), count, pItem ? pItem->m_name : "?", itemId);
}

DIALOG_EVENT("continue", OnContinueDialog) {
}
