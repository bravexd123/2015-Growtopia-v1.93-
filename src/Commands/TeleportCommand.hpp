#pragma once
#include <string>
#include <fmt/core.h>
#include <Commands/CommandType.hpp>
#include <Commands/CommandUtils.hpp>
#include <World/World.hpp>
#include <Event/EventPool.hpp>
#include <Event/EventType.hpp>
#include <Server/ServerPool.hpp>
#include <Packet/VariantFunction.hpp>
#include <Packet/TextFunction.hpp>
#include <Logger/Logger.hpp>

COMMAND_EVENT_ROLE("/warpto", WarpToCommand, PlayerRole::Moderator) {
    std::string targetQuery = args;
    while (!targetQuery.empty() && targetQuery.back() == ' ')
        targetQuery.pop_back();
    if (targetQuery.empty()) {
        VarList::OnConsoleMessage(pAvatar->Get(), "`4Usage:`` /warpto <partial name, exact match or userID>");
        return;
    }

    Player* pTarget = CommandUtils::FindOnlinePlayer(targetQuery);
    if (!pTarget) {
        VarList::OnConsoleMessage(pAvatar->Get(), fmt::format("`4Couldn't find an online player matching `w{}``.", targetQuery));
        return;
    }
    auto pTargetWorld = pTarget->GetWorld();
    if (!pTargetWorld) {
        VarList::OnConsoleMessage(pAvatar->Get(), fmt::format("{} isn't in a world.", pTarget->GetFormattedName()));
        return;
    }

    auto pCallerServer = CommandUtils::FindServerForPlayer(pAvatar);
    if (!pCallerServer) {
        Logger::Print(WARNING, "/warpto: couldn't resolve owning Server for caller {}", pAvatar->GetRawName());
        return;
    }

    std::string destWorldName = pTargetWorld->GetName();
    bool alreadyThere = pAvatar->GetWorld() && pAvatar->GetWorld()->GetName() == destWorldName;

    if (!alreadyThere) {
        if (auto pCallerWorld = pAvatar->GetWorld()) {
            pCallerWorld->BroadcastPlayerLeft(pAvatar);
            pCallerWorld->ReleaseNetId(pAvatar->GetNetId());
            pCallerWorld->RemovePlayer(pAvatar);
            pAvatar->SetWorld(nullptr);
            pAvatar->SetNetId(-1);
            pAvatar->GetDetail().RemoveFlag(CLIENTFLAG_IS_IN);
        }
        if (auto* pJoinEvent = GetEventPool()->ActionManager::GetEventIfExists("join_request")) {
            TextParse joinParser(fmt::format("action|join_request\nname|{}\n", destWorldName));
            pJoinEvent->sig_function(pAvatar, pCallerServer, std::string(), joinParser, nullptr);
        }
    }

    ENetPeer* callerPeer = pAvatar->Get();
    std::string targetName = pTarget->GetRawName();
    GetServerPool()->ScheduleDelayed(alreadyThere ? 0 : 200, [pCallerServer, callerPeer, targetName, destWorldName]() {
        auto playerPool = pCallerServer->GetPlayerPool();
        if (!playerPool)
            return;
        Player* pCaller = playerPool->GetPlayerByPeer(callerPeer);
        if (!pCaller || !pCaller->GetWorld() || pCaller->GetWorld()->GetName() != destWorldName)
            return;

        Player* pFreshTarget = CommandUtils::FindOnlinePlayer(targetName);
        if (!pFreshTarget || !pFreshTarget->GetWorld() || pFreshTarget->GetWorld()->GetName() != destWorldName)
            return;

        pCaller->SetPosition(pFreshTarget->GetX(), pFreshTarget->GetY());
        VarList::OnSetPos(pCaller->Get(), pCaller->GetNetId(), pFreshTarget->GetX(), pFreshTarget->GetY());
    });

    VarList::OnConsoleMessage(pAvatar->Get(), fmt::format("Warped to {}.", pTarget->GetFormattedName()));
    Logger::Print(INFO, "{} warped to {} in world '{}'", pAvatar->GetRawName(), targetName, destWorldName);
}

COMMAND_EVENT_ROLE("/warptolock", WarpToLockCommand, PlayerRole::Moderator) {
    auto pWorld = pAvatar->GetWorld();
    if (!pWorld) {
        VarList::OnConsoleMessage(pAvatar->Get(), "You're not in a world.");
        return;
    }
    const LockInfo* pLock = pWorld->GetWorldLock();
    if (!pLock) {
        VarList::OnConsoleMessage(pAvatar->Get(), "This world has no World Lock.");
        return;
    }

    float x = static_cast<float>(pLock->m_anchorX) * 32.0f;
    float y = static_cast<float>(pLock->m_anchorY) * 32.0f;
    pAvatar->SetPosition(x, y);
    VarList::OnSetPos(pAvatar->Get(), pAvatar->GetNetId(), x, y);
    VarList::OnConsoleMessage(pAvatar->Get(), "Warped to the World Lock.");
}
