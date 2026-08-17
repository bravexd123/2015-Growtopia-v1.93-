#pragma once
#include <string>
#include <cstring>
#include <algorithm>
#include <fmt/core.h>
#include <Player/Player.hpp>
#include <World/World.hpp>
#include <Manager/Item/ItemManager.hpp>
#include <Manager/Item/ItemInfo.hpp>
#include <Packet/PacketFactory.hpp>
#include <Packet/VariantFunction.hpp>
#include <Packet/TextFunction.hpp>
#include <ENetWrapper/ENetWrapper.hpp>

namespace LockHelpers {

    inline uint32_t GetLockCapacity(const std::string& itemName) {
        if (itemName == "Small Lock") return 10;
        if (itemName == "Big Lock") return 48;
        if (itemName == "Huge Lock") return 200;
        return 0;
    }
    inline bool IsWorldLockItem(const std::string& itemName) {
        return itemName == "World Lock" || itemName == "Diamond Lock";
    }

    inline void SendLockStateBroadcast(std::shared_ptr<World> pWorld, const LockInfo& lock) {

        TankPacketData t{};
        t.m_type = NET_GAME_PACKET_SEND_LOCK;
        t.m_netId = static_cast<int32_t>(lock.m_ownerId);
        t.m_item = static_cast<int32_t>(lock.m_coveredTiles.size());
        t.m_mainData = lock.m_itemId;
        t.m_tilePositionX = lock.m_anchorX;
        t.m_tilePositionY = lock.m_anchorY;

        std::vector<uint8_t> payload(lock.m_coveredTiles.size() * 2);
        for (size_t i = 0; i < lock.m_coveredTiles.size(); i++) {
            uint16_t v = static_cast<uint16_t>(lock.m_coveredTiles[i]);
            std::memcpy(payload.data() + i * 2, &v, 2);
        }
        SExtendedTankPacket packet(t, payload);
        pWorld->BroadcastPacket(packet);
    }

    inline void SendLockRecalcAck(std::shared_ptr<World> pWorld, const LockInfo& lock) {
        TankPacketData t{};
        t.m_type = NET_GAME_PACKET_SEND_LOCK;
        t.m_netId = static_cast<int32_t>(lock.m_ownerId);
        t.m_mainData = lock.m_itemId;
        t.m_tilePositionX = lock.m_anchorX;
        t.m_tilePositionY = lock.m_anchorY;

        if (lock.m_coveredTiles.empty()) {
            t.m_item = 0;
            SExtendedTankPacket packet(t, std::vector<uint8_t>{});
            pWorld->BroadcastPacket(packet);
            return;
        }
        t.m_item = 1;
        uint16_t v = static_cast<uint16_t>(lock.m_coveredTiles.front());
        std::vector<uint8_t> payload(2);
        std::memcpy(payload.data(), &v, 2);
        SExtendedTankPacket packet(t, payload);
        pWorld->BroadcastPacket(packet);
    }

    inline void SendLockEditDialog(Player* pAvatar, uint32_t tileX, uint32_t tileY, const LockInfo& lock) {
        auto* pItem = GetItemManager()->GetItem(lock.m_itemId);
        std::string lockName = pItem ? pItem->m_name : "Lock";
        std::string accessText = lock.m_accessUserIds.empty()
            ? "Currently, you're the only one with access."
            : fmt::format("Currently, {} other player(s) have access.", lock.m_accessUserIds.size());

        std::string content = fmt::format(
            "\r\nset_default_color|`o\r\n"
            "add_label_with_icon|big|`wEdit {}``|left|{}|\r\n"
            "add_label|small|`wAccess list:``|left|0|\r\n"
            "embed_data|tilex|{}\r\n"
            "embed_data|tiley|{}\r\n"
            "add_spacer|small|\r\n"
            "add_label|small|{}|left|0|\r\n"
            "add_spacer|small|\r\n"
            "add_player_picker|playerNetID|`wAdd``\r\n"
            "add_checkbox|public_lock|Allow anyone to Build and Break|{}\r\n"
            "add_checkbox|ignore_air|Ignore empty air|{}\r\n"
            "add_button|recalcLock|`wRe-apply lock``|noflags|0|0|\r\n"
            "end_dialog|lock_edit|Cancel|OK",
            lockName, lock.m_itemId, tileX, tileY, accessText, lock.m_isPublic ? 1 : 0, lock.m_ignoreAir ? 1 : 0);

        auto vList = VariantList::Create("OnDialogRequest");
        vList.Insert(content);
        ENetWrapper::SendVariantList(pAvatar->Get(), vList);
    }

    inline void SendWorldLockEditDialog(Player* pAvatar, const LockInfo& lock) {
        auto* pItem = GetItemManager()->GetItem(lock.m_itemId);
        std::string lockName = pItem ? pItem->m_name : "World Lock";
        std::string accessText = lock.m_accessUserIds.empty()
            ? "Currently, you're the only one with access."
            : fmt::format("Currently, {} other player(s) have access.", lock.m_accessUserIds.size());

        std::string content = fmt::format(
            "\r\nset_default_color|`o\r\n"
            "add_label_with_icon|big|`wEdit {}``|left|{}|\r\n"
            "add_label|small|`wAccess list:``|left|0|\r\n"
            "add_spacer|small|\r\n"
            "add_label|small|{}|left|0|\r\n"
            "add_spacer|small|\r\n"
            "add_player_picker|playerNetID|`wAdd``\r\n"
            "add_checkbox|public_lock|Allow anyone to Build and Break|{}\r\n"
            "add_checkbox|disable_music|Disable Custom Music Blocks|{}\r\n"
            "add_checkbox|hide_music|Make Custom Music Blocks invisible|{}\r\n"
            "add_button|getWorldKey|`wGet World Key``|noflags|0|0|\r\n"
            "end_dialog|world_lock_edit|Cancel|OK",
            lockName, lock.m_itemId, accessText, lock.m_isPublic ? 1 : 0, lock.m_disableCustomMusic ? 1 : 0, lock.m_hideCustomMusic ? 1 : 0);

        auto vList = VariantList::Create("OnDialogRequest");
        vList.Insert(content);
        ENetWrapper::SendVariantList(pAvatar->Get(), vList);
    }

    inline void SendLockOwnershipBubble(Player* pAvatar, const LockInfo& lock) {
        auto* pItem = GetItemManager()->GetItem(lock.m_itemId);
        std::string lockName = pItem ? pItem->m_name : "Lock";
        std::string ownerName = lock.m_ownerName.empty() ? "Someone" : lock.m_ownerName;

        std::string bracket;
        if (lock.m_isPublic) {
            bracket = "Open to public";
        } else {
            bool hasAccess = std::find(lock.m_accessUserIds.begin(), lock.m_accessUserIds.end(), pAvatar->GetUserId()) != lock.m_accessUserIds.end();
            bracket = hasAccess ? "`2You have access``" : "`4No access``";
        }

        std::string message = IsWorldLockItem(lockName)
            ? fmt::format("`w`w{}``'s `${}``. ({})``", ownerName, lockName, bracket)
            : fmt::format("`w`w{}``'s `${}``. ({})`` (Last played today)", ownerName, lockName, bracket);

        auto vList = VariantList::Create("OnTalkBubble");
        vList.Insert(pAvatar->GetNetId());
        vList.Insert(message);
        vList.Insert(static_cast<int32_t>(0));
        vList.Insert(static_cast<int32_t>(1));
        ENetWrapper::SendVariantList(pAvatar->Get(), vList);
        VarList::OnPlayPositioned(pAvatar->Get(), pAvatar->GetNetId(), "audio/punch_locked.wav");
    }

}
