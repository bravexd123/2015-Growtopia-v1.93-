#pragma once
#include <ctime>
#include <config.hpp>
#include <Logger/Logger.hpp>
#include <Event/EventType.hpp>
#include <Event/EventPool.hpp>
#include <Manager/Database/Database.hpp>
#include <Manager/Item/ItemComponent.hpp>
#include <Packet/TextFunction.hpp>
#include <Packet/VariantFunction.hpp>

DIALOG_EVENT("growid_apply", OnDialogGrowIDApply) {
    if (!pAvatar->GetDetail().GetTankIDName().empty())
        return;

    JsonPlayerTable* pTable = GetDatabase()->GetPlayerTable();
    if (!pTable)
        return;

    std::string
        name = eventParser.Get("logon", 1),
        password = eventParser.Get("password", 1),
        verifyPassword = eventParser.Get("verify_password", 1),
        email = eventParser.Get("email", 1)
        ;
    auto lowerName = name;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);

    auto playerAccount = pTable->RegisterPlayer(name, password, verifyPassword, email);
    if (playerAccount.m_result != PlayerRegistration::Result::SUCCESS) {
        pAvatar->PlayerDialog::Send(DIALOG_TYPE_REGISTRATION, TextParse({
            { "logon",              name                    },
            { "password",           password                },
            { "verify_password",    verifyPassword          },
            { "email",              email                   },
            { "extra_message",      playerAccount.m_data    }
            }));
        return;
    }

    TankInfo& det = pAvatar->GetDetail();
    PlayerItems* pItems = pAvatar->GetItems();

    det.SetTankIDName(lowerName);
    det.SetTankIDPass(password);
    pAvatar->SetEmailAddress(email);

    pItems->AddItem(ITEM_WORLD_LOCK, 1);
    pItems->AddItem(ITEM_PICKAXE, 1);

    pAvatar->SetRawName(name);
    pAvatar->SetDisplayName(name);
    pAvatar->SetCreatedAt(static_cast<int64_t>(std::time(nullptr)));

    uint32_t previousUserId = pAvatar->GetUserId();
    pAvatar->SetUserId(pTable->Insert(pAvatar, previousUserId));

    std::string rid = det.GetRelativeId();
    if (!rid.empty())
        pTable->DeleteGuest(rid);

    ENetPeer* peer = pAvatar->Get();

    CAction::PlaySFX(peer, "success", 0);
    VarList::SetHasGrowID(peer, true, name, det.GetTankIDPass());
    VarList::OnConsoleMessage(peer, fmt::format("`oA `wGrowID`` with the logon of `w{}`` created. Write it and your password down as the will be required to logon!``", pAvatar->GetRawName()));
    VarList::OnConsoleMessage(peer, fmt::format("`5Welcome to `w{}`5, press `wBack`` and then press `wConnect``, enjoy!", Configuration::GetName()));

    pAvatar->RequestDisconnect();
    Logger::Print(INFO, "GrowID successfully created for player {}, disconnect requested.", name);
}
