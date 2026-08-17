#pragma once
#include <string>
#include <Commands/CommandType.hpp>
#include <Player/Player.hpp>
#include <Packet/VariantFunction.hpp>

COMMAND_EVENT("/help", HelpCommand) {
    std::string message = ">> Commands:";
    for (const auto& name : CommandManager::Get().GetCommandNames(pAvatar->GetRole()))
        message += " " + name;

    VarList::OnConsoleMessage(pAvatar->Get(), message);
}

namespace {
    struct HelpCommandAlias {
        HelpCommandAlias() { CommandManager::Get().RegisterAlias("/?", "/help"); }
    } g_helpCommandAlias;
}
