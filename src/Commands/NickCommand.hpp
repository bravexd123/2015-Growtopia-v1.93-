#pragma once
#include <string>
#include <algorithm>
#include <cctype>
#include <fmt/core.h>
#include <Commands/CommandType.hpp>
#include <World/World.hpp>
#include <Packet/VariantFunction.hpp>

COMMAND_EVENT_ROLE("/nick", NickCommand, PlayerRole::Moderator) {
    std::string input = args;
    while (!input.empty() && (input.back() == '\r' || input.back() == '\n' || input.back() == ' '))
        input.pop_back();

    if (input.empty()) {
        pAvatar->ClearNickname();
        auto pWorld = pAvatar->GetWorld();
        if (pWorld) {
            for (auto* pOther : pWorld->GetPlayers()) {
                if (pOther == pAvatar)
                    continue;
                VarList::OnNameChanged(pOther->Get(), pAvatar->GetNetId(), pAvatar->GetFormattedName());
            }
        }
        VarList::OnNameChanged(pAvatar->Get(), pAvatar->GetNetId(), pAvatar->GetFormattedName());
        VarList::OnConsoleMessage(pAvatar->Get(), fmt::format("Nickname cleared - back to {}.", pAvatar->GetFormattedName()));
        return;
    }

    std::string colorCode = "`w";
    std::string visibleName = input;
    if (input.size() >= 2 && input[0] == '`') {
        colorCode = input.substr(0, 2);
        visibleName = input.substr(2);
    }

    if (visibleName.length() < 3 || visibleName.length() > 20) {
        VarList::OnConsoleMessage(pAvatar->Get(), "`4Usage:`` /nick <new name> (3-20 characters)");
        return;
    }
    bool valid = std::all_of(visibleName.begin(), visibleName.end(), [](unsigned char c) {
        return std::isalnum(c) || c == '.' || c == '-';
    });
    if (!valid) {
        VarList::OnConsoleMessage(pAvatar->Get(), "`4Nicknames can only contain letters, numbers, dots (.) and dashes (-).``");
        return;
    }

    pAvatar->SetNickname(fmt::format("{}{}``", colorCode, visibleName));

    auto pWorld = pAvatar->GetWorld();
    if (pWorld) {
        for (auto* pOther : pWorld->GetPlayers()) {
            if (pOther == pAvatar)
                continue;
            VarList::OnNameChanged(pOther->Get(), pAvatar->GetNetId(), pAvatar->GetFormattedName());
        }
    }
    VarList::OnNameChanged(pAvatar->Get(), pAvatar->GetNetId(), pAvatar->GetFormattedName());

    VarList::OnConsoleMessage(pAvatar->Get(), fmt::format("Nickname set to {} for this session.", pAvatar->GetFormattedName()));
}
