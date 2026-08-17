#pragma once
#include <fmt/core.h>
#include <Player/Player.hpp>
#include <World/World.hpp>
#include <Manager/Item/ItemManager.hpp>
#include <Manager/Item/ItemInfo.hpp>
#include <Packet/VariantList.hpp>
#include <ENetWrapper/ENetWrapper.hpp>

namespace TileDialogs {

    inline void SendSignEditDialog(Player* pAvatar, uint32_t tileX, uint32_t tileY, const Tile& tile, const ItemInfo* pItem) {
        std::string content = fmt::format(
            "\r\n"
            "embed_data|tilex|{}\r\n"
            "embed_data|tiley|{}\r\n"
            "add_label_with_icon|big|`w{}``|left|{}|\r\n"
            "add_textbox|What do you want this sign to say?|\r\n"
            "add_text_input|sign_text|Text|{}|100|\r\n"
            "end_dialog|sign_edit|Cancel|OK|",
            tileX, tileY,
            pItem ? pItem->m_name : "Sign", tile.m_foregroundId,
            tile.m_signExtra.m_text);

        auto vList = VariantList::Create("OnDialogRequest");
        vList.Insert(content);
        ENetWrapper::SendVariantList(pAvatar->Get(), vList);
    }

    inline void SendDoorEditDialog(Player* pAvatar, uint32_t tileX, uint32_t tileY, const Tile& tile, const ItemInfo* pItem) {
        std::string content = fmt::format(
            "\r\n"
            "embed_data|tilex|{}\r\n"
            "embed_data|tiley|{}\r\n"
            "add_label_with_icon|big|`w{}``|left|{}|\r\n"
            "add_text_input|door_name|Label|{}|100|\r\n"
            "add_text_input|door_target|Destination|{}|24|\r\n"
            "add_text_input|door_id|Door ID|{}|11|\r\n"
            "add_textbox|Leave the destination blank for a door that goes nowhere.|\r\n"
            "end_dialog|door_edit|Cancel|OK|",
            tileX, tileY,
            pItem ? pItem->m_name : "Door", tile.m_foregroundId,
            tile.m_doorExtra.m_label, tile.m_doorExtra.m_target, tile.m_doorExtra.m_targetId);

        auto vList = VariantList::Create("OnDialogRequest");
        vList.Insert(content);
        ENetWrapper::SendVariantList(pAvatar->Get(), vList);
    }

