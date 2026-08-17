#pragma once
#include <algorithm>
#include <fmt/core.h>
#include <Event/EventType.hpp>
#include <Player/Player.hpp>
#include <World/World.hpp>
#include <Manager/Database/Database.hpp>
#include <Packet/TextFunction.hpp>
#include <Event/Dialogs/LockHelpers.hpp>
#include <Logger/Logger.hpp>

DIALOG_EVENT("lock_edit", OnLockEditDialog) {
    uint32_t tileX = eventParser.Get<uint32_t>("tilex", 1);
    uint32_t tileY = eventParser.Get<uint32_t>("tiley", 1);
    auto pWorld = pAvatar->GetWorld();
    if (!pWorld)
        return;

    LockInfo* pLock = pWorld->FindLockByAnchor(tileX, tileY);
    if (!pLock)
        return;
    if (!pWorld->CanEdit(tileX, tileY, pAvatar->GetUserId()))
        return;

    pLock->m_isPublic = eventParser.Get<int32_t>("public_lock", 1) != 0;
    pLock->m_ignoreAir = eventParser.Get<int32_t>("ignore_air", 1) != 0;

    if (eventParser.Contain("playerNetID")) {
        std::string netIdStr = eventParser.Get("playerNetID", 1);
        if (!netIdStr.empty()) {
            try {
                int32_t targetNetId = std::stoi(netIdStr);
                for (auto* pOther : pWorld->GetPlayers()) {
                    if (pOther->GetNetId() == targetNetId) {
                        uint32_t targetUserId = pOther->GetUserId();
                        if (std::find(pLock->m_accessUserIds.begin(), pLock->m_accessUserIds.end(), targetUserId) == pLock->m_accessUserIds.end())
                            pLock->m_accessUserIds.push_back(targetUserId);
                        break;
                    }
                }
            } catch (const std::exception&) {}
        }
    }

    if (eventParser.Contain("buttonClicked") && eventParser.Get("buttonClicked", 1) == "recalcLock") {

        LockInfo clearedLock = *pLock;
        clearedLock.m_coveredTiles.clear();
        LockHelpers::SendLockStateBroadcast(pWorld, clearedLock);

        pWorld->RecomputeTileLock(tileX, tileY);

        LockHelpers::SendLockStateBroadcast(pWorld, *pLock);

        VarList::OnPlayPositioned(pAvatar->Get(), pAvatar->GetNetId(), "audio/use_lock.wav");
    }

    GetDatabase()->GetWorldTable()->Save(*pWorld);
}

DIALOG_EVENT("world_lock_edit", OnWorldLockEditDialog) {
    auto pWorld = pAvatar->GetWorld();
    if (!pWorld)
        return;

    LockInfo* pLock = pWorld->GetWorldLock();
    if (!pLock)
        return;
    if (pLock->m_ownerId != pAvatar->GetUserId() &&
        std::find(pLock->m_accessUserIds.begin(), pLock->m_accessUserIds.end(), pAvatar->GetUserId()) == pLock->m_accessUserIds.end())
        return;

    pLock->m_isPublic = eventParser.Get<int32_t>("public_lock", 1) != 0;
    pLock->m_disableCustomMusic = eventParser.Get<int32_t>("disable_music", 1) != 0;
    pLock->m_hideCustomMusic = eventParser.Get<int32_t>("hide_music", 1) != 0;

    if (eventParser.Contain("playerNetID")) {
        std::string netIdStr = eventParser.Get("playerNetID", 1);
        if (!netIdStr.empty()) {
            try {
                int32_t targetNetId = std::stoi(netIdStr);
                for (auto* pOther : pWorld->GetPlayers()) {
                    if (pOther->GetNetId() == targetNetId) {
                        uint32_t targetUserId = pOther->GetUserId();
                        if (std::find(pLock->m_accessUserIds.begin(), pLock->m_accessUserIds.end(), targetUserId) == pLock->m_accessUserIds.end())
                            pLock->m_accessUserIds.push_back(targetUserId);

                        VarList::OnNameChanged(pOther->Get(), pOther->GetNetId(), pOther->GetFormattedName());
                        break;
                    }
                }
            } catch (const std::exception&) {}
        }
    }

    if (eventParser.Contain("buttonClicked") && eventParser.Get("buttonClicked", 1) == "getWorldKey")
        VarList::OnConsoleMessage(pAvatar->Get(), "World Keys aren't available on this server yet.");

    GetDatabase()->GetWorldTable()->Save(*pWorld);
}
