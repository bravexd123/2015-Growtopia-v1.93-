#pragma once
#include <ctime>
#include <string>
#include <vector>
#include <fmt/core.h>
#include <Commands/CommandType.hpp>
#include <World/World.hpp>
#include <World/WorldPool.hpp>
#include <Server/Server.hpp>
#include <Server/ServerPool.hpp>
#include <Player/PlayerPool.hpp>
#include <Packet/VariantFunction.hpp>
#include <Manager/Item/WeatherMachines.hpp>

namespace SocialCommandUtils {

    inline std::vector<Player*> GetAllPlayers() {
        std::vector<Player*> players;
        for (auto& [instanceId, pServer] : GetServerPool()->GetServers()) {
            if (!pServer)
                continue;
            auto pPool = pServer->GetPlayerPool();
            if (!pPool)
                continue;
            for (auto& [connectId, pPlayer] : pPool->GetPlayers())
                if (pPlayer)
                    players.push_back(pPlayer);
        }
        return players;
    }
}

COMMAND_EVENT("/who", WhoCommand) {
    auto pWorld = pAvatar->GetWorld();
    if (!pWorld)
        return;

    std::vector<std::string> names;
    for (auto* pOther : pWorld->GetPlayers()) {
        names.push_back(pOther->GetFormattedName());
        if (pOther != pAvatar)
            VarList::OnTalkBubble(pAvatar->Get(), pOther->GetNetId(), pOther->GetFormattedName());
    }

    std::string joined;
    for (std::size_t i = 0; i < names.size(); i++) {
        if (i)
            joined += ", ";
        joined += names[i];
    }
    VarList::OnConsoleMessage(pAvatar->Get(), fmt::format("`wWho's in `${}``:`` {}``", pWorld->GetName(), joined));
}

COMMAND_EVENT("/me", MeCommand) {
    if (args.empty())
        return;
    auto pWorld = pAvatar->GetWorld();
    if (!pWorld || pAvatar->IsSilenced())
        return;

    std::string message = fmt::format("`6<{}`` `#{}```6>``", pAvatar->GetFormattedName(), args);
    for (auto* pOther : pWorld->GetPlayers()) {
        VarList::OnTalkBubble(pOther->Get(), pAvatar->GetNetId(), message);
        VarList::OnConsoleMessage(pOther->Get(), message);
    }
}

COMMAND_EVENT("/sb", BroadcastCommand) {
    if (args.empty())
        return;
    if (pAvatar->IsSilenced())
        return;

    auto pWorld = pAvatar->GetWorld();
    std::string worldName = pWorld ? pWorld->GetName() : std::string("EXIT");
    std::string message = fmt::format("CP:0_PL:0_OID:_CT:[SB]_ `5** from ({}`5) in [```${}```5] ** : ```${}``",
        pAvatar->GetFormattedName(), worldName, args);

    for (auto* pOther : SocialCommandUtils::GetAllPlayers())
        VarList::OnConsoleMessage(pOther->Get(), message);
}

COMMAND_EVENT("/time", TimeCommand) {
    std::time_t now = std::time(nullptr);
    std::tm local{};
#ifdef _WIN32
    localtime_s(&local, &now);
#else
    localtime_r(&now, &local);
#endif
    static const char* kMonths[] = { "January", "February", "March", "April", "May", "June",
        "July", "August", "September", "October", "November", "December" };
    int day = local.tm_mday;
    const char* suffix = (day >= 11 && day <= 13) ? "th"
        : (day % 10 == 1) ? "st"
        : (day % 10 == 2) ? "nd"
        : (day % 10 == 3) ? "rd" : "th";

    VarList::OnConsoleMessage(pAvatar->Get(), fmt::format("`2Growtopia Time: {} {}{}, {:02d}:{:02d}.",
        kMonths[local.tm_mon], day, suffix, local.tm_hour, local.tm_min));
}

