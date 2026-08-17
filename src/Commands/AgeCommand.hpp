#pragma once
#include <string>
#include <fmt/core.h>
#include <Commands/CommandType.hpp>
#include <Commands/CommandUtils.hpp>
#include <World/World.hpp>
#include <Manager/Database/Database.hpp>
#include <Packet/VariantFunction.hpp>

COMMAND_EVENT_ROLE("/age", AgeCommand, PlayerRole::Developer) {
    if (!CommandUtils::IsAllDigits(args) || args.empty()) {
        VarList::OnConsoleMessage(pAvatar->Get(), "`4Usage:`` /age <minutes>");
        return;
    }
    auto pWorld = pAvatar->GetWorld();
    if (!pWorld)
        return;

    int64_t minutes = std::stoll(args);
    if (minutes <= 0)
        return;

    uint32_t affected = pWorld->AgeWorld(minutes * 60);
    GetDatabase()->GetWorldTable()->Save(*pWorld);

    VarList::OnConsoleMessage(pAvatar->Get(), fmt::format("Aged `w{}`` planted seed(s) in `w{}`` by {} minutes.", affected, pWorld->GetName(), minutes));
}
