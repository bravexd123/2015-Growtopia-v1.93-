#pragma once
#include <algorithm>
#include <fstream>
#include <fmt/core.h>
#include <Event/EventType.hpp>
#include <Event/EventPool.hpp>
#include <Server/Server.hpp>
#include <World/World.hpp>
#include <World/WorldPool.hpp>
#include <Packet/VariantFunction.hpp>
#include <Packet/PacketFactory.hpp>
#include <ENetWrapper/ENetWrapper.hpp>
#include <Utils/MiscUtils.hpp>
#include <Utils/BinaryWriter.hpp>
#include <Player/PlayerItems.hpp>
#include <Logger/Logger.hpp>
#include <Server/ServerPool.hpp>
#include <Event/Dialogs/LockHelpers.hpp>
#include <Manager/Item/ItemEffects.hpp>
#include <Manager/Item/Emoticons.hpp>

inline std::string BuildCountryState(Player* p) {
    return p->GetDetail().GetCountryCode() + (p->GetLevel() >= Player::kMaxLevel ? "|maxLevel" : "");
}

ACTION_EVENT("join_request", OnJoinRequest) {
    if (!pAvatar->GetDetail().IsFlagOn(CLIENTFLAG_LOGGED_ON))
        return;
    if (pAvatar->GetDetail().IsFlagOn(CLIENTFLAG_IS_IN))
        return;

    std::string worldName = eventParser.Get("name", 1);
    if (worldName.empty())
        worldName = "START";
    Utils::ToUpperCase(worldName);

    if (worldName == "EXIT") {
        VarList::OnFailedToEnterWorld(pAvatar->Get(), 1);
        return;
    }

    auto pWorld = pServer->GetWorldPool()->NewWorld(worldName);
    if (!pWorld) {
        VarList::OnFailedToEnterWorld(pAvatar->Get(), 1);
        return;
    }

    if (pWorld->IsUserBanned(pAvatar->GetUserId())) {
        VarList::OnConsoleMessage(pAvatar->Get(), fmt::format("`4You are banned from `w{}``.", pWorld->GetName()));
        VarList::OnFailedToEnterWorld(pAvatar->Get(), 1);
        return;
    }

    pWorld->AddPlayer(pAvatar);
    pAvatar->SetWorld(pWorld);
    pAvatar->SetNetId(pWorld->AssignNetId(pAvatar));

    uint32_t doorX = 0, doorY = 0;
    float spawnX, spawnY;
    if (pWorld->FindDoorTile(doorX, doorY)) {
        spawnX = static_cast<float>(doorX) * 32.0f;
        spawnY = static_cast<float>(doorY) * 32.0f;
    } else {
        uint32_t surfaceTile = pWorld->GetHeight() * 4 / 10;
        spawnX = (pWorld->GetWidth() / 2) * 32.0f;
        spawnY = (surfaceTile >= 2 ? surfaceTile - 2 : 0) * 32.0f;
    }
    pAvatar->SetPosition(spawnX, spawnY);
    pAvatar->GetDetail().SetFlag(CLIENTFLAG_IS_IN);

    pAvatar->ClearCheckpoint();

    pAvatar->PushRecentWorld(pWorld->GetName());
    GetDatabase()->GetPlayerTable()->Save(pAvatar);

    Logger::Print(INFO, "Player {} joined world '{}' (netId={})", pAvatar->GetRawName(), pWorld->GetName(), pAvatar->GetNetId());

    auto mapData = pWorld->SerializeMapData();
    auto mapPacket = SMapDataPacket(mapData);
    ENetWrapper::SendPacket(pAvatar->Get(), mapPacket);
    Logger::Print(INFO, "Sent SEND_MAP_DATA ({} bytes) to player {}", mapData.size(), pAvatar->GetRawName());

    VarList::OnSpawn(pAvatar->Get(), pAvatar->GetSpawnData(true));
    Logger::Print(INFO, "Sent OnSpawn to player {}", pAvatar->GetRawName());

    for (auto* pOther : pWorld->GetPlayers()) {
        if (pOther == pAvatar)
            continue;
        VarList::OnSpawn(pOther->Get(), pAvatar->GetSpawnData(false));
        VarList::OnSpawn(pAvatar->Get(), pOther->GetSpawnData(false));

        VarList::OnCountryState(pOther->Get(), pAvatar->GetNetId(), BuildCountryState(pAvatar));
        VarList::OnCountryState(pAvatar->Get(), pOther->GetNetId(), BuildCountryState(pOther));
    }
    Logger::Print(INFO, "Cross-announced player {} with {} other player(s) already in '{}'", pAvatar->GetRawName(), pWorld->GetPlayers().size() - 1, pWorld->GetName());

    VarList::OnConsoleMessage(pAvatar->Get(), fmt::format(
        "`oWorld `w{}`````` `oentered.  There are `w{}`` `oother people here, `w{}`o online.``",
        pWorld->GetName(), pWorld->GetPlayers().size() - 1, pServer->GetActivePlayers()));

    pWorld->RenumberDroppedItemsForJoin();
    for (const auto& drop : pWorld->GetDroppedItems()) {
        TankPacketData t{};
        t.m_type = NET_GAME_PACKET_ITEM_CHANGE_OBJECT;
        t.m_netId = -1;
        t.m_item = -1;
        t.m_floatVariable = static_cast<float>(drop.m_count);
        t.m_mainData = drop.m_itemId;
        t.m_vectorX = drop.m_x;
        t.m_vectorY = drop.m_y;
        STankPacket packet(t);
        ENetWrapper::SendPacket(pAvatar->Get(), packet);
    }
    Logger::Print(INFO, "Replayed {} existing dropped item(s) to player {}", pWorld->GetDroppedItems().size(), pAvatar->GetRawName());

    for (const auto& lock : pWorld->GetTileLocks())
        LockHelpers::SendLockStateBroadcast(pWorld, lock);
    if (!pWorld->GetTileLocks().empty())
        Logger::Print(INFO, "Sent {} existing lock boundary(s) to player {}", pWorld->GetTileLocks().size(), pAvatar->GetRawName());

    ENetPeer* joinPeer = pAvatar->Get();
    std::string joinWorldName = pWorld->GetName();
    GetServerPool()->ScheduleDelayed(150, [pServer, joinPeer, joinWorldName]() {
        auto playerPool = pServer->GetPlayerPool();
        if (!playerPool)
            return;
        Player* pAvatar = playerPool->GetPlayerByPeer(joinPeer);
        if (!pAvatar)
            return;
        auto pWorld = pAvatar->GetWorld();
        if (!pWorld || pWorld->GetName() != joinWorldName)
            return;

        VarList::OnSetClothing(pAvatar->Get(), pAvatar->GetNetId(), std::array<uint16_t, NUM_BODY_PARTS>{}, 0);
        VarList::OnSetClothing(pAvatar->Get(), pAvatar->GetNetId(), pAvatar->GetItems()->GetClothes(), static_cast<int32_t>(pAvatar->GetItems()->GetSkinColor().GetInt()));
        Logger::Print(INFO, "Sent OnSetClothing to player {}", pAvatar->GetRawName());

        for (auto* pOther : pWorld->GetPlayers()) {
            if (pOther == pAvatar)
                continue;
            VarList::OnSetClothing(pOther->Get(), pAvatar->GetNetId(), pAvatar->GetItems()->GetClothes(), static_cast<int32_t>(pAvatar->GetItems()->GetSkinColor().GetInt()));
            VarList::OnSetClothing(pAvatar->Get(), pOther->GetNetId(), pOther->GetItems()->GetClothes(), static_cast<int32_t>(pOther->GetItems()->GetSkinColor().GetInt()));
        }
        Logger::Print(INFO, "Sent cross-clothing for {} other player(s) to/about {}", pWorld->GetPlayers().size() - 1, pAvatar->GetRawName());

        VarList::OnSetBux(pAvatar->Get(), pAvatar->GetItems()->GetGems());
        Logger::Print(INFO, "Sent OnSetBux ({} gems) to player {}", pAvatar->GetItems()->GetGems(), pAvatar->GetRawName());

        VarList::OnSetCurrentWeather(pAvatar->Get(), pWorld->GetWeather());

        VarList::OnEmoticonDataChanged(pAvatar->Get(), kEmoticonVersion, BuildEmoticonData());

        VarList::OnCountryState(pAvatar->Get(), pAvatar->GetNetId(), BuildCountryState(pAvatar));

        uint32_t effectFlags = ItemEffects::ComputeCharacterStateFlags(pAvatar);
        for (int i = 0; i < 2; i++) {
            TankPacketData t{};
            t.m_type = NET_GAME_PACKET_SET_CHARACTER_STATE;
            t.m_netId = pAvatar->GetNetId();
            t.m_jumpCount = 128;

            t.m_punchIndex = ItemEffects::GetActivePunchEffect(pAvatar->GetItems());
            t.m_animationType = 128;
            t.m_floatVariable = 150.0f;
            t.m_vectorX = 1000.0f;
            t.m_vectorY = 350.0f;
            t.m_vectorX2 = 200.0f;
            t.m_vectorY2 = 1000.0f;
            t.m_effectFlags = static_cast<int32_t>(effectFlags);
            STankPacket packet(t);
            ENetWrapper::SendPacket(pAvatar->Get(), packet);
        }

        {
            auto* pItems = pAvatar->GetItems();
            BinaryWriter iw(9 + pItems->m_bpItems.size() * 4);
            iw.Write<uint8_t>(0);
            iw.Write<uint32_t>(pItems->m_backpackSpace);
            iw.Write<uint8_t>(static_cast<uint8_t>(pItems->m_bpItems.size()));
            for (const auto& [itemId, count] : pItems->m_bpItems) {
                iw.Write<uint16_t>(itemId);
                iw.Write<uint8_t>(count);
                iw.Write<uint8_t>(0);
            }
            std::vector<uint8_t> invData(iw.Get(), iw.Get() + iw.GetPosition());

            TankPacketData t{};
            t.m_type = NET_GAME_PACKET_SEND_INVENTORY_STATE;
            t.m_netId = 0;

            SExtendedTankPacket packet(t, invData);
            ENetWrapper::SendPacket(pAvatar->Get(), packet);
        }
        Logger::Print(INFO, "Sent inventory state to player {}", pAvatar->GetRawName());

        {
            TankPacketData t{};
            t.m_type = NET_GAME_PACKET_PING_REQUEST;
            t.m_netId = 0;
            STankPacket packet(t);
            ENetWrapper::SendPacket(pAvatar->Get(), packet);
        }
    });

}
