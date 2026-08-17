#pragma once
#include <string>
#include <sstream>
#include <algorithm>
#include <fmt/core.h>
#include <Commands/CommandType.hpp>
#include <Commands/CommandUtils.hpp>
#include <World/World.hpp>
#include <Manager/Database/Database.hpp>
#include <Packet/VariantFunction.hpp>

COMMAND_EVENT_ROLE("/resetworld", ResetWorldCommand, PlayerRole::Developer) {
    auto pWorld = pAvatar->GetWorld();
    if (!pWorld)
        return;

    uint32_t width = pWorld->GetWidth();
    uint32_t height = pWorld->GetHeight();

    std::istringstream stream(args);
    std::string widthStr, heightStr;
    stream >> widthStr >> heightStr;
    if (CommandUtils::IsAllDigits(widthStr))
        width = std::clamp<uint32_t>(static_cast<uint32_t>(std::stoul(widthStr)), 30, 255);
    if (CommandUtils::IsAllDigits(heightStr))
        height = std::clamp<uint32_t>(static_cast<uint32_t>(std::stoul(heightStr)), 30, 255);

    std::string worldName = pWorld->GetName();
    pWorld->Init(worldName, width, height);
    GetDatabase()->GetWorldTable()->Save(*pWorld);

    auto players = pWorld->GetPlayers();
    for (auto* pOther : players) {
        VarList::OnConsoleMessage(pOther->Get(), fmt::format("World `w{}`` was reset to {}x{} - reconnecting...", worldName, width, height));
        pOther->RequestDisconnect();
    }
}
