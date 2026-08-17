#pragma once
#include <World/WorldMenu.hpp>
#include <Event/EventType.hpp>
#include <Event/EventPool.hpp>
#include <Packet/VariantFunction.hpp>
#include <Logger/Logger.hpp>

ACTION_EVENT("enter_game", OnEnterGame) {
    if (!pAvatar->GetDetail().IsFlagOn(CLIENTFLAG_LOGGED_ON))
        return;

    std::string menu = WorldMenu::Build(pAvatar, pServer->GetWorldPool());

    VarList::OnRequestWorldSelectMenu(pAvatar->Get(), menu);
    Logger::Print(INFO, "Sent world-select menu to player {}", pAvatar->GetRawName());
}
