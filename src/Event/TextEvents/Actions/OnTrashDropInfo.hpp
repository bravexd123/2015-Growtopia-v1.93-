#pragma once
#include <fmt/core.h>
#include <Event/EventType.hpp>
#include <Player/Player.hpp>
#include <Player/PlayerDialog/PlayerDialog.hpp>
#include <Manager/Item/ItemManager.hpp>
#include <Manager/Item/ItemInfo.hpp>
#include <Utils/TextParse.hpp>

namespace {
    ItemInfo* GetHeldItem(Player* pAvatar, TextParse& eventParser, uint16_t& outItemId, uint8_t& outHeld) {
        outItemId = eventParser.Get<uint16_t>("itemID", 1);
        auto it = pAvatar->GetItems()->m_bpItems.find(outItemId);
        if (it == pAvatar->GetItems()->m_bpItems.end())
            return nullptr;
        outHeld = it->second;
        return GetItemManager()->GetItem(outItemId);
    }
}

ACTION_EVENT("trash", OnTrash) {
    uint16_t itemId; uint8_t held;
    auto* pItem = GetHeldItem(pAvatar, eventParser, itemId, held);
    if (!pItem || held == 0)
        return;
    if (pItem->IsUntradable()) {

        VarList::OnTextOverlay(pAvatar->Get(), "You'd be sorry if you lost that!");
        return;
    }

    pAvatar->PlayerDialog::Send(DIALOG_TYPE_TRASH_ITEM, TextParse({
        { "itemId",     std::to_string(itemId) },
        { "itemName",   pItem->m_name           },
        { "have",       std::to_string(held)    },
        { "accountTier", std::to_string(static_cast<int>(pAvatar->GetAccountTier())) }
        }));
}

ACTION_EVENT("drop", OnDrop) {
    uint16_t itemId; uint8_t held;
    auto* pItem = GetHeldItem(pAvatar, eventParser, itemId, held);
    if (!pItem || held == 0)
        return;
    if (pItem->IsUntradable()) {

        VarList::OnTextOverlay(pAvatar->Get(), "You'd be sorry if you lost that!");
        return;
    }

    pAvatar->PlayerDialog::Send(DIALOG_TYPE_DROP_ITEM, TextParse({
        { "itemId",   std::to_string(itemId) },
        { "itemName", pItem->m_name           },
        { "have",     std::to_string(held)    }
        }));
}

ACTION_EVENT("info", OnItemInfo) {
    uint16_t itemId; uint8_t held;
    auto* pItem = GetHeldItem(pAvatar, eventParser, itemId, held);
    if (!pItem)
        return;

    std::string description;
    if (pItem->IsSeed()) {
        auto* pBlock = GetItemManager()->GetItem(itemId - 1);
        description = fmt::format("Plant this seed to grow a `w{} Tree``.", pBlock ? pBlock->m_name : pItem->m_name);
    } else {
        std::string real = GetItemManager()->GetDescription(itemId);
        description = !real.empty() ? real : fmt::format("`w{}``", pItem->m_name);
    }

    std::vector<std::pair<std::string, std::string>> fields{
        { "itemId",      std::to_string(itemId)          },
        { "itemName",    pItem->m_name                   },
        { "description", description                     },
        { "rarity",      std::to_string(pItem->m_rarity) }
    };
    {
        uint16_t props = pItem->m_properties;
        std::vector<std::string> lines;
        if (pItem->IsLock())
            lines.emplace_back("`oA lock makes it so only you (and designated friends) can edit an area.``");
        if (props & ITEMFLAG1_FLIPPED)
            lines.emplace_back("`1This item can be placed in two directions, depending on the direction you're facing.``");
        if (props & ITEMFLAG1_EDITABLE)
            lines.emplace_back("`1This item has special properties you can adjust with the Wrench.``");
        if (props & ITEMFLAG1_SEEDLESS)
            lines.emplace_back("`1This item never drops any seeds.``");
        if (props & ITEMFLAG1_PERMANENT)
            lines.emplace_back("`1This item can't be destroyed - smashing it will return it to your backpack if you have room!``");
        if (props & ITEMFLAG1_WORLD_LOCK)
            lines.emplace_back("`1This item can only be used in World-Locked worlds.``");
        if ((props >> 8) & ITEMFLAG2_MOD)
            lines.emplace_back("`1This item is only available to Growtopia moderators.``");
        if ((props >> 8) & ITEMFLAG2_RANDGROW)
            lines.emplace_back("`1A tree of this type can bear surprising fruit!``");
        if ((props >> 8) & ITEMFLAG2_PUBLIC)
            lines.emplace_back("`1This item is PUBLIC: Even if it's locked, anyone can smash it.``");
        if ((props >> 8) & ITEMFLAG2_HOLIDAY)
            lines.emplace_back("`1This item can only be created during WinterFest/Halloween.``");
        if ((props >> 8) & ITEMFLAG2_UNTRADABLE)
            lines.emplace_back("`1This item cannot be dropped or traded.``");

        for (std::size_t i = 0; i < lines.size(); i++)
            fields.push_back({ fmt::format("prop{}", i), lines[i] });
    }
    pAvatar->PlayerDialog::Send(DIALOG_TYPE_ITEM_INFO, TextParse(fields));
}

ACTION_EVENT("itemfavourite", OnItemFavourite) {
    uint16_t itemId = eventParser.Get<uint16_t>("itemID", 1);
    if (itemId == 0)
        return;

    auto& favourites = pAvatar->GetFavouriteItems();
    auto it = std::find(favourites.begin(), favourites.end(), itemId);
    bool alreadyFavourite = it != favourites.end();

    if (!alreadyFavourite && favourites.size() >= 20) {
        const char* message = "You cannot favorite any more items. Remove some from your list and try again.";
        VarList::OnTalkBubble(pAvatar->Get(), pAvatar->GetNetId(), message);
        VarList::OnConsoleMessage(pAvatar->Get(), message);
        return;
    }

    VarList::OnFavItemUpdated(pAvatar->Get(), itemId, alreadyFavourite ? 0 : 1);
    if (alreadyFavourite)
        favourites.erase(it);
    else
        favourites.push_back(itemId);
}
