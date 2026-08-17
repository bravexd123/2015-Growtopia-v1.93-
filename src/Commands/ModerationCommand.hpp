#pragma once
#include <World/WorldMenu.hpp>
#include <Server/ServerPool.hpp>
#include <string>
#include <vector>
#include <sstream>
#include <ctime>
#include <cstring>
#include <fmt/core.h>
#include <Commands/CommandType.hpp>
#include <Commands/CommandUtils.hpp>
#include <World/World.hpp>
#include <Manager/Database/Database.hpp>
#include <Packet/VariantFunction.hpp>

namespace {

    struct TimedPunishmentArgs {
        bool valid = false;
        std::string target;
        int64_t minutes = 0;
        std::string reason;
    };

    inline TimedPunishmentArgs ParseTimedPunishment(const std::string& args) {
        TimedPunishmentArgs result;
        std::istringstream stream(args);
        std::string target, minutesStr;
        stream >> target >> minutesStr;
        if (target.empty() || !CommandUtils::IsAllDigits(minutesStr))
            return result;
        std::string rest;
        std::getline(stream, rest);

        if (!rest.empty() && rest.front() == ' ')
            rest.erase(0, 1);

        for (const char* filter : { "soft", "hard", "ip" }) {
            if (rest.rfind(filter, 0) == 0 && (rest.size() == std::strlen(filter) || rest[std::strlen(filter)] == ' ')) {
                rest.erase(0, std::strlen(filter));
                if (!rest.empty() && rest.front() == ' ')
                    rest.erase(0, 1);
                break;
            }
        }

        result.valid = true;
        result.target = target;
        result.minutes = std::stoll(minutesStr);
        result.reason = rest.empty() ? "No reason given." : rest;
        return result;
    }
}

COMMAND_EVENT_ROLE("/ban", BanCommand, PlayerRole::Moderator) {
    auto parsed = ParseTimedPunishment(args);
    if (!parsed.valid) {
        VarList::OnConsoleMessage(pAvatar->Get(), "`4Usage:`` /ban <partial name, exact match or userID> <minutes> <optional reason>");
        return;
    }

    int64_t until = std::time(nullptr) + parsed.minutes * 60;
    Player* pTarget = CommandUtils::FindOnlinePlayer(parsed.target);

    if (pTarget) {
        pTarget->SetBan(until, parsed.reason);
        GetDatabase()->GetPlayerTable()->Save(pTarget);

        if (auto pWorld = pTarget->GetWorld()) {
            std::string broadcast = fmt::format("** The Ancients have used Ban on {}! **", pTarget->GetRawName());
            auto vList = VariantList::Create("OnConsoleMessage");
            vList.Insert(broadcast);
            SVariantPacket packet(vList);
            pWorld->BroadcastPacket(packet);
        }
        pTarget->RequestDisconnect();
        VarList::OnConsoleMessage(pAvatar->Get(), fmt::format("Banned {} for {} minute(s).", pTarget->GetFormattedName(), parsed.minutes));
        Logger::Print(INFO, "{} banned {} for {} minute(s): {}", pAvatar->GetRawName(), pTarget->GetRawName(), parsed.minutes, parsed.reason);
        return;
    }

    std::string resolvedName = GetDatabase()->GetPlayerTable()->SetBanByQuery(parsed.target, until, parsed.reason);
    if (resolvedName.empty()) {
        VarList::OnConsoleMessage(pAvatar->Get(), fmt::format("`4No account found matching `w{}``.", parsed.target));
        return;
    }
    VarList::OnConsoleMessage(pAvatar->Get(), fmt::format("Banned `w{}`` (offline) for {} minute(s).", resolvedName, parsed.minutes));
    Logger::Print(INFO, "{} banned offline account {} for {} minute(s): {}", pAvatar->GetRawName(), resolvedName, parsed.minutes, parsed.reason);
}

COMMAND_EVENT_ROLE("/boot", BootCommand, PlayerRole::Moderator) {
    std::string target = args;
    while (!target.empty() && target.back() == ' ')
        target.pop_back();
    if (target.empty()) {
        VarList::OnConsoleMessage(pAvatar->Get(), "`4Usage:`` /boot <exact match or partial name>");
        return;
    }

    Player* pTarget = CommandUtils::FindOnlinePlayer(target);
    if (!pTarget) {
        VarList::OnConsoleMessage(pAvatar->Get(), fmt::format("`4Couldn't find an online player matching `w{}``.", target));
        return;
    }

    auto pWorld = pTarget->GetWorld();
    if (!pWorld) {
        VarList::OnConsoleMessage(pAvatar->Get(), fmt::format("{} isn't in a world.", pTarget->GetFormattedName()));
        return;
    }

    pWorld->BroadcastPlayerLeft(pTarget);
    pWorld->ReleaseNetId(pTarget->GetNetId());
    pWorld->RemovePlayer(pTarget);
    pTarget->SetWorld(nullptr);
    pTarget->SetNetId(-1);
    pTarget->GetDetail().RemoveFlag(CLIENTFLAG_IS_IN);

    auto pServers = GetServerPool()->GetServers();
    std::string menu = pServers.empty() ? std::string("default|START\n")
        : WorldMenu::Build(pTarget, pServers.begin()->second->GetWorldPool());
    VarList::OnRequestWorldSelectMenu(pTarget->Get(), menu);

    VarList::OnConsoleMessage(pAvatar->Get(), fmt::format("Booted {} from their world.", pTarget->GetFormattedName()));
    Logger::Print(INFO, "{} booted {}", pAvatar->GetRawName(), pTarget->GetRawName());
}

