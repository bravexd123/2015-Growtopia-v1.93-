#pragma once
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include <Player/Player.hpp>
#include <Packet/VariantFunction.hpp>

using CommandHandler = std::function<void(Player*, const std::string& args)>;

class CommandManager {
public:
    static CommandManager& Get() {
        static CommandManager instance;
        return instance;
    }

    void Register(const std::string& name, CommandHandler handler, PlayerRole requiredRole = PlayerRole::Default) {
        m_commands[name] = std::move(handler);
        m_commandRoles[name] = requiredRole;
        m_commandNames.push_back(name);
    }

    void RegisterAlias(const std::string& alias, const std::string& existingName) {
        auto it = m_commands.find(existingName);
        if (it == m_commands.end())
            return;
        Register(alias, it->second, m_commandRoles[existingName]);
    }

    bool Dispatch(Player* pAvatar, const std::string& text) const {
        if (text.empty() || text[0] != '/')
            return false;

        std::string name = text;
        std::string args;
        if (auto spacePos = text.find(' '); spacePos != std::string::npos) {
            name = text.substr(0, spacePos);
            args = text.substr(spacePos + 1);
        }

        auto it = m_commands.find(name);
        bool allowed = it != m_commands.end() && pAvatar->GetRole() >= m_commandRoles.at(name);
        if (allowed) {
            it->second(pAvatar, args);
        } else {

            VarList::OnConsoleMessage(pAvatar->Get(), "`4Unknown command.`` Enter /? for a list of valid commands.");
        }
        return true;
    }

    std::vector<std::string> GetCommandNames(PlayerRole viewerRole) const {
        std::vector<std::string> names;
        names.reserve(m_commandNames.size());
        for (const auto& name : m_commandNames)
            if (viewerRole >= m_commandRoles.at(name))
                names.push_back(name);
        return names;
    }

private:
    std::unordered_map<std::string, CommandHandler> m_commands;
    std::unordered_map<std::string, PlayerRole> m_commandRoles;
    std::vector<std::string> m_commandNames;
};

inline CommandManager* GetCommandManager() {
    return &CommandManager::Get();
}
