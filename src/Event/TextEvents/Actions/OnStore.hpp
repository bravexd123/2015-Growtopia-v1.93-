#pragma once
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <cstdlib>
#include <fmt/core.h>
#include <Event/EventType.hpp>
#include <Player/Player.hpp>
#include <Manager/Item/ItemManager.hpp>
#include <Manager/Item/ItemInfo.hpp>
#include <Manager/Item/ItemComponent.hpp>
#include <Manager/Store/StoreCatalog.hpp>
#include <Manager/Database/Database.hpp>
#include <Utils/MiscUtils.hpp>
#include <Packet/PacketFactory.hpp>
#include <Packet/VariantFunction.hpp>
#include <Packet/TextFunction.hpp>
#include <ENetWrapper/ENetWrapper.hpp>
#include <Logger/Logger.hpp>

namespace {

    void SendInventoryGain(Player* pAvatar, uint16_t itemId, uint8_t gainedCount) {
        TankPacketData t{};
        t.m_type = NET_GAME_PACKET_MODIFY_ITEM_INVENTORY;
        t.m_gainedItemCount = gainedCount;
        t.m_mainData = itemId;
        STankPacket packet(t);
        ENetWrapper::SendPacket(pAvatar->Get(), packet);
    }

    void SendStoreError(Player* pAvatar, const std::string& message) {
        VarList::OnStorePurchaseResult(pAvatar->Get(), message);
        VarList::OnConsoleMessage(pAvatar->Get(), message);
    }

    bool WouldOverflowBackpack(PlayerItems* pItems, const std::vector<std::pair<ItemInfo*, uint32_t>>& grants) {
        std::unordered_set<uint16_t> newItemIds;
        for (const auto& [pGrantItem, count] : grants) {
            if (!pItems->m_bpItems.count(static_cast<uint16_t>(pGrantItem->m_Id)))
                newItemIds.insert(static_cast<uint16_t>(pGrantItem->m_Id));
        }
        return pItems->m_bpItems.size() + newItemIds.size() > pItems->m_backpackSpace;
    }

    void FinalizePurchase(Player* pAvatar, const std::string& key, const std::string& displayLabel,
                           uint32_t cost, bool useGrowtokens,
                           const std::vector<std::pair<ItemInfo*, uint32_t>>& grants) {
        auto* pItems = pAvatar->GetItems();
        if (useGrowtokens)
            pItems->RemoveItem(ITEM_GROWTOKEN, static_cast<uint8_t>(cost));
        else
            pItems->SetGems(pItems->GetGems() - static_cast<int32_t>(cost));

        std::string receivedList;
        for (const auto& [pGrantItem, count] : grants) {
            pItems->AddItem(pGrantItem->m_Id, static_cast<uint8_t>(count));
            SendInventoryGain(pAvatar, static_cast<uint16_t>(pGrantItem->m_Id), static_cast<uint8_t>(count));
            if (!receivedList.empty())
                receivedList += ", ";
            receivedList += (count > 1) ? fmt::format("{} {}", count, pGrantItem->m_name) : pGrantItem->m_name;
            CAction::Log(pAvatar->Get(), "Got {} `#{}``.", count, pGrantItem->m_name);
        }

        if (!useGrowtokens)
            VarList::OnSetBux(pAvatar->Get(), pItems->GetGems());

        std::string currencyName = useGrowtokens ? "Growtokens" : "Gems";
        uint32_t remaining = useGrowtokens
            ? (pItems->m_bpItems.count(ITEM_GROWTOKEN) ? pItems->m_bpItems.at(ITEM_GROWTOKEN) : 0)
            : static_cast<uint32_t>(pItems->GetGems());

        std::string costText = Utils::FormatWithCommas(static_cast<int64_t>(cost));
        std::string remainingText = Utils::FormatWithCommas(static_cast<int64_t>(remaining));

        if (!receivedList.empty()) {
            VarList::OnStorePurchaseResult(pAvatar->Get(), fmt::format(
                "You've purchased `o{} `wfor `${} `w{}.\nYou have `${} `w{} left.\n\n`5Received: ``{} ",
                displayLabel, costText, currencyName, remainingText, currencyName, receivedList));
        } else {
            VarList::OnStorePurchaseResult(pAvatar->Get(), fmt::format(
                "You've purchased `o{} `wfor `${} `w{}.\nYou have `${} `w{} left.",
                displayLabel, costText, currencyName, remainingText, currencyName));
        }
        CAction::PlaySFX(pAvatar->Get(), "piano_nice", 0);
        VarList::OnConsoleMessage(pAvatar->Get(), fmt::format(
            "You've purchased {} for `${}`` {}.  You have `${}`` {} left.",
            displayLabel, costText, currencyName, remainingText, currencyName));

        GetDatabase()->GetPlayerTable()->Save(pAvatar);
        Logger::Print(INFO, "Player {} purchased store item '{}' for {} {}", pAvatar->GetRawName(), key, cost, currencyName);
    }
}

