#pragma once
#include <string>
#include <algorithm>
#include <cctype>
#include <fmt/core.h>
#include <Commands/CommandType.hpp>
#include <World/World.hpp>
#include <Packet/VariantFunction.hpp>
#include <Packet/TextFunction.hpp>
#include <Manager/Trade/TradeManager.hpp>

namespace {
    std::string ToLowerCopy(std::string s) {
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
        return s;
    }
}

COMMAND_EVENT("/trade", TradeCommand) {
    std::string targetName = args;
    while (!targetName.empty() && (targetName.back() == '\r' || targetName.back() == '\n' || targetName.back() == ' '))
        targetName.pop_back();
    while (!targetName.empty() && targetName.front() == ' ')
        targetName.erase(targetName.begin());

    if (targetName.empty()) {
        VarList::OnConsoleMessage(pAvatar->Get(), "To trade with a specific person in this world, do `2/trade <``full player name`2>``");
        return;
    }

    if (ToLowerCopy(targetName) == ToLowerCopy(pAvatar->GetRawName())) {
        VarList::OnConsoleMessage(pAvatar->Get(), "You can't trade with yourself!");
        return;
    }

    auto pWorld = pAvatar->GetWorld();
    if (!pWorld)
        return;

    Player* pTarget = nullptr;
    for (auto* pOther : pWorld->GetPlayers()) {
        if (ToLowerCopy(pOther->GetRawName()) == ToLowerCopy(targetName)) {
            pTarget = pOther;
            break;
        }
    }
    if (!pTarget) {
        VarList::OnConsoleMessage(pAvatar->Get(), fmt::format("`4{}`` isn't in this world.", targetName));
        return;
    }

    if (TradeManager::IsBusyWithSomeoneElse(pAvatar, pTarget)) {
        VarList::OnTalkBubble(pAvatar->Get(), pAvatar->GetNetId(), "That person is busy.");
        TradeManager::CancelTrade(pAvatar);
        return;
    }

    TradeManager::RequestTrade(pAvatar, pTarget);
}
