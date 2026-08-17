#pragma once
#include <World/WorldMenu.hpp>
#include <fmt/core.h>
#include <Event/EventType.hpp>
#include <Event/EventPool.hpp>
#include <Player/Player.hpp>
#include <World/World.hpp>
#include <Packet/VariantFunction.hpp>
#include <Packet/PacketFactory.hpp>
#include <Logger/Logger.hpp>

ACTION_EVENT("quit", OnQuit) {
    Logger::Print(INFO, "Player {} requested quit, disconnecting", pAvatar->GetRawName());
    pAvatar->RequestDisconnect();
}

ACTION_EVENT("quit_to_exit", OnQuitToExit) {
    Logger::Print(INFO, "Player {} exited world via quit_to_exit", pAvatar->GetRawName());

    std::string leftWorldName;
    if (auto pWorld = pAvatar->GetWorld()) {
        leftWorldName = pWorld->GetName();

        pWorld->BroadcastPlayerLeft(pAvatar);

        pWorld->RemovePlayer(pAvatar);
        pAvatar->SetWorld(nullptr);
    }
    pAvatar->GetDetail().RemoveFlag(CLIENTFLAG_IS_IN);

    std::string menu = WorldMenu::Build(pAvatar, pServer->GetWorldPool(), leftWorldName);
    VarList::OnRequestWorldSelectMenu(pAvatar->Get(), menu);
    Logger::Print(INFO, "Sent world-select menu to player {}", pAvatar->GetRawName());
}
