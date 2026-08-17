#pragma once
#include <string>
#include <Event/EventType.hpp>
#include <Event/EventPool.hpp>
#include <Player/Player.hpp>
#include <Packet/VariantFunction.hpp>
#include <Manager/Database/Database.hpp>
#include <Utils/Color.hpp>
#include <Logger/Logger.hpp>

ACTION_EVENT("setSkin", OnSetSkin) {

    std::string colorStr = eventParser.Get("color", 1);
    if (colorStr.empty())
        return;
    uint32_t colorValue = static_cast<uint32_t>(std::stoul(colorStr));
    pAvatar->GetItems()->m_skinColor = Color(colorValue);
    GetDatabase()->GetPlayerTable()->Save(pAvatar);

    VarList::OnSetClothing(pAvatar->Get(), pAvatar->GetNetId(), pAvatar->GetItems()->GetClothes(), static_cast<int32_t>(colorValue));
    Logger::Print(INFO, "Player {} set skin color to {:08X}", pAvatar->GetRawName(), colorValue);
}