COMMAND_EVENT_ROLE("/silence", SilenceCommand, PlayerRole::Moderator) {
    auto parsed = ParseTimedPunishment(args);
    if (!parsed.valid) {
        VarList::OnConsoleMessage(pAvatar->Get(), "`4Usage:`` /silence <partial name, exact match or userID> <minutes> <optional reason>");
        return;
    }
    int64_t until = std::time(nullptr) + parsed.minutes * 60;

    Player* pTarget = CommandUtils::FindOnlinePlayer(parsed.target);
    if (pTarget) {
        pTarget->SetSilencedUntil(until);
        GetDatabase()->GetPlayerTable()->Save(pTarget);
        if (auto pWorld = pTarget->GetWorld()) {
            std::string broadcast = fmt::format("`5{}`` was silenced.", pTarget->GetFormattedName());
            auto vList = VariantList::Create("OnConsoleMessage");
            vList.Insert(broadcast);
            SVariantPacket packet(vList);
            pWorld->BroadcastPacket(packet);
        }
        VarList::OnConsoleMessage(pAvatar->Get(), fmt::format("Silenced {} for {} minute(s).", pTarget->GetFormattedName(), parsed.minutes));
        Logger::Print(INFO, "{} silenced {} for {} minute(s): {}", pAvatar->GetRawName(), pTarget->GetRawName(), parsed.minutes, parsed.reason);
        return;
    }
    VarList::OnConsoleMessage(pAvatar->Get(), fmt::format("`4Couldn't find an online player matching `w{}``.", parsed.target));
}

COMMAND_EVENT_ROLE("/warn", WarnCommand, PlayerRole::Moderator) {
    std::istringstream stream(args);
    std::string target;
    stream >> target;
    std::string message;
    std::getline(stream, message);
    if (!message.empty() && message.front() == ' ')
        message.erase(0, 1);

    if (target.empty() || message.empty()) {
        VarList::OnConsoleMessage(pAvatar->Get(), "`4Usage:`` /warn <partial name, exact match or userID> <text message>");
        return;
    }

    Player* pTarget = CommandUtils::FindOnlinePlayer(target);
    if (!pTarget) {
        VarList::OnConsoleMessage(pAvatar->Get(), fmt::format("`4Couldn't find an online player matching `w{}``.", target));
        return;
    }

    VarList::OnTextOverlay(pTarget->Get(), fmt::format("`4Warning:`` {}", message));
    VarList::OnConsoleMessage(pAvatar->Get(), fmt::format("Warned {}.", pTarget->GetFormattedName()));
    Logger::Print(INFO, "{} warned {}: {}", pAvatar->GetRawName(), pTarget->GetRawName(), message);
}

COMMAND_EVENT_ROLE("/note", NoteCommand, PlayerRole::Moderator) {
    std::istringstream stream(args);
    std::string target;
    stream >> target;
    std::string text;
    std::getline(stream, text);
    if (!text.empty() && text.front() == ' ')
        text.erase(0, 1);

    if (target.empty()) {
        VarList::OnConsoleMessage(pAvatar->Get(), "`4Usage:`` /note <partial name, exact match or userID> <text message>");
        return;
    }

    Player* pTarget = CommandUtils::FindOnlinePlayer(target);
    std::string resolvedName;
    if (pTarget) {
        pTarget->AddNote(pAvatar->GetRawName(), text.empty() ? pTarget->GetRawName() : text);
        GetDatabase()->GetPlayerTable()->Save(pTarget);
        resolvedName = pTarget->GetRawName();
    } else {
        resolvedName = GetDatabase()->GetPlayerTable()->AddNoteByQuery(target, pAvatar->GetRawName(),
            text.empty() ? target : text);
    }

    if (resolvedName.empty()) {
        VarList::OnConsoleMessage(pAvatar->Get(), fmt::format("`4No account found matching `w{}``.", target));
        return;
    }
    VarList::OnConsoleMessage(pAvatar->Get(), fmt::format("Added a note to `w{}``.", resolvedName));
    Logger::Print(INFO, "{} added a note to {}: {}", pAvatar->GetRawName(), resolvedName, text);
}

