#pragma once
#include <string>
#include <vector>
#include <algorithm>
#include <fmt/core.h>
#include <Player/Player.hpp>
#include <World/World.hpp>
#include <Packet/VariantFunction.hpp>
#include <Packet/TextFunction.hpp>
#include <Manager/Item/ItemManager.hpp>

namespace TradeManager {
    constexpr size_t kMaxTradeSlots = 3;

    inline Player* FindTradePartner(Player* pAvatar) {
        int32_t partnerNetId = pAvatar->GetTradingWithNetId();
        if (partnerNetId == -1)
            return nullptr;
        auto pWorld = pAvatar->GetWorld();
        if (!pWorld)
            return nullptr;
        for (auto* pOther : pWorld->GetPlayers()) {
            if (pOther->GetNetId() == partnerNetId && pOther->GetTradingWithNetId() == pAvatar->GetNetId())
                return pOther;
        }
        return nullptr;
    }

    inline std::string BuildOfferSlots(Player* pAvatar) {
        std::string out;
        for (auto& [itemId, count] : pAvatar->GetTradeOffer())
            out += fmt::format("add_slot|{}|{}\n", itemId, count);
        return out;
    }

    inline void EndTrade(Player* pAvatar, Player* pPartner, bool notify = true) {
        pAvatar->SetTradingWithNetId(-1);
        pAvatar->ClearTradeOffer();
        pAvatar->SetTradeAccepted(false);
        pAvatar->SetTradeConfirmed(false);
        if (notify)
            VarList::OnForceTradeEnd(pAvatar->Get());

        if (pPartner) {
            pPartner->SetTradingWithNetId(-1);
            pPartner->ClearTradeOffer();
            pPartner->SetTradeAccepted(false);
            pPartner->SetTradeConfirmed(false);
            if (notify)
                VarList::OnForceTradeEnd(pPartner->Get());
        }
    }

    inline void CancelTrade(Player* pAvatar, bool notify = true) {
        EndTrade(pAvatar, FindTradePartner(pAvatar), notify);
    }

    inline void SyncOffer(Player* pAvatar, Player* pPartner);

    inline void RequestTrade(Player* pAvatar, Player* pTarget) {
        bool isMutual = pTarget->GetTradingWithNetId() == pAvatar->GetNetId();

        pAvatar->SetTradingWithNetId(pTarget->GetNetId());
        pAvatar->SetTradeAccepted(false);
        pAvatar->SetTradeConfirmed(false);
        pAvatar->ClearTradeOffer();

        VarList::OnStartTrade(pAvatar->Get(), pTarget->GetFormattedName(), pTarget->GetNetId());

        if (!isMutual) {
            CAction::PlaySFX(pTarget->Get(), "cash_register", 0);
            VarList::OnConsoleMessage(pTarget->Get(),
                fmt::format("`#TRADE ALERT:`` `w{}`` wants to trade with you! To start, type `w/trade {}", pAvatar->GetFormattedName(), pAvatar->GetRawName()));
            return;
        }

        pTarget->SetTradingWithNetId(pAvatar->GetNetId());
        pTarget->SetTradeAccepted(false);
        pTarget->SetTradeConfirmed(false);
        pTarget->ClearTradeOffer();

        VarList::OnStartTrade(pTarget->Get(), pAvatar->GetFormattedName(), pAvatar->GetNetId());
        SyncOffer(pAvatar, pTarget);
        SyncOffer(pTarget, pAvatar);
    }

    inline bool IsBusyWithSomeoneElse(Player* pAvatar, Player* pTarget) {
        return (pAvatar->GetTradingWithNetId() != -1 && pAvatar->GetTradingWithNetId() != pTarget->GetNetId())
            || (pTarget->GetTradingWithNetId() != -1 && pTarget->GetTradingWithNetId() != pAvatar->GetNetId());
    }

    inline void SyncOffer(Player* pAvatar, Player* pPartner) {
        std::string title = fmt::format("`o{}'s offer.``", pAvatar->GetFormattedName());
        std::string slots = BuildOfferSlots(pAvatar);
        std::string accepted = pAvatar->HasAcceptedTrade() ? "1" : "0";

        VarList::OnTradeStatus(pAvatar->Get(), pAvatar->GetNetId(), title, "locked|0\nreset_locks|1\naccepted|0");
        VarList::OnTradeStatus(pPartner->Get(), pAvatar->GetNetId(), title, "locked|0\nreset_locks|1\naccepted|0");
        VarList::OnTradeStatus(pAvatar->Get(), pAvatar->GetNetId(), title, slots + "locked|0\naccepted|" + accepted);
        VarList::OnTradeStatus(pPartner->Get(), pAvatar->GetNetId(), title, slots + "locked|0\naccepted|" + accepted);
    }

    inline uint8_t OfferedCount(Player* pAvatar, uint16_t itemId) {
        for (auto& [id, count] : pAvatar->GetTradeOffer()) {
            if (id == itemId)
                return count;
        }
        return 0;
    }