COMMAND_EVENT_ROLE("/weather", WeatherCommand, PlayerRole::Moderator) {
    if (args.empty()) {
        VarList::OnConsoleMessage(pAvatar->Get(), "`oUsage: /weather <id>``  `w0``=Sunny `w2``=Night `w3``=Arid `w5``=Rainy City `w8``=Spooky `w10``=Nothingness `w11``=Snowy `w15``=Warp Speed `w17``=Comet `w18``=Party `w19``=Pineapples");
        return;
    }
    int32_t weatherId = 0;
    try {
        weatherId = std::stoi(args);
    } catch (const std::logic_error&) {
        return;
    }
    VarList::OnSetCurrentWeather(pAvatar->Get(), weatherId);
    VarList::OnConsoleMessage(pAvatar->Get(), fmt::format("`5Weather set to `w{}`` (preview - the world's own weather is unchanged).", weatherId));
}

COMMAND_EVENT("/rules", RulesCommand) {
    static const char* kRules[] = {
        "`oGrowtopia Rules:``",
        "`w1.`` Don't scam, cheat, or steal from other players.",
        "`w2.`` Don't use bots, macros, or auto-paste - all your accounts will be banned.",
        "`w3.`` Don't spam, beg, or advertise other games.",
        "`w4.`` Don't share your password. Staff will NEVER ask for it.",
        "`w5.`` Don't impersonate a moderator or developer.",
        "`w6.`` Keep chat clean - no swearing, hate speech, or harassment.",
        "`w7.`` Report rule-breakers instead of retaliating.",
    };
    for (const char* line : kRules)
        VarList::OnConsoleMessage(pAvatar->Get(), line);
}

COMMAND_EVENT("/status", StatusCommand) {
    auto pWorld = pAvatar->GetWorld();
    VarList::OnConsoleMessage(pAvatar->Get(), fmt::format("`oYou are `w{}``, level `w{}`` (`w{}``/`w{}`` XP).",
        pAvatar->GetFormattedName(), pAvatar->GetLevel(), pAvatar->GetXp(),
        Player::XpForNextLevel(pAvatar->GetLevel())));
    if (pWorld) {
        VarList::OnConsoleMessage(pAvatar->Get(), fmt::format("`oCurrent world: `w{}`` (`w{}`` here) at (`w{}``, `w{}``).",
            pWorld->GetName(), pWorld->GetPlayerCount(),
            static_cast<int>(pAvatar->GetX() / 32.0f), static_cast<int>(pAvatar->GetY() / 32.0f)));
    }
    VarList::OnConsoleMessage(pAvatar->Get(), fmt::format("`oYou have `w{}`` gems and `w{}`` backpack slots.",
        pAvatar->GetItems()->GetGems(), pAvatar->GetItems()->m_backpackSpace));
}

COMMAND_EVENT("/stats", StatsCommand) {
    static const std::time_t kBootTime = std::time(nullptr);
    std::time_t uptime = std::time(nullptr) - kBootTime;

    VarList::OnConsoleMessage(pAvatar->Get(), fmt::format("`oGrowtopia server uptime: `w{}`` hours, `w{}`` minutes.",
        uptime / 3600, (uptime % 3600) / 60));
    VarList::OnConsoleMessage(pAvatar->Get(), fmt::format("`oPlayers online: `w{}``   Active worlds: `w{}``",
        GetServerPool()->GetActivePlayers(), GetServerPool()->GetActiveWorlds()));
}

COMMAND_EVENT("/mods", ModsCommand) {
    std::vector<std::string> staff;
    for (auto* pOther : SocialCommandUtils::GetAllPlayers())
        if (pOther->GetRole() != PlayerRole::Default)
            staff.push_back(pOther->GetFormattedName());

    if (staff.empty()) {
        VarList::OnConsoleMessage(pAvatar->Get(), "`oNo moderators are online right now.");
        return;
    }
    std::string joined;
    for (std::size_t i = 0; i < staff.size(); i++) {
        if (i)
            joined += ", ";
        joined += staff[i];
    }
    VarList::OnConsoleMessage(pAvatar->Get(), fmt::format("`oMods online: {}``", joined));
}

COMMAND_EVENT("/finditem", FindItemCommand) {
    VarList::OnDialogRequest(pAvatar->Get(),
        "set_default_color|`o\n"
        "add_text_input|n|Search: ||26|\n"
        "add_searchable_item_list||sourceType:allItems;listType:iconWithCustomLabel;resultLimit:30|n|\n"
        "add_quick_exit|\n"
        "end_dialog|find_item|||");
}
