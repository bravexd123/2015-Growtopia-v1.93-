#pragma once
#include <string>
#include <vector>
#include <fmt/core.h>
#include <Commands/CommandType.hpp>
#include <Commands/CommandUtils.hpp>
#include <World/World.hpp>
#include <Packet/VariantFunction.hpp>
#include <Manager/Database/Database.hpp>

namespace WorldOwnerUtils {

    inline std::shared_ptr<World> RequireOwnedWorld(Player* pAvatar) {
        auto pWorld = pAvatar->GetWorld();
        if (!pWorld)
            return nullptr;
        const LockInfo* pLock = pWorld->GetWorldLock();

        if (pAvatar->GetRole() != PlayerRole::Default)
            return pWorld;
        if (!pLock || pLock->m_ownerId != pAvatar->GetUserId()) {
            VarList::OnConsoleMessage(pAvatar->Get(), "`4You need to own the World Lock here to do that.``");
            return nullptr;
        }
        return pWorld;
    }

    inline void SendToSpawn(Player* pTarget, std::shared_ptr<World> pWorld) {
        uint32_t doorX = 0, doorY = 0;
        float x, y;
        if (pWorld->FindDoorTile(doorX, doorY)) {
            x = static_cast<float>(doorX) * 32.0f;
            y = static_cast<float>(doorY) * 32.0f;
        } else {
            uint32_t surfaceTile = pWorld->GetHeight() * 4 / 10;
            x = (pWorld->GetWidth() / 2) * 32.0f;
            y = (surfaceTile >= 2 ? surfaceTile - 2 : 0) * 32.0f;
        }
        pTarget->SetPosition(x, y);
        VarList::OnSetPos(pTarget->Get(), pTarget->GetNetId(), x, y);
        VarList::OnPlayPositioned(pTarget->Get(), pTarget->GetNetId(), "audio/teleport.wav");
    }
}

COMMAND_EVENT("/kick", KickCommand) {
    auto pWorld = WorldOwnerUtils::RequireOwnedWorld(pAvatar);
    if (!pWorld)
        return;
    if (args.empty()) {
        VarList::OnConsoleMessage(pAvatar->Get(), "`4Usage:`` /kick <name>");
        return;
    }

    Player* pTarget = CommandUtils::FindOnlinePlayer(args);
    if (!pTarget || pTarget->GetWorld() != pWorld) {
        VarList::OnConsoleMessage(pAvatar->Get(), fmt::format("`4`w{}`` isn't in this world.``", args));
        return;
    }
    if (pTarget == pAvatar)
        return;

    const LockInfo* pLock = pWorld->GetWorldLock();
    if (pLock && pLock->m_ownerId == pTarget->GetUserId() && pAvatar->GetRole() == PlayerRole::Default) {
        VarList::OnConsoleMessage(pAvatar->Get(), "`4You can't kick the world's owner.``");
        return;
    }

    WorldOwnerUtils::SendToSpawn(pTarget, pWorld);
    VarList::OnConsoleMessage(pTarget->Get(), fmt::format("{} `okicked you to the spawn point.``", pAvatar->GetFormattedName()));
    VarList::OnConsoleMessage(pAvatar->Get(), fmt::format("`oKicked {} `oto the spawn point.``", pTarget->GetFormattedName()));
}

COMMAND_EVENT("/kickall", KickAllCommand) {
    auto pWorld = WorldOwnerUtils::RequireOwnedWorld(pAvatar);
    if (!pWorld)
        return;

    std::size_t kicked = 0;
    for (auto* pOther : pWorld->GetPlayers()) {
        if (pOther == pAvatar)
            continue;
        WorldOwnerUtils::SendToSpawn(pOther, pWorld);
        VarList::OnConsoleMessage(pOther->Get(), fmt::format("{} `okicked everyone to the spawn point.``", pAvatar->GetFormattedName()));
        kicked++;
    }
    VarList::OnConsoleMessage(pAvatar->Get(), fmt::format("`oKicked `w{}`` player(s) to the spawn point.``", kicked));
}

COMMAND_EVENT("/pull", PullCommand) {
    auto pWorld = WorldOwnerUtils::RequireOwnedWorld(pAvatar);
    if (!pWorld)
        return;
    if (args.empty()) {
        VarList::OnConsoleMessage(pAvatar->Get(), "`4Usage:`` /pull <name>");
        return;
    }

    Player* pTarget = CommandUtils::FindOnlinePlayer(args);
    if (!pTarget || pTarget->GetWorld() != pWorld) {
        VarList::OnConsoleMessage(pAvatar->Get(), fmt::format("`4`w{}`` isn't in this world.``", args));
        return;
    }
    if (pTarget == pAvatar)
        return;

    float x = pAvatar->GetX(), y = pAvatar->GetY();
    pTarget->SetPosition(x, y);
    VarList::OnSetPos(pTarget->Get(), pTarget->GetNetId(), x, y);
    VarList::OnPlayPositioned(pTarget->Get(), pTarget->GetNetId(), "audio/teleport.wav");
    VarList::OnConsoleMessage(pTarget->Get(), fmt::format("{} `opulled you.``", pAvatar->GetFormattedName()));
    VarList::OnConsoleMessage(pAvatar->Get(), fmt::format("`oPulled {}``.", pTarget->GetFormattedName()));
}

COMMAND_EVENT("/uba", UnbanAllCommand) {
    auto pWorld = WorldOwnerUtils::RequireOwnedWorld(pAvatar);
    if (!pWorld)
        return;

    std::size_t count = pWorld->GetBannedUserIds().size();
    if (count == 0) {
        VarList::OnConsoleMessage(pAvatar->Get(), "`oNobody is banned from this world.``");
        return;
    }
    pWorld->RestoreBannedUsers({});
    GetDatabase()->GetWorldTable()->Save(*pWorld);
    VarList::OnConsoleMessage(pAvatar->Get(), fmt::format("`2Unbanned `w{}`` player(s) from `w{}``.", count, pWorld->GetName()));
}
