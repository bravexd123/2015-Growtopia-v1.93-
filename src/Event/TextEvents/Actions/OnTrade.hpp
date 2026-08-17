#pragma once
#include <cstdint>
#include <cstdlib>
#include <fmt/core.h>
#include <Event/EventType.hpp>
#include <Player/Player.hpp>
#include <World/World.hpp>
#include <Packet/VariantFunction.hpp>
#include <ENetWrapper/ENetWrapper.hpp>
#include <Packet/VariantList.hpp>
#include <Manager/Item/ItemManager.hpp>
#include <Manager/Trade/TradeManager.hpp>

ACTION_EVENT("mod_trade", OnModTrade) {
    auto* pPartner = TradeManager::FindTradePartner(pAvatar);
    if (!pPartner)
        return;

    int32_t itemId = eventParser.Get<int32_t>("itemID", 1);
    if (itemId <= 0)
        return;

    auto* pItemInfo = GetItemManager()->GetItem(itemId);
    if (!pItemInfo)
        return;

    uint8_t haveCount = 0;
    if (auto it = pAvatar->GetItems()->m_bpItems.find(static_cast<uint16_t>(itemId)); it != pAvatar->GetItems()->m_bpItems.end())
        haveCount = it->second;
    if (haveCount == 0)
        return;

    if (haveCount == 1) {
        TradeManager::SetOfferedItem(pAvatar, pPartner, static_cast<uint16_t>(itemId), 1);
        return;
    }

    auto vList = VariantList::Create("OnDialogRequest");
    vList.Insert(fmt::format(
        "set_default_color|`o\nadd_label_with_icon|big|`2Trade`` `w{}``|left|{}|\n"
        "add_textbox|`2Trade how many?``|left|\nadd_text_input|count||{}|5|\n"
        "embed_data|itemID|{}\nend_dialog|trade_item|Cancel|OK|",
        pItemInfo->m_name, itemId, haveCount, itemId));
    ENetWrapper::SendVariantList(pAvatar->Get(), vList);
}

ACTION_EVENT("rem_trade", OnRemTrade) {
    auto* pPartner = TradeManager::FindTradePartner(pAvatar);
    if (!pPartner)
        return;

    int32_t itemId = eventParser.Get<int32_t>("itemID", 1);
    if (itemId <= 0)
        return;

    TradeManager::SetOfferedItem(pAvatar, pPartner, static_cast<uint16_t>(itemId), 0);
}

ACTION_EVENT("trade_accept", OnTradeAccept) {
    auto* pPartner = TradeManager::FindTradePartner(pAvatar);
    if (!pPartner)
        return;

    int32_t value = 0;
    {
        size_t lineStart = eventData.find('\n');
        if (lineStart != std::string::npos) {
            lineStart++;
            size_t lineEnd = eventData.find('\n', lineStart);
            std::string line = eventData.substr(lineStart, lineEnd == std::string::npos ? std::string::npos : lineEnd - lineStart);
            size_t pipePos = line.find('|');
            std::string valueText = (pipePos != std::string::npos) ? line.substr(pipePos + 1) : line;
            if (!valueText.empty())
                value = std::atoi(valueText.c_str());
        }
    }
    pAvatar->SetTradeAccepted(value == 1);
    TradeManager::SyncOffer(pAvatar, pPartner);

    if (pAvatar->HasAcceptedTrade() && pPartner->HasAcceptedTrade()) {
        auto vListSelf = VariantList::Create("OnDialogRequest");
        vListSelf.Insert(TradeManager::BuildConfirmDialog(pAvatar, pPartner));
        ENetWrapper::SendVariantList(pAvatar->Get(), vListSelf);

        auto vListPartner = VariantList::Create("OnDialogRequest");
        vListPartner.Insert(TradeManager::BuildConfirmDialog(pPartner, pAvatar));
        ENetWrapper::SendVariantList(pPartner->Get(), vListPartner);
    }
}

ACTION_EVENT("trade_cancel", OnTradeCancel) {
    TradeManager::CancelTrade(pAvatar);
}

DIALOG_EVENT("trade_item", OnTradeItemDialog) {
    auto* pPartner = TradeManager::FindTradePartner(pAvatar);
    if (!pPartner)
        return;

    int32_t itemId = eventParser.Get<int32_t>("itemID", 1);
    int32_t count = eventParser.Get<int32_t>("count", 1);
    if (itemId <= 0 || count <= 0)
        return;

    uint8_t haveCount = 0;
    if (auto it = pAvatar->GetItems()->m_bpItems.find(static_cast<uint16_t>(itemId)); it != pAvatar->GetItems()->m_bpItems.end())
        haveCount = it->second;
    if (count > haveCount)
        return;

    TradeManager::SetOfferedItem(pAvatar, pPartner, static_cast<uint16_t>(itemId), static_cast<uint8_t>(count));
}

DIALOG_EVENT("trade_confirm", OnTradeConfirmDialog) {
    auto* pPartner = TradeManager::FindTradePartner(pAvatar);
    if (!pPartner)
        return;

    std::string button = eventParser.Get("buttonClicked", 1);
    if (button == "back") {
        TradeManager::CancelTrade(pAvatar);
        return;
    }
    if (button != "accept")
        return;

    if (!pAvatar->HasAcceptedTrade() || !pPartner->HasAcceptedTrade())
        return;

    pAvatar->SetTradeConfirmed(true);
    if (!pPartner->HasConfirmedTrade())
        return;

    if (!TradeManager::CanAffordOffer(pAvatar) || !TradeManager::CanAffordOffer(pPartner)) {
        VarList::OnConsoleMessage(pAvatar->Get(), "The trade was cancelled - an offered item is no longer available.");
        VarList::OnConsoleMessage(pPartner->Get(), "The trade was cancelled - an offered item is no longer available.");
        TradeManager::CancelTrade(pAvatar);
        return;
    }

    if (!TradeManager::HasSpaceForOffer(pAvatar, pPartner) || !TradeManager::HasSpaceForOffer(pPartner, pAvatar)) {
        VarList::OnConsoleMessage(pAvatar->Get(), "The trade was cancelled - not enough backpack space.");
        VarList::OnConsoleMessage(pPartner->Get(), "The trade was cancelled - not enough backpack space.");
        TradeManager::CancelTrade(pAvatar);
        return;
    }

    TradeManager::FinalizeTrade(pAvatar, pPartner);
}
