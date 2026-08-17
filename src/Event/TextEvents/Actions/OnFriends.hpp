#pragma once
#include <cstdint>
#include <string>
#include <fmt/core.h>
#include <Event/EventType.hpp>
#include <Player/Player.hpp>
#include <World/World.hpp>
#include <Packet/VariantFunction.hpp>
#include <Commands/CommandUtils.hpp>
#include <Manager/Database/Database.hpp>
#include <Logger/Logger.hpp>

namespace FriendDialog {
    inline void SendFriendsList(Player* pAvatar, bool showAll) {
        const auto& friends = pAvatar->GetFriends();

        int onlineCount = 0;
        std::string rows;
        for (uint32_t friendId : friends) {
            Player* pOnline = CommandUtils::FindOnlinePlayer(std::to_string(friendId));
            if (pOnline) {
                onlineCount++;
                rows += fmt::format("add_button|{}|`w{}``|noflags|0|0|\r\n", friendId, pOnline->GetRawName());
            } else if (showAll) {
                rows += fmt::format("add_button|{}|`o{}``|noflags|0|0|\r\n", friendId, friendId);
            }
        }

        std::string body;
        if (friends.empty()) {
            body = "add_textbox|You currently have no friends. That's just sad. To make some click a person's "
                   "wrench icon, then choose `5Add as friend``.|\r\n"
                   "add_spacer|small|\r\n"
                   "add_spacer|small|\r\n";
        } else if (onlineCount == 0 && !showAll) {
            body = "add_textbox|None of your friends are currently online.|\r\n"
                   "add_spacer|small|\r\n"
                   "add_spacer|small|\r\n"
                   "add_button|all_friends|Show offline and ignored too|noflags|0|0|\r\n";
        } else {
            body = rows + "add_spacer|small|\r\n";
        }

        std::string content = fmt::format(
            "\r\n"
            "set_default_color|`o\r\n"
            "add_label_with_icon|big|{} of {} `wFriends Online``|left|1366|\r\n"
            "add_spacer|small|\r\n"
            "{}"
            "add_button|friends_options|Friend Options|noflags|0|0|\r\n"
            "add_button|close|Close|noflags|0|0|\r\n"
            "end_dialog|friends|||",
            onlineCount, friends.size(), body);

        auto vList = VariantList::Create("OnDialogRequest");
        vList.Insert(content);
        ENetWrapper::SendVariantList(pAvatar->Get(), vList);
    }

    inline void SendFriendOptions(Player* pAvatar) {
        std::string content = fmt::format(
            "\r\n"
            "set_default_color|`o\r\n"
            "add_label_with_icon|big|`wFriend Options``|left|1366|\r\n"
            "add_spacer|small|\r\n"
            "add_checkbox|checkbox_public|Show location to friends|{}\r\n"
            "add_checkbox|checkbox_notifications|Show friend notifications|{}\r\n"
            "add_spacer|small|\r\n"
            "add_button|back|OK|noflags|0|0|\r\n"
            "end_dialog|friends_options|||",
            pAvatar->GetShowLocationToFriends() ? 1 : 0,
            pAvatar->GetShowFriendNotifications() ? 1 : 0);

        auto vList = VariantList::Create("OnDialogRequest");
        vList.Insert(content);
        ENetWrapper::SendVariantList(pAvatar->Get(), vList);
    }

    inline void SendFriendInfo(Player* pAvatar, uint32_t friendId) {
        if (!pAvatar->IsFriend(friendId)) {
            SendFriendsList(pAvatar, false);
            return;
        }

        Player* pTarget = CommandUtils::FindOnlinePlayer(std::to_string(friendId));
        std::string body;
        if (pTarget) {
            std::string name = pTarget->GetRawName();
            if (pTarget->GetShowLocationToFriends()) {
                auto pTargetWorld = pTarget->GetWorld();
                std::string world = pTargetWorld ? pTargetWorld->GetName() : "EXIT";
                body = fmt::format(
                    "add_textbox|`o`w{}```` is `2online`` now in the world `5{}``.|\r\n"
                    "add_spacer|small|\r\n"
                    "add_button|warp|`oWarp to `5{}``|noflags|0|0|\r\n"
                    "add_spacer|small|\r\n",
                    name, world, world);
            } else {

                body = fmt::format(
                    "add_textbox|`o`w{}```` is `2online`` now, but has not made their location public to friends.|\r\n"
                    "add_spacer|small|\r\n",
                    name);
            }
        } else {
            body = fmt::format("add_textbox|`o`w{}```` is `4offline``.|\r\n"
                                "add_spacer|small|\r\n",
                friendId);
        }

        std::string content = fmt::format(
            "\r\n"
            "set_default_color|`o\r\n"
            "embed_data|userID|{}\r\n"
            "add_label_with_icon|big|`w`w{}````|left|1366|\r\n"
            "add_spacer|small|\r\n"
            "{}"
            "add_button|remove_friend|Remove as friend|noflags|0|0|\r\n"
            "add_button|back|Back|noflags|0|0|\r\n"
            "end_dialog|friend|||",
            friendId, pTarget ? pTarget->GetRawName() : std::to_string(friendId), body);

        auto vList = VariantList::Create("OnDialogRequest");
        vList.Insert(content);
        ENetWrapper::SendVariantList(pAvatar->Get(), vList);
    }

