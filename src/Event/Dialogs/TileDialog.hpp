#pragma once
#include <Event/EventType.hpp>
#include <Event/Dialogs/TileDialogHelpers.hpp>
#include <Manager/Database/Database.hpp>
#include <Utils/MiscUtils.hpp>
#include <Logger/Logger.hpp>

DIALOG_EVENT("bulletin_edit", OnBulletinEditDialog) {
    uint32_t tileX = eventParser.Get<uint32_t>("tilex", 1);
    uint32_t tileY = eventParser.Get<uint32_t>("tiley", 1);

    auto pWorld = pAvatar->GetWorld();
    if (!pWorld)
        return;
    Tile* pTile = pWorld->GetTile(tileX, tileY);
    if (!pTile)
        return;
    auto* pItem = GetItemManager()->GetItem(pTile->m_foregroundId);
    if (!pItem || (pItem->m_itemType != ITEMTYPE_BULLETIN && pItem->m_itemType != ITEMTYPE_MAILBOX))
        return;

    bool isMailbox = pItem->m_itemType == ITEMTYPE_MAILBOX;
    bool hasAccess = pWorld->CanEdit(tileX, tileY, pAvatar->GetUserId());
    std::string button = eventParser.Get("buttonClicked", 1);

    if (button == "clear") {
        if (!hasAccess)
            return;
        pWorld->ClearBulletinPosts(tileX, tileY);
        GetDatabase()->GetWorldTable()->Save(*pWorld);
        VarList::OnTalkBubble(pAvatar->Get(), pAvatar->GetNetId(), isMailbox ? "`2Mailbox emptied.``" : "`2Text cleared.``");
        return;
    }

    if (button.rfind("edit", 0) == 0 && button.size() > 4) {
        if (!hasAccess)
            return;
        std::size_t index = static_cast<std::size_t>(std::atoi(button.c_str() + 4));
        const auto& posts = pWorld->GetBulletinPosts();
        if (index >= posts.size())
            return;
        VarList::OnDialogRequest(pAvatar->Get(), fmt::format(
            "set_default_color|`o\nembed_data|postindex|{}\n"
            "add_label_with_icon|small|Remove`` \"`w{}\"`` from your board?|left|{}|\n"
            "end_dialog|remove_bulletin|Cancel|OK|",
            index, posts[index].m_text, pTile->m_foregroundId));
        return;
    }

    if (button == "send") {

        if (!hasAccess && !pTile->m_bulletinPublicCanAdd)
            return;
        std::string text = eventParser.Get("sign_text", 1);

        if (text.length() <= 2 || text.length() >= 100) {
            VarList::OnTalkBubble(pAvatar->Get(), pAvatar->GetNetId(), "That's not interesting enough to post.");
            return;
        }

        if (!hasAccess) {
            std::size_t mine = 0;
            for (const auto& post : pWorld->GetBulletinPosts())
                if (post.m_x == tileX && post.m_y == tileY && post.m_name.find(pAvatar->GetRawName()) != std::string::npos)
                    mine++;
            if (mine >= 3)
                return;
        }
        pWorld->AddBulletinPost(tileX, tileY, fmt::format("`0{}``", pAvatar->GetRawName()), text);
        GetDatabase()->GetWorldTable()->Save(*pWorld);
        VarList::OnTalkBubble(pAvatar->Get(), pAvatar->GetNetId(),
            isMailbox ? "`2You place your letter in the mailbox.``" : "`2Bulletin posted.``");
        Logger::Print(INFO, "Player {} posted to {} at ({},{})", pAvatar->GetRawName(), pItem->m_name, tileX, tileY);
        return;
    }

    if (!hasAccess)
        return;
    pTile->m_bulletinPublicCanAdd = eventParser.Get<int>("checkbox_locked", 1) != 0;
    pTile->m_bulletinHideNames = eventParser.Get<int>("checkbox_hide", 1) != 0;
    GetDatabase()->GetWorldTable()->Save(*pWorld);
}