    inline void SetOfferedItem(Player* pAvatar, Player* pPartner, uint16_t itemId, uint8_t count) {
        auto& offer = pAvatar->GetTradeOffer();
        auto it = std::find_if(offer.begin(), offer.end(), [&](auto& entry) { return entry.first == itemId; });

        if (count == 0) {
            if (it != offer.end())
                offer.erase(it);
        }
        else if (it != offer.end()) {
            it->second = count;
        }
        else {
            if (offer.size() >= kMaxTradeSlots)
                return;
            offer.emplace_back(itemId, count);
        }

        pAvatar->SetTradeAccepted(false);
        pPartner->SetTradeAccepted(false);
        SyncOffer(pAvatar, pPartner);

        CAction::PlaySFX(pAvatar->Get(), "tile_removed", 0);
        CAction::PlaySFX(pPartner->Get(), "tile_removed", 0);
        VarList::OnTextOverlay(pAvatar->Get(), "The deal has changed");
        VarList::OnTextOverlay(pPartner->Get(), "The deal has changed");
    }

    inline bool HasSpaceForOffer(Player* pGiver, Player* pReceiver) {
        auto* pItems = pReceiver->GetItems();
        auto simulated = pItems->m_bpItems;
        size_t usedSlots = simulated.size();

        for (auto& [itemId, count] : pGiver->GetTradeOffer()) {
            auto* pItemInfo = GetItemManager()->GetItem(itemId);
            if (!pItemInfo)
                return false;
            if (auto it = simulated.find(itemId); it != simulated.end()) {
                if (static_cast<int>(it->second) + count > pItemInfo->m_maxAmount)
                    return false;
            }
            else {
                if (usedSlots >= pItems->m_backpackSpace)
                    return false;
                usedSlots++;
                simulated.insert_or_assign(itemId, count);
            }
        }
        return true;
    }

    inline bool CanAffordOffer(Player* pGiver) {
        auto& bpItems = pGiver->GetItems()->m_bpItems;
        for (auto& [itemId, count] : pGiver->GetTradeOffer()) {
            auto it = bpItems.find(itemId);
            if (it == bpItems.end() || it->second < count)
                return false;
        }
        return true;
    }

    inline std::string BuildConfirmDialog(Player* pSelf, Player* pOther) {
        auto describe = [](Player* pOwner) {
            auto& offer = pOwner->GetTradeOffer();
            if (offer.empty())
                return std::string("\nadd_textbox|`4Nothing!``|left|");
            std::string out;
            for (auto& [itemId, count] : offer) {
                auto* pItemInfo = GetItemManager()->GetItem(itemId);
                out += fmt::format("\nadd_label_with_icon|small|(`w{}``) {}|left|{}|",
                    count, pItemInfo ? pItemInfo->m_name : "Unknown Item", itemId);
            }
            return out;
        };

        std::string scamWarning = pOther->GetTradeOffer().empty()
            ? "\nadd_spacer|small|\nadd_textbox|`4SCAM WARNING: ``You are about to do a trade without receiving anything in return. Once you do the trade you cannot get the items back.|left|\nadd_textbox|`4Do you really want to do this?``|left|\nadd_spacer|small|"
            : "";

        return fmt::format(
            "set_default_color|`o\nadd_label_with_icon|big|`wTrade Confirmation``|left|1366|\n"
            "add_spacer|small|\nadd_textbox|`4You'll give:``|left|\nadd_spacer|small|{}\n"
            "add_spacer|small|\nadd_textbox|`2You'll get:``|left|\nadd_spacer|small|{}{}\n"
            "add_button|accept|Do The Trade!|noflags|0|0|\nadd_button|back|Cancel|noflags|0|0|\nend_dialog|trade_confirm|||",
            describe(pSelf), describe(pOther), scamWarning);
    }

    inline void FinalizeTrade(Player* pA, Player* pB) {
        auto describeTransfer = [](Player* pGiver, Player* pReceiver) {
            std::string out;
            for (auto& [itemId, count] : pGiver->GetTradeOffer()) {
                if (!pGiver->GetItems()->RemoveItem(itemId, count))
                    continue;
                pReceiver->GetItems()->AddItem(itemId, count);
                auto* pItemInfo = GetItemManager()->GetItem(itemId);
                out += (out.empty() ? "" : ", ") + std::to_string(count) + " " + (pItemInfo ? pItemInfo->m_name : "Unknown Item");
            }
            return out.empty() ? std::string("nothing") : out;
        };

        std::string aGave = describeTransfer(pA, pB);
        std::string bGave = describeTransfer(pB, pA);

        PlayerItems::SendInventoryState(pA);
        PlayerItems::SendInventoryState(pB);

        auto pWorld = pA->GetWorld();
        if (pWorld) {
            std::string msgA = fmt::format("`1{} traded {} to {}.``", pA->GetFormattedName(), aGave, pB->GetFormattedName());
            std::string msgB = fmt::format("`1{} traded {} to {}.``", pB->GetFormattedName(), bGave, pA->GetFormattedName());
            for (auto* pOther : pWorld->GetPlayers()) {
                VarList::OnConsoleMessage(pOther->Get(), msgA);
                VarList::OnConsoleMessage(pOther->Get(), msgB);
            }
        }

        VarList::OnPlayPositioned(pA->Get(), pA->GetNetId(), "audio/keypad_hit.wav");
        VarList::OnPlayPositioned(pB->Get(), pB->GetNetId(), "audio/keypad_hit.wav");

        EndTrade(pA, pB, true);
    }
}