    inline void SendRemoveConfirm(Player* pAvatar, uint32_t friendId, const std::string& displayName) {
        pAvatar->SetFriendDialogConfirmingRemoval(friendId);

        std::string content = fmt::format(
            "\r\n"
            "set_default_color|`o\r\n"
            "embed_data|userID|{}\r\n"
            "add_label_with_icon|big|`4Remove friend``|left|1366|\r\n"
            "add_spacer|small|\r\n"
            "add_textbox|Are you sure you want to `4remove`` `w{}`` from your friend list?|\r\n"
            "add_spacer|small|\r\n"
            "add_button|remove_confirm|Remove as friend|noflags|0|0|\r\n"
            "add_button|back|Back|noflags|0|0|\r\n"
            "end_dialog|friend|||",
            friendId, displayName);

        auto vList = VariantList::Create("OnDialogRequest");
        vList.Insert(content);
        ENetWrapper::SendVariantList(pAvatar->Get(), vList);
    }

    inline void SendRemoved(Player* pAvatar, const std::string& displayName) {
        std::string content = fmt::format(
            "\r\n"
            "set_default_color|`o\r\n"
            "add_label_with_icon|big|`4Friend removed``|left|1366|\r\n"
            "add_spacer|small|\r\n"
            "add_textbox|Ok, you are no longer friends with that jerk `w{}``.|\r\n"
            "add_spacer|small|\r\n"
            "add_button|back|OK|noflags|0|0|\r\n"
            "end_dialog|friend|||",
            displayName);

        auto vList = VariantList::Create("OnDialogRequest");
        vList.Insert(content);
        ENetWrapper::SendVariantList(pAvatar->Get(), vList);
    }
}

ACTION_EVENT("friends", OnFriendsAction) {
    FriendDialog::SendFriendsList(pAvatar, false);
    Logger::Print(INFO, "Sent friends list to {}", pAvatar->GetRawName());
}

DIALOG_EVENT("friends", OnFriendsListReturn) {
    std::string button = eventParser.Get("buttonClicked", 1);
    if (button == "friends_options") {
        FriendDialog::SendFriendOptions(pAvatar);
    } else if (button == "all_friends") {
        FriendDialog::SendFriendsList(pAvatar, true);
    } else if (button == "close" || button.empty()) {

    } else {

        try {
            FriendDialog::SendFriendInfo(pAvatar, static_cast<uint32_t>(std::stoul(button)));
        } catch (const std::logic_error&) {

        }
    }
}

DIALOG_EVENT("friends_options", OnFriendOptionsReturn) {
    if (eventParser.Get("buttonClicked", 1) != "back")
        return;
    pAvatar->SetShowLocationToFriends(eventParser.Get<int32_t>("checkbox_public", 1) != 0);
    pAvatar->SetShowFriendNotifications(eventParser.Get<int32_t>("checkbox_notifications", 1) != 0);
    GetDatabase()->GetPlayerTable()->Save(pAvatar);
    FriendDialog::SendFriendsList(pAvatar, false);
}

DIALOG_EVENT("friend", OnFriendReturn) {
    uint32_t friendId = eventParser.Get<uint32_t>("userID", 1);
    std::string button = eventParser.Get("buttonClicked", 1);

    if (button == "warp") {

        if (!pAvatar->IsFriend(friendId))
            return;
        Player* pTarget = CommandUtils::FindOnlinePlayer(std::to_string(friendId));
        auto pTargetWorld = pTarget ? pTarget->GetWorld() : nullptr;
        if (!pTarget || !pTarget->GetShowLocationToFriends() || !pTargetWorld)
            return;

        VarList::OnTextOverlay(pAvatar->Get(), fmt::format("Moving to friend location (`2{}``) ...", pTargetWorld->GetName()));
        CommandUtils::SendToWorld(pAvatar, pTargetWorld->GetName());
        return;
    }

    if (button == "remove_friend") {
        if (!pAvatar->IsFriend(friendId))
            return;
        Player* pTarget = CommandUtils::FindOnlinePlayer(std::to_string(friendId));
        FriendDialog::SendRemoveConfirm(pAvatar, friendId, pTarget ? pTarget->GetRawName() : std::to_string(friendId));
        return;
    }

    if (button == "remove_confirm") {
        Player* pTarget = CommandUtils::FindOnlinePlayer(std::to_string(friendId));
        std::string displayName = pTarget ? pTarget->GetRawName() : std::to_string(friendId);

        pAvatar->RemoveFriend(friendId);
        pAvatar->SetFriendDialogConfirmingRemoval(0);
        GetDatabase()->GetPlayerTable()->Save(pAvatar);

        if (pTarget) {
            pTarget->RemoveFriend(pAvatar->GetUserId());
            GetDatabase()->GetPlayerTable()->Save(pTarget);
        }

        FriendDialog::SendRemoved(pAvatar, displayName);
        return;
    }

    if (button == "back") {

        if (pAvatar->GetFriendDialogConfirmingRemoval() == friendId && friendId != 0) {
            pAvatar->SetFriendDialogConfirmingRemoval(0);
            FriendDialog::SendFriendInfo(pAvatar, friendId);
        } else {
            FriendDialog::SendFriendsList(pAvatar, false);
        }
    }
}
