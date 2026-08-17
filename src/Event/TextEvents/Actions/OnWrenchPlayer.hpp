#pragma once
#include <World/WorldMenu.hpp>
#include <ctime>
#include <vector>
#include <memory>
#include <algorithm>
#include <fmt/core.h>
#include <Event/EventType.hpp>
#include <Player/Player.hpp>
#include <World/World.hpp>
#include <Packet/VariantFunction.hpp>
#include <Packet/VariantList.hpp>
#include <ENetWrapper/ENetWrapper.hpp>
#include <Logger/Logger.hpp>
#include <Manager/Database/Database.hpp>
#include <Manager/Item/ItemEffects.hpp>
#include <Manager/Trade/TradeManager.hpp>
#include <Event/TextEvents/Actions/OnFriends.hpp>

namespace {
    enum class WrenchViewerRank { Regular, Admin, Owner };

    inline WrenchViewerRank GetWrenchRank(const std::shared_ptr<World>& pWorld, uint32_t userId) {
        if (!pWorld || !pWorld->HasWorldLock())
            return WrenchViewerRank::Regular;
        const auto* lock = pWorld->GetWorldLock();
        if (lock->m_ownerId == userId)
            return WrenchViewerRank::Owner;
        if (std::find(lock->m_accessUserIds.begin(), lock->m_accessUserIds.end(), userId) != lock->m_accessUserIds.end())
            return WrenchViewerRank::Admin;
        return WrenchViewerRank::Regular;
    }

    inline void SendWrenchTargetMenu(Player* pAvatar, Player* pTarget) {
        auto pWorld = pAvatar->GetWorld();
        if (!pWorld)
            return;

        WrenchViewerRank viewerRank = GetWrenchRank(pWorld, pAvatar->GetUserId());
        WrenchViewerRank targetRank = GetWrenchRank(pWorld, pTarget->GetUserId());

        bool canModerate = viewerRank != WrenchViewerRank::Regular
            && (viewerRank == WrenchViewerRank::Owner || targetRank == WrenchViewerRank::Regular);

        std::string buttons = "add_button|wrench_trade|`2Trade``|noflags|0|0|\r\n";

        if (!pAvatar->IsFriend(pTarget->GetUserId()))
            buttons += "add_button|friend_add|`2Add as friend``|noflags|0|0|\r\n";
        if (canModerate) {
            buttons +=
                "add_button|wrench_kick|`4Kick``|noflags|0|0|\r\n"
                "add_button|wrench_pull|`4Pull``|noflags|0|0|\r\n"
                "add_button|wrench_ban|`4World Ban``|noflags|0|0|\r\n";
        }

        std::string content = fmt::format(
            "\r\n"
            "embed_data|targetNetID|{}\r\n"
            "add_label_with_icon|big|`w{}`` (`w{}``)|left|18|\r\n"
            "add_spacer|small|\r\n"
            "{}"
            "end_dialog|wrench_target_menu||Continue",
            pTarget->GetNetId(), pTarget->GetFormattedName(), pTarget->GetLevel(), buttons);

        auto vList = VariantList::Create("OnDialogRequest");
        vList.Insert(content);
        ENetWrapper::SendVariantList(pAvatar->Get(), vList);
        Logger::Print(INFO, "Sent wrench-target menu for {} to {}", pTarget->GetRawName(), pAvatar->GetRawName());
    }
}

