#pragma once
#include <string>
#include <sstream>
#include <vector>
#include <fmt/core.h>
#include <Commands/CommandType.hpp>
#include <Commands/CommandUtils.hpp>
#include <Manager/Database/Database.hpp>
#include <Packet/VariantFunction.hpp>

COMMAND_EVENT_ROLE("/rename", RenameCommand, PlayerRole::Developer) {
    std::istringstream stream(args);
    std::string targetQuery, newName;
    stream >> targetQuery;
    std::getline(stream, newName);
    if (!newName.empty() && newName.front() == ' ')
        newName.erase(0, 1);

    if (targetQuery.empty() || newName.empty() || newName.size() < 3 || newName.size() > 20) {
        VarList::OnConsoleMessage(pAvatar->Get(), "`4Usage:`` /rename <user> <new name> (3-20 characters)");
        return;
    }

    Player* pTarget = CommandUtils::FindOnlinePlayer(targetQuery);
    if (!pTarget) {
        VarList::OnConsoleMessage(pAvatar->Get(), fmt::format("`4Couldn't find an online player matching `w{}``.", targetQuery));
        return;
    }

    std::string oldName = pTarget->GetRawName();
    pTarget->SetRawName(newName);
    pTarget->SetNameLocked(true);
    GetDatabase()->GetPlayerTable()->Save(pTarget);

    VarList::OnConsoleMessage(pAvatar->Get(), fmt::format("Renamed `w{}`` to `w{}``.", oldName, newName));
    VarList::OnConsoleMessage(pTarget->Get(), fmt::format("Your name was changed to `w{}`` by staff.", newName));
    Logger::Print(INFO, "{} renamed {} to {}", pAvatar->GetRawName(), oldName, newName);
}

COMMAND_EVENT_ROLE("/flag", FlagCommand, PlayerRole::Developer) {
    std::string code = args;
    while (!code.empty() && code.back() == ' ')
        code.pop_back();
    if (code.empty()) {
        VarList::OnConsoleMessage(pAvatar->Get(), "`4Usage:`` /flag <country code, e.g. in>");
        return;
    }

    if (!pAvatar->GetDetail().SetCountryCode(code)) {
        VarList::OnConsoleMessage(pAvatar->Get(), fmt::format("`4Invalid country code:`` {}", code));
        return;
    }
    GetDatabase()->GetPlayerTable()->Save(pAvatar);
    VarList::OnConsoleMessage(pAvatar->Get(), fmt::format("Flag set to `w{}``.", code));
}

COMMAND_EVENT_ROLE("/selfage", SelfAgeCommand, PlayerRole::Developer) {
    if (!CommandUtils::IsAllDigits(args) || args.empty()) {
        VarList::OnConsoleMessage(pAvatar->Get(), "`4Usage:`` /selfage <seconds>");
        return;
    }
    int64_t seconds = std::stoll(args);
    if (seconds <= 0)
        return;

    pAvatar->AgePlaymods(seconds);
    VarList::OnConsoleMessage(pAvatar->Get(), fmt::format("Aged yourself by {} second(s) - active playmods wear off that much sooner.", seconds));
}

COMMAND_EVENT_ROLE("/dropall", DropAllCommand, PlayerRole::Developer) {
    auto* pItems = pAvatar->GetItems();

    std::vector<uint16_t> itemIds;
    itemIds.reserve(pItems->m_bpItems.size());
    for (const auto& [itemId, count] : pItems->m_bpItems)
        itemIds.push_back(itemId);

    for (uint16_t itemId : itemIds) {
        auto it = pItems->m_bpItems.find(itemId);
        if (it != pItems->m_bpItems.end())
            pItems->RemoveItem(itemId, it->second);
    }
    GetDatabase()->GetPlayerTable()->Save(pAvatar);
    PlayerItems::SendInventoryState(pAvatar);

    VarList::OnConsoleMessage(pAvatar->Get(), fmt::format("Trashed {} item stack(s).", itemIds.size()));
    Logger::Print(INFO, "{} used /dropall ({} stacks)", pAvatar->GetRawName(), itemIds.size());
}
