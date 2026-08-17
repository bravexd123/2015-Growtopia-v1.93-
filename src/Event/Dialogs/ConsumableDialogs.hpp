#pragma once
#include <algorithm>
#include <cctype>
#include <fmt/core.h>
#include <Event/EventType.hpp>
#include <Event/EventPool.hpp>
#include <Server/ServerPool.hpp>
#include <World/World.hpp>
#include <World/WorldPool.hpp>
#include <Manager/Item/ItemComponent.hpp>
#include <Manager/Item/Playmods.hpp>
#include <Manager/Database/Database.hpp>
#include <Manager/Item/ItemManager.hpp>
#include <Commands/CommandManager.hpp>
#include <Packet/VariantFunction.hpp>
#include <Packet/TextFunction.hpp>
#include <Utils/MiscUtils.hpp>
#include <Logger/Logger.hpp>

DIALOG_EVENT("megaphone", OnMegaphoneDialog) {
    std::string text = eventParser.Get("words", 1);
    if (text.empty())
        return;

    auto* pItems = pAvatar->GetItems();
    auto it = pItems->m_bpItems.find(static_cast<uint16_t>(ITEM_MEGAPHONE));
    if (it == pItems->m_bpItems.end() || it->second < 1)
        return;

    if (pAvatar->IsDuctTaped()) {
        VarList::OnConsoleMessage(pAvatar->Get(), "`6>> That's sort of hard to do while duct-taped.``");
        return;
    }

    if (int64_t remaining = pAvatar->GetPlaymodRemaining(13); remaining > 0) {
        CAction::Log(pAvatar->Get(), fmt::format(">> ({} before you can broadcast again)",
            Playmods::FormatDuration(remaining)));
        return;
    }

    pItems->RemoveItem(static_cast<uint16_t>(ITEM_MEGAPHONE), 1);
    pAvatar->AddPlaymod(13, 300);
    GetDatabase()->GetPlayerTable()->Save(pAvatar);
    PlayerItems::SendInventoryState(pAvatar);

    CommandManager::Get().Dispatch(pAvatar, "/sb " + text);
    Logger::Print(INFO, "Player {} used a Megaphone: {}", pAvatar->GetRawName(), text);
}

DIALOG_EVENT("name_change", OnNameChangeDialog) {
    std::string name = eventParser.Get("name_box", 1);
    if (name.empty())
        return;

    std::string typed = name, current = pAvatar->GetRawName();
    Utils::ToLowerCase(typed);
    Utils::ToLowerCase(current);
    if (typed != current) {
        VarList::OnDialogRequest(pAvatar->Get(),
            "set_default_color|`o\n"
            "add_label_with_icon|big|`wChange your GrowID``|left|1280|\n"
            "add_textbox|`4The name doesn't match your current name!``|left|\n"
            "add_smalltext|This will change your GrowID `4permanently``.<CR>Your `wBirth Certificate`` will be consumed if you press `5Change It``.<CR>NOTE: The birth certificate only will change your name case (you can not change your whole GrowID)!``|left|\n"
            "add_textbox|Enter your new name:|left|\n"
            "add_text_input|name_box|||32|\n"
            "add_spacer|small|\n"
            "end_dialog|name_change|Cancel|Change it!|");
        return;
    }

    auto* pItems = pAvatar->GetItems();
    auto it = pItems->m_bpItems.find(static_cast<uint16_t>(ITEM_BIRTH_CERTIFICATE));
    if (it == pItems->m_bpItems.end() || it->second < 1)
        return;
    pItems->RemoveItem(static_cast<uint16_t>(ITEM_BIRTH_CERTIFICATE), 1);

    pAvatar->SetRawName(name);
    GetDatabase()->GetPlayerTable()->Save(pAvatar);
    PlayerItems::SendInventoryState(pAvatar);

    if (auto pWorld = pAvatar->GetWorld()) {
        for (auto* pOther : pWorld->GetPlayers())
            VarList::OnNameChanged(pOther->Get(), pAvatar->GetNetId(), pAvatar->GetFormattedName());
    }
    VarList::OnConsoleMessage(pAvatar->Get(), fmt::format("Your GrowID is now `w{}``.", name));
    Logger::Print(INFO, "Player {} used a Birth Certificate", name);
}

