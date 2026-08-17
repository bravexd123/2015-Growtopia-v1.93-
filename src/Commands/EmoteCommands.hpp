#pragma once
#include <array>
#include <string>
#include <string_view>
#include <Commands/CommandType.hpp>
#include <World/World.hpp>
#include <Packet/VariantFunction.hpp>

namespace Emotes {
    inline std::string_view Canonical(std::string_view name) {
        if (name == "facepalm") return "fp";
        return name;
    }

    inline void Perform(Player* pAvatar, std::string_view name) {
        auto pWorld = pAvatar->GetWorld();
        if (!pWorld)
            return;
        std::string action = "/" + std::string(Canonical(name));
        for (auto* pOther : pWorld->GetPlayers())
            VarList::OnAction(pOther->Get(), pAvatar->GetNetId(), action);
    }

    inline constexpr std::array<std::string_view, 17> kEmotes{
        "smile", "sad", "cry", "laugh", "wave", "dance",
        "love", "kiss", "sleep", "yes", "no", "wink",
        "troll", "cheer", "fp", "mad", "facepalm"
    };
}

namespace {
    struct EmoteCommandRegistrar {
        EmoteCommandRegistrar() {
            for (std::string_view emote : Emotes::kEmotes) {
                std::string name = "/" + std::string(emote);
                CommandManager::Get().Register(name, [emote](Player* pAvatar, const std::string&) {
                    Emotes::Perform(pAvatar, emote);
                });
            }
        }
    } g_emoteCommandRegistrar;
}