    inline void SendBulletinDialog(Player* pAvatar, uint32_t tileX, uint32_t tileY,
                                   const Tile& tile, const ItemInfo* pItem, bool hasAccess) {
        auto pWorld = pAvatar->GetWorld();
        if (!pWorld)
            return;
        bool isMailbox = pItem && pItem->m_itemType == ITEMTYPE_MAILBOX;
        std::string itemName = pItem ? pItem->m_name : std::string("Bulletin Board");

        std::string posts;
        std::size_t letters = 0, mine = 0;
        const auto& all = pWorld->GetBulletinPosts();
        for (std::size_t i = 0; i < all.size(); i++) {
            const auto& post = all[i];
            if (post.m_x != tileX || post.m_y != tileY)
                continue;
            letters++;
            if (post.m_name.find(pAvatar->GetRawName()) != std::string::npos)
                mine++;
            if (isMailbox) {
                posts += fmt::format("\nadd_label_with_icon|small|`#\"{}\" - `w{}``|left|660|\nadd_spacer|small|", post.m_text, post.m_name);
            } else if (tile.m_bulletinHideNames) {
                posts += fmt::format("\nadd_label|small|`2{}``|left|", post.m_text);
            } else if (hasAccess) {

                posts += fmt::format("\nadd_label_with_icon_button|small|{}: ```2{}``|left|660|edit{}|", post.m_name, post.m_text, i);
            } else {
                posts += fmt::format("\nadd_label_with_icon|small|{}: ```2{}``|left|660|", post.m_name, post.m_text);
            }
        }

        std::string content;
        if (isMailbox) {
            content = fmt::format(
                "set_default_color|`o\nembed_data|tilex|{}\nembed_data|tiley|{}\n"
                "add_label_with_icon|big|`w{}``|left|{}|{}{}\nend_dialog|bulletin_edit|Cancel|{}|\nadd_quick_exit|",
                tileX, tileY, itemName, tile.m_foregroundId,
                hasAccess
                    ? (letters == 0
                        ? std::string("\nadd_textbox|Your mailbox is currently empty.|left|")
                        : fmt::format("\nadd_textbox|You have `w{}`` letters:|left|\nadd_spacer|small|{}\nadd_spacer|small|\nadd_button|clear|`4Empty Mailbox``|noflags|0|0|", letters, posts))
                    : std::string(),
                letters >= World::kMaxBulletinPosts
                    ? std::string("\nadd_textbox|This mailbox already has `w20`` letters in it.  Try again later.|left|")
                    : (!hasAccess && mine >= 3
                        ? std::string("\nadd_textbox|You've already crammed `w3 ``of your letters into the mailbox, better wait.|left|")
                        : fmt::format("\nadd_textbox|{}|left|\nadd_text_input|sign_text|||128|\nadd_spacer|small|\nadd_button|send|`2Send Letter``|noflags|0|0|",
                            hasAccess ? "Write a letter to yourself?" : "Want to leave a message for the owner?")),
                hasAccess ? "" : "");
        } else if (hasAccess) {
            content = fmt::format(
                "set_default_color|`o\nembed_data|tilex|{}\nembed_data|tiley|{}\n"
                "add_label_with_icon|big|`w{}``|left|{}|\nadd_spacer|small|{}"
                "\nadd_textbox|Add to conversation?|left|\nadd_text_input|sign_text|||128|\nadd_spacer|small|"
                "\nadd_button|send|`2Add``|noflags|0|0|\nadd_spacer|small|"
                "\nadd_label_with_icon|big|`wOwner Options|left|242|\nadd_spacer|small|"
                "\nadd_textbox|To remove an individual comment, press the icon to the left of it.|left|"
                "\nadd_spacer|small|\nadd_spacer|small|{}"
                "\nadd_checkbox|checkbox_locked|Public can add|{}"
                "\nadd_checkbox|checkbox_hide|Hide names|{}"
                "\nend_dialog|bulletin_edit|Cancel|OK|\nadd_quick_exit|",
                tileX, tileY, itemName, tile.m_foregroundId,
                letters == 0 ? fmt::format("\nadd_textbox|The {} is empty.|left|", itemName) : posts,
                letters == 0 ? "" : "\nadd_button|clear|`4Clear Board``|noflags|0|0|",
                tile.m_bulletinPublicCanAdd ? "1" : "0",
                tile.m_bulletinHideNames ? "1" : "0");
        } else {
            content = fmt::format(
                "set_default_color|`o\nembed_data|tilex|{}\nembed_data|tiley|{}\n"
                "add_label_with_icon|big|`w{}``|left|{}|\nadd_spacer|small|{}{}"
                "\nend_dialog|bulletin_edit|Cancel||\nadd_quick_exit|",
                tileX, tileY, itemName, tile.m_foregroundId,
                letters == 0 ? fmt::format("\nadd_textbox|The {} is empty.|left|", itemName) : posts,
                !tile.m_bulletinPublicCanAdd
                    ? std::string()
                    : (mine >= 3
                        ? std::string("\nadd_textbox|You already have `03`` posts up, take a break!|left|")
                        : std::string("\nadd_textbox|Add to conversation?|left|\nadd_text_input|sign_text|||128|\nadd_spacer|small|\nadd_button|send|`2Add``|noflags|0|0|")));
        }

        auto vList = VariantList::Create("OnDialogRequest");
        vList.Insert(content);
        ENetWrapper::SendVariantList(pAvatar->Get(), vList);
    }

    inline bool TryOpenTileDialog(Player* pAvatar, uint32_t tileX, uint32_t tileY) {
        auto pWorld = pAvatar->GetWorld();
        if (!pWorld)
            return false;
        Tile* pTile = pWorld->GetTile(tileX, tileY);
        if (!pTile || pTile->IsForegroundEmpty())
            return false;

        auto* pItem = GetItemManager()->GetItem(pTile->m_foregroundId);
        if (!pItem)
            return false;

        bool canEdit = pWorld->CanEdit(tileX, tileY, pAvatar->GetUserId());

        if (pItem->m_itemType == ITEMTYPE_BULLETIN || pItem->m_itemType == ITEMTYPE_MAILBOX) {
            SendBulletinDialog(pAvatar, tileX, tileY, *pTile, pItem, canEdit);
            return true;
        }

        if (!canEdit)
            return false;

        if (pItem->m_itemType == ITEMTYPE_SIGN && pTile->m_hasSignExtra) {
            SendSignEditDialog(pAvatar, tileX, tileY, *pTile, pItem);
            return true;
        }
        if (pItem->IsDoor() && pTile->m_hasDoorExtra) {
            SendDoorEditDialog(pAvatar, tileX, tileY, *pTile, pItem);
            return true;
        }
        return false;
    }

    inline Tile* ResolveEditableTile(Player* pAvatar, uint32_t tileX, uint32_t tileY) {
        auto pWorld = pAvatar->GetWorld();
        if (!pWorld)
            return nullptr;
        if (!pWorld->CanEdit(tileX, tileY, pAvatar->GetUserId()))
            return nullptr;
        return pWorld->GetTile(tileX, tileY);
    }

}