COMMAND_EVENT_ROLE("/notes", NotesCommand, PlayerRole::Moderator) {
    std::string target = args;
    while (!target.empty() && target.back() == ' ')
        target.pop_back();
    if (target.empty()) {
        VarList::OnConsoleMessage(pAvatar->Get(), "`4Usage:`` /notes <partial name, exact match or userID>");
        return;
    }

    Player* pTarget = CommandUtils::FindOnlinePlayer(target);
    const std::vector<Player::PlayerNote>* pNotes = nullptr;
    std::string resolvedName;
    JsonPlayerTable::AccountSummary offline;

    if (pTarget) {
        pNotes = &pTarget->GetNotes();
        resolvedName = pTarget->GetRawName();
    } else {
        offline = GetDatabase()->GetPlayerTable()->FindAccountByQuery(target);
        if (!offline.found) {
            VarList::OnConsoleMessage(pAvatar->Get(), fmt::format("`4No account found matching `w{}``.", target));
            return;
        }
        pNotes = &offline.notes;
        resolvedName = offline.rawName;
    }

    if (pNotes->empty()) {
        VarList::OnConsoleMessage(pAvatar->Get(), fmt::format("`w{}`` has no notes.", resolvedName));
        return;
    }

    VarList::OnConsoleMessage(pAvatar->Get(), fmt::format("`5Notes for `w{}``:``", resolvedName));
    for (const auto& note : *pNotes) {
        std::time_t t = static_cast<std::time_t>(note.time);
        char buf[32];
        std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", std::localtime(&t));
        VarList::OnConsoleMessage(pAvatar->Get(), fmt::format("`w[{}] {}:`` {}", buf, note.author, note.text));
    }
}

COMMAND_EVENT_ROLE("/find", FindCommand, PlayerRole::Moderator) {
    std::string target = args;
    while (!target.empty() && target.back() == ' ')
        target.pop_back();
    if (target.empty()) {
        VarList::OnConsoleMessage(pAvatar->Get(), "`4Usage:`` /find <exact match>");
        return;
    }

    Player* pTarget = CommandUtils::FindOnlinePlayer(target);
    if (pTarget) {
        std::string banStatus = pTarget->IsBanned()
            ? fmt::format("Banned until `w{}``", pTarget->GetBannedUntil())
            : "Not banned";
        VarList::OnConsoleMessage(pAvatar->Get(), fmt::format(
            "`w{}`` - userID `w{}``, role `w{}``, IP `w{}``, `w{}`` (online now)",
            pTarget->GetRawName(), pTarget->GetUserId(), static_cast<int>(pTarget->GetRole()),
            pTarget->GetIp(), banStatus));
        return;
    }

    auto account = GetDatabase()->GetPlayerTable()->FindAccountByQuery(target);
    if (!account.found) {
        VarList::OnConsoleMessage(pAvatar->Get(), fmt::format("`4No account found matching `w{}``.", target));
        return;
    }
    std::string banStatus = (account.bannedUntil > std::time(nullptr))
        ? fmt::format("Banned until `w{}``", account.bannedUntil)
        : "Not banned";
    VarList::OnConsoleMessage(pAvatar->Get(), fmt::format(
        "`w{}`` - userID `w{}``, role `w{}``, last IP `w{}``, `w{}`` (offline)",
        account.rawName, account.userId, static_cast<int>(account.role),
        account.lastIp.empty() ? "unknown" : account.lastIp, banStatus));
}

COMMAND_EVENT_ROLE("/ipcheck", IpCheckCommand, PlayerRole::Moderator) {
    std::string target = args;
    while (!target.empty() && target.back() == ' ')
        target.pop_back();
    if (target.empty()) {
        VarList::OnConsoleMessage(pAvatar->Get(), "`4Usage:`` /ipcheck <partial name, exact match or userID>");
        return;
    }

    Player* pTargetOnline = CommandUtils::FindOnlinePlayer(target);
    std::string ip = pTargetOnline ? pTargetOnline->GetIp() : GetDatabase()->GetPlayerTable()->FindAccountByQuery(target).lastIp;
    if (ip.empty()) {
        VarList::OnConsoleMessage(pAvatar->Get(), fmt::format("`4No account found matching `w{}`` (or no IP on record).", target));
        return;
    }

    std::vector<std::string> matches = GetDatabase()->GetPlayerTable()->FindAccountsByIp(ip, 40);
    if (matches.empty()) {
        VarList::OnConsoleMessage(pAvatar->Get(), fmt::format("No other accounts found on IP `w{}``.", ip));
        return;
    }
    std::string list;
    for (const auto& name : matches) {
        if (!list.empty())
            list += ", ";
        list += name;
    }
    VarList::OnConsoleMessage(pAvatar->Get(), fmt::format("`w{}`` account(s) on IP `w{}``: {}", matches.size(), ip, list));
}
