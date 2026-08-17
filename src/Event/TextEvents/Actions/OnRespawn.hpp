#pragma once
#include <Event/EventType.hpp>
#include <World/World.hpp>
#include <Packet/VariantFunction.hpp>
#include <Server/ServerPool.hpp>
#include <Manager/Item/ItemManager.hpp>
#include <Manager/Item/ItemInfo.hpp>
#include <Logger/Logger.hpp>

inline void DoRespawn(EventArguments) {
    auto pWorld = pAvatar->GetWorld();
    if (!pWorld)
        return;

    uint32_t doorX = 0, doorY = 0;
    float spawnX, spawnY;
    bool usedCheckpoint = false;
    if (pAvatar->HasCheckpoint()) {
        auto* pCheckTile = pWorld->GetTile(pAvatar->GetCheckpointX(), pAvatar->GetCheckpointY());
        auto* pCheckItem = pCheckTile ? GetItemManager()->GetItem(pCheckTile->m_foregroundId) : nullptr;
        if (pCheckItem && pCheckItem->m_itemType == ITEMTYPE_CHECKPOINT) {
            spawnX = static_cast<float>(pAvatar->GetCheckpointX()) * 32.0f;
            spawnY = static_cast<float>(pAvatar->GetCheckpointY()) * 32.0f;
            usedCheckpoint = true;
        } else {
            pAvatar->ClearCheckpoint();
        }
    }
    if (!usedCheckpoint) {
        if (pWorld->FindDoorTile(doorX, doorY)) {
            spawnX = static_cast<float>(doorX) * 32.0f;
            spawnY = static_cast<float>(doorY) * 32.0f;
        } else {
            uint32_t surfaceTile = pWorld->GetHeight() * 4 / 10;
            spawnX = (pWorld->GetWidth() / 2) * 32.0f;
            spawnY = (surfaceTile >= 2 ? surfaceTile - 2 : 0) * 32.0f;
        }
    }

    pAvatar->ResetHealth();

    int32_t netId = pAvatar->GetNetId();
    VarList::OnKilled(pAvatar->Get(), netId, 1);
    VarList::OnSetFreezeState(pAvatar->Get(), netId, 2);
    Logger::Print(INFO, "Player {} respawning in world '{}' (killed, frozen, waiting 2s)", pAvatar->GetRawName(), pWorld->GetName());

    ENetPeer* respawnPeer = pAvatar->Get();
    std::string respawnWorldName = pWorld->GetName();
    GetServerPool()->ScheduleDelayed(2000, [pServer, respawnPeer, respawnWorldName, spawnX, spawnY, netId]() {
        auto playerPool = pServer->GetPlayerPool();
        if (!playerPool)
            return;
        Player* pAvatar = playerPool->GetPlayerByPeer(respawnPeer);
        if (!pAvatar)
            return;
        auto pWorld = pAvatar->GetWorld();
        if (!pWorld || pWorld->GetName() != respawnWorldName)
            return;

        pAvatar->SetPosition(spawnX, spawnY);

        VarList::OnSetPos(pAvatar->Get(), netId, spawnX, spawnY);
        VarList::OnSetFreezeState(pAvatar->Get(), netId, 0);
        VarList::OnCountdownEnd(pAvatar->Get(), netId, 0);
        VarList::OnPlayPositioned(pAvatar->Get(), netId, "audio/teleport.wav");

        Logger::Print(INFO, "Player {} respawned in world '{}'", pAvatar->GetRawName(), pWorld->GetName());
    });
}

ACTION_EVENT("respawn", OnRespawn) {
    DoRespawn(pAvatar, pServer, eventData, eventParser, pTankData);
}

ACTION_EVENT("respawn_spike", OnRespawnSpike) {
    Logger::Print(INFO, "Player {} died on a deadly block", pAvatar->GetRawName());
    DoRespawn(pAvatar, pServer, eventData, eventParser, pTankData);
}
