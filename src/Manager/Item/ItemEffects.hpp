#pragma once
#include <string>
#include <array>
#include <vector>
#include <algorithm>
#include <Manager/Item/ItemManager.hpp>
#include <Manager/Item/ItemInfo.hpp>
#include <Manager/Item/PunchEffects.hpp>
#include <Manager/Item/Playmods.hpp>
#include <Player/PlayerItems.hpp>
#include <Player/Player.hpp>
#include <ctime>

namespace ItemEffects {

    inline bool IsEnhancedDiggingTool(const std::string& itemName) {
        static const std::array<std::string, 9> kNames = {
            "Pickaxe", "Golden Pickaxe", "Demonic Arm", "Death Ray", "Ring Of Force",
            "Flame Scythe", "Atomic Shadow Scythe", "Cybernetic Arm", "Explorer's Shovel"
        };
        return std::find(kNames.begin(), kNames.end(), itemName) != kNames.end();
    }

    inline bool IsDoubleJumpItem(const std::string& itemName) {
        return itemName.find("Wings") != std::string::npos || itemName.find("Cape") != std::string::npos;
    }

    inline bool IsLaserVisor(const std::string& itemName) {
        return itemName == "Cyclopean Visor";
    }

    inline std::vector<std::string> GetEquippedItemNames(PlayerItems* pItems) {
        std::vector<std::string> names;
        const auto& clothes = pItems->GetClothes();
        for (uint16_t itemId : clothes) {
            if (itemId == 0)
                continue;
            auto* pItem = GetItemManager()->GetItem(itemId);
            if (pItem)
                names.push_back(pItem->m_name);
        }
        return names;
    }

    inline bool HasEnhancedDigging(PlayerItems* pItems) {
        for (const auto& name : GetEquippedItemNames(pItems))
            if (IsEnhancedDiggingTool(name))
                return true;
        return false;
    }

    inline bool HasDoubleJump(PlayerItems* pItems) {
        for (const auto& name : GetEquippedItemNames(pItems))
            if (IsDoubleJumpItem(name))
                return true;
        return false;
    }

    inline bool HasLaserVisor(PlayerItems* pItems) {
        for (const auto& name : GetEquippedItemNames(pItems))
            if (IsLaserVisor(name))
                return true;
        return false;
    }

    inline uint8_t GetActivePunchEffect(PlayerItems* pItems) {
        for (uint16_t itemId : pItems->GetClothes()) {
            if (itemId == 0)
                continue;
            if (uint8_t effect = GetPunchEffectId(itemId); effect != 0)
                return effect;
        }
        return 0;
    }

    inline uint8_t GetPunchEffectStateByte(Player* pAvatar) {
        return GetActivePunchEffect(pAvatar->GetItems());
    }

    constexpr uint32_t kStateFlagNoclip = (1u << 0);
    constexpr uint32_t kStateFlagDoubleJump = (1u << 1);
    constexpr uint32_t kStateFlagInvisible = (1u << 2);

    inline uint32_t ComputeCharacterStateFlags(Player* pAvatar) {
        uint32_t flags = 0;
        if (pAvatar->IsGhostEnabled())
            flags |= kStateFlagNoclip;
        if (pAvatar->IsFlagOn(PLAYERFLAG_IS_INVISIBLE))
            flags |= kStateFlagInvisible;
        if (HasDoubleJump(pAvatar->GetItems()))
            flags |= kStateFlagDoubleJump;

        int64_t now = std::time(nullptr);
        for (const auto& mod : pAvatar->GetPlaymods()) {
            if (mod.m_expiry <= now)
                continue;
            const auto* pInfo = Playmods::GetById(mod.m_id);
            if (pInfo && pInfo->m_stateBit >= 0)
                flags |= (1u << pInfo->m_stateBit);
        }
        return flags;
    }

    inline int32_t ComputeSkinColor(Player* pAvatar) {
        int64_t now = std::time(nullptr);
        for (const auto& mod : pAvatar->GetPlaymods()) {
            if (mod.m_expiry <= now)
                continue;
            const auto* pInfo = Playmods::GetById(mod.m_id);
            if (pInfo && pInfo->m_skinColour != 0)
                return static_cast<int32_t>(pInfo->m_skinColour);
        }
        return static_cast<int32_t>(pAvatar->GetItems()->GetSkinColor().GetInt());
    }

}
