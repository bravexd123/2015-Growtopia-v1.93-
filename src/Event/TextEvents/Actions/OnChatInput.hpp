#pragma once
#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>
#include <fmt/core.h>
#include <Event/EventType.hpp>
#include <Event/EventPool.hpp>
#include <Player/Player.hpp>
#include <World/World.hpp>
#include <Packet/VariantList.hpp>
#include <Packet/VariantFunction.hpp>
#include <Packet/PacketFactory.hpp>
#include <Logger/Logger.hpp>
#include <Commands/CommandManager.hpp>

ACTION_EVENT("input", OnChatInput) {
    std::size_t textPos = eventData.find("text|");
    if (textPos == std::string::npos)
        return;
    std::string text = eventData.substr(textPos + 5);
    while (!text.empty() && (text.back() == '\r' || text.back() == '\n'))
        text.pop_back();
    if (text.empty())
        return;

    if (CommandManager::Get().Dispatch(pAvatar, text))
        return;

    auto pWorld = pAvatar->GetWorld();
    if (!pWorld)
        return;

    if (pAvatar->IsSilenced())
        return;

    if (pAvatar->RegisterChatMessageAndCheckSpam()) {
        VarList::OnConsoleMessage(pAvatar->Get(),
            "`6>>`4Spam detected! ``Please wait a bit before typing anything else.  "
            "Please note, any form of bot/macro/auto-paste will get all your accounts banned, so don't do it!");
        return;
    }

    if (pAvatar->IsDuctTaped()) {
        static constexpr std::string_view kMuffled[] = { "mfmm", "mmfmfm", "mffm", "mfmfmm" };
        std::string muffled;
        muffled.reserve(text.size());
        std::size_t wordIndex = 0;
        for (std::size_t i = 0; i < text.size();) {
            if (std::isspace(static_cast<unsigned char>(text[i]))) {
                muffled += text[i++];
                continue;
            }
            while (i < text.size() && !std::isspace(static_cast<unsigned char>(text[i])))
                i++;
            muffled += kMuffled[wordIndex++ % 4];
        }
        text = std::move(muffled);
    }

    std::string consoleMsg = fmt::format("`o<{}`o> `o{}``", pAvatar->GetFormattedName(), text);
    auto consoleList = VariantList::Create("OnConsoleMessage");
    consoleList.Insert(consoleMsg);
    SVariantPacket consolePacket(consoleList);
    pWorld->BroadcastPacket(consolePacket);

    std::string chatColor = (pAvatar->GetRole() == PlayerRole::Developer) ? "`5"
        : (pAvatar->GetRole() == PlayerRole::Moderator) ? "`^"
        : "`w";
    std::string bubbleMsg = fmt::format("{}{}``", chatColor, text);
    for (auto* pOther : pWorld->GetPlayers())
        VarList::OnTalkBubble(pOther->Get(), pAvatar->GetNetId(), bubbleMsg);

    Logger::Print(INFO, "Player {} said: {}", pAvatar->GetRawName(), text);
}