ACTION_EVENT("wrench", OnWrenchPlayer) {
    int32_t targetNetId = eventParser.Get<int32_t>("netid", 1);
    if (targetNetId != pAvatar->GetNetId()) {
        auto pWorld = pAvatar->GetWorld();
        if (!pWorld)
            return;
        Player* pTarget = nullptr;
        for (auto* pOther : pWorld->GetPlayers()) {
            if (pOther->GetNetId() == targetNetId) {
                pTarget = pOther;
                break;
            }
        }
        if (pTarget)
            SendWrenchTargetMenu(pAvatar, pTarget);
        return;
    }

    auto pWorld = pAvatar->GetWorld();
    if (!pWorld)
        return;

    auto* pItems = pAvatar->GetItems();
    std::string supporterLine = pAvatar->GetAccountTier() != AccountTier::Default
        ? "You are a `2Supporter``!"
        : "You are not yet a `2Supporter``.";

    std::size_t playerCount = pWorld->GetPlayers().size();
    int64_t daysOld = (static_cast<int64_t>(std::time(nullptr)) - pAvatar->GetCreatedAt()) / 86400;

    std::string worldsOwned;
    for (const auto& name : GetDatabase()->GetWorldTable()->FindWorldsOwnedBy(pAvatar->GetUserId()))
        worldsOwned += name + " ";
    if (!worldsOwned.empty())
        worldsOwned.pop_back();

    std::vector<std::string> effectNames;
    if (pAvatar->IsGhostEnabled())
        effectNames.push_back("Ghost Mode");
    if (pAvatar->IsFlagOn(PLAYERFLAG_IS_INVISIBLE))
        effectNames.push_back("Invisibility");
    if (ItemEffects::HasEnhancedDigging(pItems))
        effectNames.push_back("Enhanced Digging");
    if (ItemEffects::HasDoubleJump(pItems))
        effectNames.push_back("Double Jump");
    if (ItemEffects::HasLaserVisor(pItems))
        effectNames.push_back("Laser Visor");

    pAvatar->PrunePlaymods();
    for (const auto& mod : pAvatar->GetPlaymods()) {
        const auto* pPlaymod = Playmods::GetById(mod.m_id);
        if (!pPlaymod || !pPlaymod->m_name[0])
            continue;
        effectNames.push_back(fmt::format("{}`` (`w{}`` left)",
            pPlaymod->m_name, Playmods::FormatDuration(mod.m_expiry - std::time(nullptr))));
    }

    std::string effectsBlock;
    if (effectNames.empty()) {
        effectsBlock = "add_label|small|`wNo active effects``|left|0|\r\n";
    } else {
        for (const auto& name : effectNames)
            effectsBlock += fmt::format("add_label|small|`2{}``|left|0|\r\n", name);
    }

    std::string content = fmt::format(
        "\r\n"
        "embed_data|netID|{}\r\n"
        "add_player_info|{}|{}|{}|{}\r\n"
        "add_spacer|small|\r\n"
        "add_button|alist|`$Achievements (0`5/``96)``|noflags|0|0|\r\n"
        "add_spacer|small|\r\n"
        "add_textbox|`wActive effects:``|\r\n"
        "add_spacer|small|\r\n"
        "{}"
        "add_spacer|small|\r\n"
        "add_textbox|You have `w{}`` backpack slots.|\r\n"
        "add_spacer|small|\r\n"
        "add_textbox|Current world: `w{}`` (`w{}``, `w{}``) (`w{}`` person{})``|\r\n"
        "add_spacer|small|\r\n"
        "add_textbox|Worlds owned :{}|\r\n"
        "add_spacer|small|\r\n"
        "add_textbox|{}|\r\n"
        "add_spacer|small|\r\n"
        "add_textbox|This account was created `w{}`` days ago.|\r\n"
        "add_spacer|small|\r\n"
        "end_dialog|popup||Continue",
        pAvatar->GetNetId(), pAvatar->GetFormattedName(),
        pAvatar->GetLevel(), pAvatar->GetXp(), Player::XpForNextLevel(pAvatar->GetLevel()),
        effectsBlock, pItems->m_backpackSpace,
        pWorld->GetName(), static_cast<int>(pAvatar->GetX() / 32.0f), static_cast<int>(pAvatar->GetY() / 32.0f),
        playerCount, playerCount == 1 ? "" : "s", worldsOwned, supporterLine, daysOld);

    auto vList = VariantList::Create("OnDialogRequest");
    vList.Insert(content);
    ENetWrapper::SendVariantList(pAvatar->Get(), vList);
    Logger::Print(INFO, "Sent self-wrench player info popup to {}", pAvatar->GetRawName());
}

DIALOG_EVENT("alist", OnAchievementsListDialog) {
    VarList::OnConsoleMessage(pAvatar->Get(), "Achievements aren't available on this server yet.");
}

