#pragma once
#include <string>
#include <Commands/CommandManager.hpp>

class Player;

#define COMMAND_EVENT(name, className)                                                  \
    class className {                                                                   \
    public:                                                                             \
        static void Run(Player* pAvatar, const std::string& args);                      \
        className() { CommandManager::Get().Register(name, className::Run); }           \
    } className ## _commandEvent;                                                       \
    void className::Run(Player* pAvatar, const std::string& args)

#define COMMAND_EVENT_ROLE(name, className, role)                                       \
    class className {                                                                   \
    public:                                                                             \
        static void Run(Player* pAvatar, const std::string& args);                      \
        className() { CommandManager::Get().Register(name, className::Run, role); }     \
    } className ## _commandEvent;                                                       \
    void className::Run(Player* pAvatar, const std::string& args)
