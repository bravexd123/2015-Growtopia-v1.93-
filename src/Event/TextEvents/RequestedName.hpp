#pragma once
#include <functional>
#include <algorithm>
#include <cctype>
#include <ctime>
#include <fmt/core.h>
#include <Event/EventType.hpp>
#include <Packet/TextFunction.hpp>
#include <Packet/VariantFunction.hpp>
#include <Utils/MiscUtils.hpp>
#include <Logger/Logger.hpp>
#include <Manager/Database/Database.hpp>
#include <Commands/CommandUtils.hpp>

EVENT("tankIDName", OnRequestedName) {
    if (pAvatar->GetDetail().IsFlagOn(CLIENTFLAG_LOGGED_ON))
        return;
    if (!pAvatar->GetDetail().Serialize(eventParser)) {
        pAvatar->RequestDisconnect();
        return;
    }
    pAvatar->SetRawName(pAvatar->GetDetail().GetRequestedName());

    if (!Utils::IsValidUsername(pAvatar->GetRawName()) || (pAvatar->GetRawName().length() < 3 || pAvatar->GetRawName().length() > 16)) {
        CAction::Log(pAvatar->Get(), "`4Oops! `oYour name is including invalid characters, please try again.``");
        pAvatar->RequestDisconnect();
        return;
    }

    std::string submittedName = pAvatar->GetDetail().GetTankIDName();
    std::string submittedPass = pAvatar->GetDetail().GetTankIDPass();
    bool isGrowIdLogin = !submittedName.empty() && !submittedPass.empty();

    if (isGrowIdLogin) {
        JsonPlayerTable* pTable = GetDatabase()->GetPlayerTable();
        if (!pTable || !pTable->IsAccountExist(submittedName) || !pTable->Load(submittedName, pAvatar) ||
            pAvatar->GetDetail().GetTankIDPass() != submittedPass) {
            Logger::Print(INFO, "Player failed GrowID logon for name '{}': invalid credentials", submittedName);
            CAction::Log(pAvatar->Get(), "`4Unable to log on:`` That GrowID doesn't seem valid.  If you don't have one, click Cancel, un-check 'I have a GrowID', then click Connect.");
            pAvatar->RequestDisconnect();
            return;
        }

        auto lowerCopy = [](std::string s) { std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); }); return s; };
        std::string lowerSubmitted = lowerCopy(submittedName);
        for (auto& [connectId, pOther] : pServer->GetPlayerPool()->GetPlayers()) {
            if (pOther == pAvatar || !pOther->GetDetail().IsFlagOn(CLIENTFLAG_LOGGED_ON))
                continue;
            if (lowerCopy(pOther->GetDetail().GetTankIDName()) == lowerSubmitted) {
                Logger::Print(INFO, "Evicting stale session for '{}' (connectId {}) - same account logging in again (connectId {})",
                    submittedName, connectId, pAvatar->GetConnectId());

                ENetPeer* pStalePeer = pOther->Get();
                pOther->OnDisconnect();
                pServer->GetPlayerPool()->RemovePlayerByPeer(pStalePeer);
                enet_peer_reset(pStalePeer);
                break;
            }
        }

        if (pAvatar->IsBanned()) {
            int64_t remainingMinutes = (pAvatar->GetBannedUntil() - std::time(nullptr) + 59) / 60;
            std::string reason = pAvatar->GetBanReason().empty() ? "No reason given." : pAvatar->GetBanReason();
            Logger::Print(INFO, "Rejected login for banned player '{}' ({} minute(s) remaining)", submittedName, remainingMinutes);
            CAction::Log(pAvatar->Get(), fmt::format("`4You are banned:`` {} (`w{}`` minute(s) remaining)", reason, remainingMinutes));
            pAvatar->RequestDisconnect();
            return;
        }
    }

    pAvatar->SetLastIp(pAvatar->GetIp());

    pAvatar->GetDetail().SetFlag(CLIENTFLAG_LOGGED_ON);

    if (isGrowIdLogin) {

        VarList::SetHasGrowID(pAvatar->Get(), true, submittedName, submittedPass);
    } else {

        std::string guestName = pAvatar->GetRawName();
        std::string rid = pAvatar->GetDetail().GetRelativeId();
        if (!rid.empty()) {

            uint32_t suffix = static_cast<uint32_t>(std::hash<std::string>{}(rid) % 1000);
            guestName = fmt::format("{}_{:03d}", guestName, suffix);

            JsonPlayerTable* pTable = GetDatabase()->GetPlayerTable();
            if (pTable && pTable->IsGuestExist(rid) && pTable->LoadGuest(rid, pAvatar))
                Logger::Print(INFO, "Restored guest progress for rid {} (name '{}')", rid, guestName);
            else
                pAvatar->SetCreatedAt(static_cast<int64_t>(std::time(nullptr)));
        }
        pAvatar->SetRawName(guestName);
        pAvatar->SetDisplayName(guestName);
        VarList::SetHasGrowID(pAvatar->Get(), false, "", "");
    }

    VarList::OnLogonAccepted(pAvatar->Get());

    if (isGrowIdLogin) {

        std::size_t friendsOnline = 0;
        for (uint32_t friendId : pAvatar->GetFriends())
            if (CommandUtils::FindOnlinePlayer(std::to_string(friendId)))
                friendsOnline++;

        std::string message = friendsOnline == 0
            ? fmt::format("Welcome back, `w{}``. No friends are online.", pAvatar->GetFormattedName())
            : fmt::format("Welcome back, `w{}``.  `w{}`` friend{} {} online.", pAvatar->GetFormattedName(),
                friendsOnline, friendsOnline == 1 ? "" : "s", friendsOnline == 1 ? "is" : "are");
        CAction::Log(pAvatar->Get(), message);

        if (!pAvatar->IsFlagOn(PLAYERFLAG_IS_INVISIBLE)) {
            for (uint32_t friendId : pAvatar->GetFriends()) {
                Player* pFriend = CommandUtils::FindOnlinePlayer(std::to_string(friendId));
                if (!pFriend || !pFriend->GetShowFriendNotifications())
                    continue;
                VarList::OnConsoleMessage(pFriend->Get(), fmt::format("`3FRIEND ALERT:`` {} has `2logged on``.", pAvatar->GetFormattedName()));
                CAction::PlaySFX(pFriend->Get(), "friend_logon", 0);
            }
        }
    }

    Logger::Print(INFO, "Player {} logged on ({})", pAvatar->GetRawName(), isGrowIdLogin ? "GrowID" : "guest");

}

namespace {
    struct OnRequestedNameAlias {
        OnRequestedNameAlias() { GetEventPool()->AddEvent("requestedName", OnRequestedName::Run); }
    } g_onRequestedNameAlias;
}
