#pragma once
#include <string>
#include <vector>
#include <sstream>
#include <fmt/core.h>
#include <Commands/CommandType.hpp>
#include <Commands/CommandUtils.hpp>
#include <Manager/Database/Database.hpp>
#include <Packet/VariantFunction.hpp>

COMMAND_EVENT_ROLE("/givegems", GiveGemsCommand, PlayerRole::Developer) {
    std::istringstream stream(args);
    std::vector<std::string> tokens;
    std::string token;
    while (stream >> token)
        tokens.push_back(token);

    auto usage = [&]() {
        VarList::OnConsoleMessage(pAvatar->Get(), "`4Usage:`` /givegems <partial name, exact match or userID> <amount>");
    };

    if (tokens.empty()) {
        usage();
        return;
    }

    Player* pTarget = pAvatar;
    size_t amountIndex = 0;
    if (!CommandUtils::IsAllDigits(tokens[0])) {
        pTarget = CommandUtils::FindOnlinePlayer(tokens[0]);
        if (!pTarget) {
            VarList::OnConsoleMessage(pAvatar->Get(), fmt::format("`4Couldn't find an online player matching `w{}``.", tokens[0]));
            return;
        }
        amountIndex = 1;
    }

    if (amountIndex >= tokens.size() || !CommandUtils::IsAllDigits(tokens[amountIndex])) {
        usage();
        return;
    }
    int32_t amount = std::stoi(tokens[amountIndex]);
    if (amount <= 0)
        return;

    auto* pItems = pTarget->GetItems();
    pItems->SetGems(pItems->GetGems() + amount);
    GetDatabase()->GetPlayerTable()->Save(pTarget);
    VarList::OnSetBux(pTarget->Get(), pItems->GetGems());

    VarList::OnConsoleMessage(pAvatar->Get(), fmt::format("Gave {} gems to {}.", amount, pTarget->GetFormattedName()));
    if (pTarget != pAvatar)
        VarList::OnConsoleMessage(pTarget->Get(), fmt::format("You were given {} gems by {}.", amount, pAvatar->GetFormattedName()));
}