DIALOG_EVENT("2646", OnSpotlightDialog) {
    uint32_t tileX = eventParser.Get<uint32_t>("tilex", 1);
    uint32_t tileY = eventParser.Get<uint32_t>("tiley", 1);

    auto pWorld = pAvatar->GetWorld();
    if (!pWorld)
        return;
    Tile* pTile = pWorld->GetTile(tileX, tileY);
    if (!pTile)
        return;
    auto* pItem = GetItemManager()->GetItem(pTile->m_foregroundId);
    if (!pItem || pItem->m_itemType != ITEMTYPE_SPOTLIGHT)
        return;
    if (!pWorld->CanEdit(tileX, tileY, pAvatar->GetUserId()))
        return;

    auto ClearCurrent = [&]() {
        if (pTile->m_spotlightName.empty())
            return;
        for (auto* pOther : pWorld->GetPlayers()) {
            if (pOther->GetRawName() != pTile->m_spotlightName)
                continue;
            pOther->RemovePlaymod(901);
            VarList::OnConsoleMessage(pOther->Get(), "Back to anonymity. (`$In the Spotlight`` mod removed)");
            VarList::OnTalkBubble(pOther->Get(), pOther->GetNetId(), "Lights out!");
        }
        pTile->m_spotlightName.clear();
    };

    if (eventParser.Get("buttonClicked", 1) == "off") {
        ClearCurrent();
        GetDatabase()->GetWorldTable()->Save(*pWorld);
        return;
    }

    std::string chosen = eventParser.Get("ID", 1);
    if (chosen.empty())
        return;
    Player* pTarget = nullptr;
    for (auto* pOther : pWorld->GetPlayers())
        if (pOther->GetRawName() == chosen)
            pTarget = pOther;
    if (!pTarget)
        return;

    ClearCurrent();
    pTile->m_spotlightName = pTarget->GetRawName();

    pTarget->AddPlaymod(901, 60 * 60 * 24 * 365, pAvatar->GetRawName());
    GetDatabase()->GetWorldTable()->Save(*pWorld);
    Logger::Print(INFO, "Player {} put {} in the spotlight at ({},{})", pAvatar->GetRawName(), pTarget->GetRawName(), tileX, tileY);
}

DIALOG_EVENT("remove_bulletin", OnRemoveBulletinDialog) {
    auto pWorld = pAvatar->GetWorld();
    if (!pWorld)
        return;
    std::size_t index = eventParser.Get<std::size_t>("postindex", 1);
    const auto& posts = pWorld->GetBulletinPosts();
    if (index >= posts.size())
        return;

    if (!pWorld->CanEdit(posts[index].m_x, posts[index].m_y, pAvatar->GetUserId()))
        return;
    pWorld->RemoveBulletinPost(index);
    GetDatabase()->GetWorldTable()->Save(*pWorld);
}

DIALOG_EVENT("sign_edit", OnSignEditDialog) {
    uint32_t tileX = eventParser.Get<uint32_t>("tilex", 1);
    uint32_t tileY = eventParser.Get<uint32_t>("tiley", 1);

    Tile* pTile = TileDialogs::ResolveEditableTile(pAvatar, tileX, tileY);
    if (!pTile || !pTile->m_hasSignExtra)
        return;

    std::string text = eventParser.Get("sign_text", 1);
    if (text.size() > 100)
        text.resize(100);
    pTile->m_signExtra.m_text = text;

    auto pWorld = pAvatar->GetWorld();
    GetDatabase()->GetWorldTable()->Save(*pWorld);

    pWorld->BroadcastTileUpdate(tileX, tileY);
    Logger::Print(INFO, "Player {} set sign text at ({},{}) to '{}'", pAvatar->GetRawName(), tileX, tileY, text);
}

DIALOG_EVENT("door_edit", OnDoorEditDialog) {
    uint32_t tileX = eventParser.Get<uint32_t>("tilex", 1);
    uint32_t tileY = eventParser.Get<uint32_t>("tiley", 1);

    Tile* pTile = TileDialogs::ResolveEditableTile(pAvatar, tileX, tileY);
    if (!pTile || !pTile->m_hasDoorExtra)
        return;

    std::string label = eventParser.Get("door_name", 1);
    std::string target = eventParser.Get("door_target", 1);
    std::string targetId = eventParser.Get("door_id", 1);
    if (label.size() > 100)
        label.resize(100);

    Utils::ToUpperCase(target);
    if (target.size() > 24)
        target.resize(24);

    pTile->m_doorExtra.m_label = label;
    pTile->m_doorExtra.m_target = target;
    pTile->m_doorExtra.m_targetId = targetId;

    auto pWorld = pAvatar->GetWorld();
    GetDatabase()->GetWorldTable()->Save(*pWorld);
    pWorld->BroadcastTileUpdate(tileX, tileY);
    Logger::Print(INFO, "Player {} set door at ({},{}) label='{}' target='{}'", pAvatar->GetRawName(), tileX, tileY, label, target);
}
