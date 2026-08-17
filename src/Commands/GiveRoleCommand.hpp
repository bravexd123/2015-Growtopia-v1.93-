#pragma once
#include <string>
#include <vector>
#include <sstream>
#include <fmt/core.h>
#include <Commands/CommandType.hpp>
#include <Commands/CommandUtils.hpp>
#include <Manager/Database/Database.hpp>
#include <Packet/VariantFunction.hpp>

COMMAND_EVENT_ROLE("/giverole", GiveRoleCommand, PlayerRole::Developer) {
    std::istringstream stream(args);
    std::vector<std::string> tokens;
    std::string token;
    while (stream >> token)
        tokens.push_back(token);

    if (tokens.size() != 2 || !CommandUtils::IsAllDigits(tokens[1])) {
        VarList::OnConsoleMessage(pAvatar->Get(), "`4Usage:`` /giverole <player> <role id> (0=Default, 1=Moderator, 2=Developer)");
        return;
    }
    int roleId = std::stoi(tokens[1]);
    if (roleId < 0 || roleId > 2) {
        VarList::OnConsoleMessage(pAvatar->Get(), "`4Role id must be 0 (Default), 1 (Moderator), or 2 (Developer).``");
        return;
    }
    PlayerRole role = static_cast<PlayerRole>(roleId);
    static const char* kRoleNames[] = { "Default", "Moderator", "Developer" };

    Player* pTarget = CommandUtils::FindOnlinePlayer(tokens[0]);
    if (pTarget) {
        pTarget->SetRole(role);
        GetDatabase()->GetPlayerTable()->Save(pTarget);
        VarList::OnConsoleMessage(pAvatar->Get(), fmt::format("Set {}'s role to {}.", pTarget->GetFormattedName(), kRoleNames[roleId]));
        VarList::OnConsoleMessage(pTarget->Get(), fmt::format("Your role has been set to {}.", kRoleNames[roleId]));
        return;
    }

    if (GetDatabase()->GetPlayerTable()->SetRoleByName(tokens[0], role)) {
        VarList::OnConsoleMessage(pAvatar->Get(), fmt::format("Set `w{}``'s role to {} (offline - takes effect next login).", tokens[0], kRoleNames[roleId]));
    } else {
        VarList::OnConsoleMessage(pAvatar->Get(), fmt::format("`4No account found matching `w{}``.", tokens[0]));
    }
}
