#include <Commands/CommandManager.hpp>

#include <Commands/HelpCommand.hpp>
#include <Commands/GiveCommand.hpp>
#include <Commands/GiveGemsCommand.hpp>
#include <Commands/OneHitCommand.hpp>
#include <Commands/InvisCommand.hpp>
#include <Commands/GhostCommand.hpp>
#include <Commands/NickCommand.hpp>
#include <Commands/ResetWorldCommand.hpp>
#include <Commands/GiveRoleCommand.hpp>
#include <Commands/AgeCommand.hpp>
#include <Commands/ModerationCommand.hpp>
#include <Commands/TeleportCommand.hpp>
#include <Commands/TradeCommand.hpp>
#include <Commands/DevCommand.hpp>
#include <Commands/SocialCommands.hpp>
#include <Commands/EmoteCommands.hpp>
#include <Commands/WorldOwnerCommands.hpp>

namespace {
    struct PlaceholderCommands {
        PlaceholderCommands() {

            static const char* kNames[] = {
                "/top", "/radio", "/renderworld", "/news",
                "/friends", "/friend", "/ignore", "/msg", "/r", "/rgo"
            };
            for (const char* name : kNames)
                CommandManager::Get().Register(name, [](Player*, const std::string&) {});
        }
    } g_placeholderCommands;
}