DIALOG_EVENT("wrench_target_menu", OnWrenchTargetMenu) {
    auto pWorld = pAvatar->GetWorld();
    if (!pWorld)
        return;

    int32_t targetNetId = eventParser.Get<int32_t>("targetNetID", 1);
    Player* pTarget = nullptr;
    for (auto* pOther : pWorld->GetPlayers()) {
        if (pOther->GetNetId() == targetNetId) {
            pTarget = pOther;
            break;
        }
    }
    if (!pTarget || pTarget == pAvatar)
        return;

    std::string button = eventParser.Get("buttonClicked", 1);
    if (button.empty() || button == "Continue")
        return;

    if (button == "wrench_trade") {
        if (TradeManager::IsBusyWithSomeoneElse(pAvatar, pTarget)) {
            VarList::OnTalkBubble(pAvatar->Get(), pAvatar->GetNetId(), "That person is busy.");
            TradeManager::CancelTrade(pAvatar);
            return;
        }
        TradeManager::RequestTrade(pAvatar, pTarget);
        return;
    }

    if (button == "friend_add") {

        uint32_t myId = pAvatar->GetUserId();
        uint32_t targetId = pTarget->GetUserId();

        if (pAvatar->IsFriend(targetId))
            return;
        if (pTarget->HasSentRequestTo(myId)) {

            if (!pAvatar->AddFriend(targetId)) {
                VarList::OnConsoleMessage(pAvatar->Get(), fmt::format("`4Your friends list is full`` (`w{}`` max).", Player::kMaxFriends));
                return;
            }
            pTarget->AddFriend(myId);
            pTarget->ClearSentRequest(myId);
            GetDatabase()->GetPlayerTable()->Save(pAvatar);
            GetDatabase()->GetPlayerTable()->Save(pTarget);

            VarList::OnTalkBubble(pAvatar->Get(), pAvatar->GetNetId(), fmt::format("`5[`wFriend request sent to `w{}```5]``", pTarget->GetRawName()));
            CAction::PlaySFX(pAvatar->Get(), "love_in", 0);
            VarList::OnConsoleMessage(pAvatar->Get(), fmt::format("`3FRIEND ADDED: `oYou're now friends with `w{}```o!", pTarget->GetRawName()));
            CAction::PlaySFX(pTarget->Get(), "love_in", 0);
            VarList::OnConsoleMessage(pTarget->Get(), fmt::format("`3FRIEND ADDED: `oYou're now friends with `w{}```o!", pAvatar->GetRawName()));
            return;
        }
        if (pAvatar->HasSentRequestTo(targetId))
            return;

        pAvatar->AddSentRequest(targetId);
        GetDatabase()->GetPlayerTable()->Save(pAvatar);
        VarList::OnTalkBubble(pAvatar->Get(), pAvatar->GetNetId(), fmt::format("`5[`wFriend request sent to `w{}```5]``", pTarget->GetRawName()));
        return;
    }

    WrenchViewerRank viewerRank = GetWrenchRank(pWorld, pAvatar->GetUserId());
    WrenchViewerRank targetRank = GetWrenchRank(pWorld, pTarget->GetUserId());
    bool canModerate = viewerRank != WrenchViewerRank::Regular
        && (viewerRank == WrenchViewerRank::Owner || targetRank == WrenchViewerRank::Regular);
    if (!canModerate)
        return;

    if (button == "wrench_kick") {
        pWorld->BroadcastPlayerLeft(pTarget);
        pWorld->ReleaseNetId(pTarget->GetNetId());
        pWorld->RemovePlayer(pTarget);
        pTarget->SetWorld(nullptr);
        pTarget->SetNetId(-1);
        pTarget->GetDetail().RemoveFlag(CLIENTFLAG_IS_IN);

        std::string menu = WorldMenu::Build(pTarget, pServer->GetWorldPool());
        VarList::OnRequestWorldSelectMenu(pTarget->Get(), menu);

        VarList::OnConsoleMessage(pAvatar->Get(), fmt::format("Kicked {} from the world.", pTarget->GetFormattedName()));
        Logger::Print(INFO, "{} wrench-kicked {} from world '{}'", pAvatar->GetRawName(), pTarget->GetRawName(), pWorld->GetName());
        return;
    }

    if (button == "wrench_pull") {

        pTarget->SetPosition(pAvatar->GetX(), pAvatar->GetY());
        VarList::OnSetPos(pTarget->Get(), pTarget->GetNetId(), pAvatar->GetX(), pAvatar->GetY());
        VarList::OnConsoleMessage(pAvatar->Get(), fmt::format("Pulled {} to you.", pTarget->GetFormattedName()));
        Logger::Print(INFO, "{} wrench-pulled {} in world '{}'", pAvatar->GetRawName(), pTarget->GetRawName(), pWorld->GetName());
        return;
    }

    if (button == "wrench_ban") {
        pWorld->BanUser(pTarget->GetUserId());
        GetDatabase()->GetWorldTable()->Save(*pWorld);

        pWorld->BroadcastPlayerLeft(pTarget);
        pWorld->ReleaseNetId(pTarget->GetNetId());
        pWorld->RemovePlayer(pTarget);
        pTarget->SetWorld(nullptr);
        pTarget->SetNetId(-1);
        pTarget->GetDetail().RemoveFlag(CLIENTFLAG_IS_IN);

        std::string menu = WorldMenu::Build(pTarget, pServer->GetWorldPool());
        VarList::OnRequestWorldSelectMenu(pTarget->Get(), menu);

        VarList::OnConsoleMessage(pAvatar->Get(), fmt::format("Banned {} from this world.", pTarget->GetFormattedName()));
        Logger::Print(INFO, "{} wrench-world-banned {} from world '{}'", pAvatar->GetRawName(), pTarget->GetRawName(), pWorld->GetName());
        return;
    }
}