DIALOG_EVENT("world_swap", OnWorldSwapDialog) {
    std::string other = eventParser.Get("name_box", 1);
    Utils::ToUpperCase(other);

    auto pWorld = pAvatar->GetWorld();
    auto pWorldPool = pServer->GetWorldPool();
    auto Fail = [&]() {
        VarList::OnDialogRequest(pAvatar->Get(),
            "set_default_color|`o\n"
            "add_label_with_icon|big|`wSwap World Names``|left|2580|\n"
            "add_textbox|`4World swap failed - you don't own both worlds!``|left|\n"
            "add_smalltext|This will swap the name of the world you are standing in with another world `4permanently``.  You must own both worlds, with a World Lock in place.<CR>Your `wChange of Address`` will be consumed if you press `5Swap 'Em``.|left|\n"
            "add_textbox|Enter the other world's name:|left|\n"
            "add_text_input|name_box|||32|\n"
            "add_spacer|small|\n"
            "end_dialog|world_swap|Cancel|Swap 'Em!|");
    };

    if (other.empty() || !pWorld || !pWorldPool || other == pWorld->GetName()) {
        Fail();
        return;
    }
    const LockInfo* pHereLock = pWorld->GetWorldLock();
    if (!pHereLock || pHereLock->m_ownerId != pAvatar->GetUserId()) {
        Fail();
        return;
    }

    if (!GetDatabase()->GetWorldTable()->Exists(other)) {
        Fail();
        return;
    }
    auto pOther = pWorldPool->NewWorld(other);
    if (!pOther) {
        Fail();
        return;
    }
    const LockInfo* pOtherLock = pOther->GetWorldLock();
    if (!pOtherLock || pOtherLock->m_ownerId != pAvatar->GetUserId()) {
        Fail();
        return;
    }

    if (!pOther->GetPlayers().empty() || pWorld->GetPlayers().size() > 1) {
        Fail();
        return;
    }

    auto* pItems = pAvatar->GetItems();
    auto it = pItems->m_bpItems.find(static_cast<uint16_t>(ITEM_CHANGE_OF_ADDRESS));
    if (it == pItems->m_bpItems.end() || it->second < 1)
        return;
    pItems->RemoveItem(static_cast<uint16_t>(ITEM_CHANGE_OF_ADDRESS), 1);

    std::string here = pWorld->GetName();
    if (!pWorldPool->SwapWorldNames(here, other)) {
        pItems->AddItem(static_cast<uint16_t>(ITEM_CHANGE_OF_ADDRESS), 1);
        Fail();
        return;
    }
    GetDatabase()->GetPlayerTable()->Save(pAvatar);
    PlayerItems::SendInventoryState(pAvatar);
    VarList::OnConsoleMessage(pAvatar->Get(), fmt::format("`5{}`` and `5{}`` have swapped names!", here, other));
    Logger::Print(INFO, "Player {} swapped world names '{}' <-> '{}'", pAvatar->GetRawName(), here, other);

    pWorld->BroadcastPlayerLeft(pAvatar);
    pWorld->ReleaseNetId(pAvatar->GetNetId());
    pWorld->RemovePlayer(pAvatar);
    pAvatar->SetWorld(nullptr);
    pAvatar->SetNetId(-1);
    pAvatar->GetDetail().RemoveFlag(CLIENTFLAG_IS_IN);
    if (auto* pJoinEvent = GetEventPool()->ActionManager::GetEventIfExists("join_request")) {
        TextParse joinParser(fmt::format("action|join_request\nname|{}\n", other));
        pJoinEvent->sig_function(pAvatar, pServer, std::string(), joinParser, nullptr);
    }
}

DIALOG_EVENT("blast", OnBlastDialog) {
    uint16_t blastItem = pAvatar->GetPendingBlastItem();
    pAvatar->SetPendingBlastItem(0);
    if (blastItem == 0)
        return;

    std::string name = eventParser.Get("name", 1);
    Utils::ToUpperCase(name);

    name.erase(std::remove_if(name.begin(), name.end(),
        [](unsigned char c) { return !std::isalnum(c); }), name.end());
    if (name.empty() || name.size() > 24 || name == "EXIT") {
        VarList::OnConsoleMessage(pAvatar->Get(), "`4That world name won't work.``");
        return;
    }

    auto pWorldPool = pServer->GetWorldPool();
    if (!pWorldPool)
        return;
    if (GetDatabase()->GetWorldTable()->Exists(name) || pWorldPool->GetWorld(name)) {
        VarList::OnConsoleMessage(pAvatar->Get(), "`4That world already exists!``");
        return;
    }

    auto* pItems = pAvatar->GetItems();
    auto it = pItems->m_bpItems.find(blastItem);
    if (it == pItems->m_bpItems.end() || it->second < 1)
        return;

    if (blastItem == ITEM_BEACH_BLAST) {
        auto fireworks = pItems->m_bpItems.find(static_cast<uint16_t>(ITEM_FIREWORKS));
        if (fireworks == pItems->m_bpItems.end() || fireworks->second < 100) {
            VarList::OnTalkBubble(pAvatar->Get(), pAvatar->GetNetId(), "Beach blast requires 100 Fireworks.");
            return;
        }
        pItems->RemoveItem(static_cast<uint16_t>(ITEM_FIREWORKS), 100);
    }
    pItems->RemoveItem(blastItem, 1);

    auto pNewWorld = pWorldPool->NewWorld(name);
    if (!pNewWorld) {
        VarList::OnConsoleMessage(pAvatar->Get(), "`4That world can't be created right now.``");
        return;
    }
    if (blastItem == ITEM_THERMONUCLEAR_BLAST)
        pNewWorld->GenerateEmpty();
    else if (blastItem == ITEM_HARVEST_MOON_BLAST)
        pNewWorld->SetWeather(6, 0, 0);

    GetDatabase()->GetWorldTable()->Save(*pNewWorld);
    GetDatabase()->GetPlayerTable()->Save(pAvatar);
    PlayerItems::SendInventoryState(pAvatar);

    auto* pBlastItem = GetItemManager()->GetItem(blastItem);
    std::string announce = fmt::format("** `5{} activates a {}! ``**",
        pAvatar->GetRawName(), pBlastItem ? pBlastItem->m_name : std::string("Blast"));
    VarList::OnConsoleMessage(pAvatar->Get(), announce);
    VarList::OnTalkBubble(pAvatar->Get(), pAvatar->GetNetId(), announce);
    Logger::Print(INFO, "Player {} created world '{}' with a {}", pAvatar->GetRawName(), name,
        pBlastItem ? pBlastItem->m_name : std::string());

    if (auto* pJoinEvent = GetEventPool()->ActionManager::GetEventIfExists("join_request")) {
        TextParse joinParser(fmt::format("action|join_request\nname|{}\n", name));
        pJoinEvent->sig_function(pAvatar, pServer, std::string(), joinParser, nullptr);
    }
}
