#pragma once
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <fmt/core.h>
#include <Commands/CommandType.hpp>
#include <Commands/CommandUtils.hpp>
#include <Manager/Item/ItemManager.hpp>
#include <Manager/Item/ItemInfo.hpp>
#include <Manager/Database/Database.hpp>
#include <Packet/VariantFunction.hpp>

COMMAND_EVENT_ROLE("/give", GiveCommand, PlayerRole::Developer) {
    std::istringstream stream(args);
    std::vector<std::string> tokens;
    std::string token;
    while (stream >> token)
        tokens.push_back(token);

    auto usage = [&]() {
        VarList::OnConsoleMessage(pAvatar->Get(), "`4Usage:`` /give <partial name, exact match or userID> <count> <full item name or ID>");
    };

    if (tokens.size() < 2) {
        usage();
        return;
    }

    Player* pTarget = pAvatar;
    size_t countIndex = 0;
    if (!CommandUtils::IsAllDigits(tokens[0])) {
        pTarget = CommandUtils::FindOnlinePlayer(tokens[0]);
        if (!pTarget) {
            VarList::OnConsoleMessage(pAvatar->Get(), fmt::format("`4Couldn't find an online player matching `w{}``.", tokens[0]));
            return;
        }
        countIndex = 1;
    }

    if (countIndex >= tokens.size() || !CommandUtils::IsAllDigits(tokens[countIndex])) {
        usage();
        return;
    }
    int32_t count = std::stoi(tokens[countIndex]);
    if (count <= 0)
        return;

    std::string itemQuery;
    for (size_t i = countIndex + 1; i < tokens.size(); i++) {
        if (!itemQuery.empty())
            itemQuery += " ";
        itemQuery += tokens[i];
    }
    if (itemQuery.empty()) {
        usage();
        return;
    }

    ItemInfo* pItem = CommandUtils::IsAllDigits(itemQuery)
        ? GetItemManager()->GetItem(static_cast<uint32_t>(std::stoul(itemQuery)))
        : GetItemManager()->GetItemByName(itemQuery);
    if (!pItem) {
        VarList::OnConsoleMessage(pAvatar->Get(), fmt::format("`4Couldn't find an item matching `w{}``.", itemQuery));
        return;
    }

    int32_t remaining = count;
    while (remaining > 0) {
        uint8_t chunk = static_cast<uint8_t>(std::min<int32_t>(remaining, 255));
        pTarget->GetItems()->AddItem(static_cast<uint16_t>(pItem->m_Id), chunk);
        remaining -= chunk;
    }
    GetDatabase()->GetPlayerTable()->Save(pTarget);
    PlayerItems::SendInventoryState(pTarget);

    VarList::OnConsoleMessage(pAvatar->Get(), fmt::format("Gave {} `w{}`` to {}.", count, pItem->m_name, pTarget->GetFormattedName()));
    if (pTarget != pAvatar)
        VarList::OnConsoleMessage(pTarget->Get(), fmt::format("You were given {} `w{}`` by {}.", count, pItem->m_name, pAvatar->GetFormattedName()));
}