ACTION_EVENT("store", OnStoreOpen) {
    VarList::OnStoreRequest(pAvatar->Get(), StoreCatalog::GetCategoryContent().at("main"));
}

ACTION_EVENT("buy", OnBuy) {
    std::string key = eventParser.Get("item", 1);
    if (key.empty())
        return;

    if (key == "token") {
        auto* pItems = pAvatar->GetItems();
        auto bpIt = pItems->m_bpItems.find(ITEM_GROWTOKEN);
        int growtokens = (bpIt != pItems->m_bpItems.end()) ? bpIt->second : 0;
        VarList::OnStoreRequest(pAvatar->Get(), StoreCatalog::GetTokenMenuContent(growtokens));
        return;
    }

    const auto& categories = StoreCatalog::GetCategoryContent();
    if (auto it = categories.find(key); it != categories.end()) {
        if (key == "locks") {

            VarList::OnStoreRequest(pAvatar->Get(), StoreCatalog::GetLocksMenuContent(pAvatar->GetItems()->m_backpackSpace));
        } else {
            VarList::OnStoreRequest(pAvatar->Get(), it->second);
        }
        return;
    }

    if (StoreCatalog::IsGemPackage(key) || StoreCatalog::IsUnimplementedPack(key)) {
        SendStoreError(pAvatar, "This item is not available for purchase on this server yet.");
        return;
    }

    if (key == "upgrade_backpack") {
        auto* pItems = pAvatar->GetItems();
        auto cost = StoreCatalog::GetBackpackUpgradeCost(pItems->m_backpackSpace);
        if (!cost.has_value()) {
            SendStoreError(pAvatar, "Your backpack is already at its maximum size.");
            return;
        }
        if (static_cast<uint32_t>(pItems->GetGems()) < *cost) {
            SendStoreError(pAvatar, "You don't have enough Gems for that.");
            return;
        }

        pItems->SetGems(pItems->GetGems() - static_cast<int32_t>(*cost));
        pItems->m_backpackSpace += 10;
        PlayerItems::SendInventoryState(pAvatar);
        VarList::OnSetBux(pAvatar->Get(), pItems->GetGems());

        VarList::OnStorePurchaseResult(pAvatar->Get(), fmt::format(
            "You've purchased `oUpgrade Backpack `wfor `${} `wGems.\nYou have `${} `wGems left.\n\n`5Received: ``10 Backpack Slots ",
            *cost, pItems->GetGems()));
        CAction::PlaySFX(pAvatar->Get(), "piano_nice", 0);
        VarList::OnConsoleMessage(pAvatar->Get(), fmt::format(
            "You've purchased Upgrade Backpack for `${}`` Gems.  You have `${}`` Gems left.",
            *cost, pItems->GetGems()));

        VarList::OnStoreRequest(pAvatar->Get(), StoreCatalog::GetLocksMenuContent(pItems->m_backpackSpace));

        GetDatabase()->GetPlayerTable()->Save(pAvatar);
        Logger::Print(INFO, "Player {} upgraded backpack to {} slots for {} gems", pAvatar->GetRawName(), pItems->m_backpackSpace, *cost);
        return;
    }

    if (const auto& randomPacks = StoreCatalog::GetRandomPacks(); randomPacks.count(key)) {
        const StoreCatalog::RandomPack& pack = randomPacks.at(key);
        auto* pItems = pAvatar->GetItems();
        uint32_t cost = static_cast<uint32_t>(pack.price);

        if (static_cast<uint32_t>(pItems->GetGems()) < cost) {
            SendStoreError(pAvatar, "You don't have enough Gems for that.");
            return;
        }

        std::vector<ItemInfo*> pool;
        if (!pack.curatedPool.empty()) {
            for (const auto& name : pack.curatedPool) {
                ItemInfo* pPoolItem = GetItemManager()->GetItemByName(name);
                if (pPoolItem)
                    pool.push_back(pPoolItem);
            }
        } else if (pack.seedPool) {
            pool = GetItemManager()->GetSeedsInRarityRange(pack.minRarity, pack.maxRarity);
        } else if (pack.clothingPool) {
            pool = GetItemManager()->GetClothingInRarityRange(pack.minRarity, pack.maxRarity);
        }
        if (pool.empty()) {
            Logger::Print(WARNING, "Store random pack {} has an empty item pool - aborting.", key);
            SendStoreError(pAvatar, "That item isn't available on this server yet.");
            return;
        }

        std::unordered_map<uint16_t, uint32_t> grantCounts;
        for (const auto& [name, count] : pack.fixedGrants) {
            ItemInfo* pFixedItem = GetItemManager()->GetItemByName(name);
            if (!pFixedItem) {
                Logger::Print(WARNING, "Store random pack {} references unknown fixed item '{}' - aborting.", key, name);
                SendStoreError(pAvatar, "That item isn't available on this server yet.");
                return;
            }
            grantCounts[static_cast<uint16_t>(pFixedItem->m_Id)] += count;
        }
        for (uint8_t i = 0; i < pack.pickCount; i++) {
            ItemInfo* pPicked = pool[static_cast<size_t>(std::rand()) % pool.size()];
            grantCounts[static_cast<uint16_t>(pPicked->m_Id)] += pack.perPickCount;
        }
        if (pack.hasBonusPick) {

            std::vector<ItemInfo*> bonusPool = GetItemManager()->GetSeedsInRarityRange(pack.bonusMinRarity, pack.bonusMaxRarity);
            if (!bonusPool.empty()) {
                ItemInfo* pPicked = bonusPool[static_cast<size_t>(std::rand()) % bonusPool.size()];
                grantCounts[static_cast<uint16_t>(pPicked->m_Id)] += 1;
            }
        }

        std::vector<std::pair<ItemInfo*, uint32_t>> grants;
        for (const auto& [itemId, count] : grantCounts)
            grants.emplace_back(GetItemManager()->GetItem(itemId), count);

        if (WouldOverflowBackpack(pItems, grants)) {
            SendStoreError(pAvatar, "Your backpack is full! Free up some space and try again.");
            return;
        }

        std::string displayLabel = grants.empty() ? key : grants.front().first->m_name;
        FinalizePurchase(pAvatar, key, displayLabel, cost, false, grants);
        return;
    }

    const auto& catalog = StoreCatalog::GetPurchasableItems();
    auto entry = catalog.find(key);
    if (entry == catalog.end()) {
        CAction::Log(pAvatar->Get(), "`oUnhandled store purchase, key(`w{}``)``", key);
        return;
    }

    const StoreCatalog::StoreItem& item = entry->second;
    auto* pItems = pAvatar->GetItems();
    bool useGrowtokens = item.price < 0;
    uint32_t cost = static_cast<uint32_t>(std::abs(item.price));

    if (useGrowtokens) {
        auto bpIt = pItems->m_bpItems.find(ITEM_GROWTOKEN);
        uint32_t have = (bpIt != pItems->m_bpItems.end()) ? bpIt->second : 0;
        if (have < cost) {
            SendStoreError(pAvatar, "You don't have enough Growtokens for that.");
            return;
        }
    } else if (static_cast<uint32_t>(pItems->GetGems()) < cost) {
        SendStoreError(pAvatar, "You don't have enough Gems for that.");
        return;
    }

    std::vector<std::pair<ItemInfo*, uint32_t>> resolvedGrants;
    for (const auto& [name, count] : item.grants) {
        ItemInfo* pGrantItem = GetItemManager()->GetItemByName(name);
        if (!pGrantItem) {
            Logger::Print(WARNING, "Store purchase {} references unknown item '{}' - aborting.", key, name);
            SendStoreError(pAvatar, "That item isn't available on this server yet.");
            return;
        }
        resolvedGrants.emplace_back(pGrantItem, count);
    }

    if (WouldOverflowBackpack(pItems, resolvedGrants)) {
        SendStoreError(pAvatar, "Your backpack is full! Free up some space and try again.");
        return;
    }

    std::string displayLabel = resolvedGrants.empty() ? key : resolvedGrants.front().first->m_name;
    FinalizePurchase(pAvatar, key, displayLabel, cost, useGrowtokens, resolvedGrants);
}
