#pragma once
#include <algorithm>
#include <string>
#include <vector>
#include <fmt/core.h>
#include <Player/Player.hpp>
#include <World/World.hpp>
#include <World/WorldPool.hpp>
#include <Manager/Database/Database.hpp>

namespace WorldMenu {

    inline std::string Floater(const std::string& name, std::size_t players, const char* colour, float size = 0.5f) {
        return fmt::format("add_floater|{}|{}|{:.2f}|{}\n", name, players, size, colour);
    }

    inline std::string Build(Player* pAvatar, std::shared_ptr<WorldPool> pWorldPool, const std::string& focusWorld = "") {
        std::string menu;
        menu += fmt::format("default|{}\n", focusWorld.empty() ? "START" : focusWorld);
        menu += "add_filter|\n";

        {
            std::vector<std::pair<std::string, std::size_t>> busy;
            for (auto& [name, pWorld] : pWorldPool->GetWorlds()) {
                if (!pWorld)
                    continue;
                std::size_t count = pWorld->GetPlayerCount();
                if (count > 0)
                    busy.emplace_back(name, count);
            }
            std::sort(busy.begin(), busy.end(), [](const auto& a, const auto& b) {
                if (a.second != b.second)
                    return a.second > b.second;
                return a.first < b.first;
            });
            if (busy.size() > 10)
                busy.resize(10);

            menu += "add_heading|Top Worlds<ROW2>|\n";
            if (busy.empty()) {

                menu += Floater("START", 0, "3529161471", 0.55f);
            } else {
                for (const auto& [name, count] : busy)
                    menu += Floater(name, count, "3529161471", count >= 5 ? 0.6f : 0.5f);
            }
        }

        {
            auto owned = GetDatabase()->GetWorldTable()->FindWorldsOwnedBy(pAvatar->GetUserId());
            std::sort(owned.begin(), owned.end());
            if (!owned.empty()) {
                menu += "add_heading|My Worlds<CR>|\n";
                for (const auto& name : owned) {
                    auto pWorld = pWorldPool->GetWorld(name);
                    menu += Floater(name, pWorld ? pWorld->GetPlayerCount() : 0, "2147418367");
                }
            }
        }

        {
            const auto& recent = pAvatar->GetRecentWorlds();
            if (!recent.empty()) {
                menu += "add_heading|Recently Visited Worlds<CR>|\n";
                for (const auto& name : recent) {
                    auto pWorld = pWorldPool->GetWorld(name);
                    menu += Floater(name, pWorld ? pWorld->GetPlayerCount() : 0, "3417414143");
                }
            }
        }

        return menu;
    }

}
