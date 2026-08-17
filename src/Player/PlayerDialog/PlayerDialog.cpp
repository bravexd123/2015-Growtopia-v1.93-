#include <Player/PlayerDialog/PlayerDialog.hpp>
#include <algorithm>
#include <fmt/core.h>
#include <ENetWrapper/ENetWrapper.hpp>
#include <Manager/Item/ItemComponent.hpp>
#include <Logger/Logger.hpp>

PlayerDialog::PlayerDialog() {
    m_pPeer = nullptr;
}
PlayerDialog::PlayerDialog(ENetPeer* peer) {
    m_pPeer = peer;
}

void PlayerDialog::SendData(int32_t delayMS, DialogBuilder* pData) {
    std::string dialogContent = pData->Get();
    if (!dialogContent.empty() && dialogContent.front() == '\n')
        dialogContent.erase(0, 1);

    Logger::Print(INFO, "PlayerDialog::SendData called, delayMS={}, dialog content length={}", delayMS, dialogContent.size());
    Logger::Print(INFO, "Dialog content: {}", dialogContent);

    auto vList = VariantList::Create("OnDialogRequest", delayMS);
    vList.Insert(dialogContent);

    auto vPacket = SVariantPacket(vList);
    Logger::Print(INFO, "SVariantPacket created, packetLength={}", vPacket.m_packetLength);

    ENetWrapper::SendPacket(m_pPeer, vPacket);
    Logger::Print(INFO, "SendPacket called for dialog");
}

void PlayerDialog::Send(eDialogTypes eType, TextParse data) {
    DialogBuilder db;

    switch (eType) {
    case DIALOG_TYPE_REGISTRATION: {
        db.SetDefaultColor('o')
            ->AddLabel("`wGet a GrowID!``", LEFT, BIG)
            ->AddSpacer();
        if (data.Contain("extra_message"))
            db.AddTextbox(data.Get("extra_message"))->AddSpacer();
        db.AddTextbox("By choosing a `wGrowID``, you can use a name and password to logon from any device. Your`` name`` will be shown to other players!")
            ->AddTextInput("logon", "Name ", data.Get("logon"), 18)
            ->AddSpacer()
            ->AddTextbox("Your `wpassword`` must contain`` 8 to 18 characters, 1 letter, 1 number`` and`` 1 special character: @#!$^&*.,``")

            ->AddTextInput("password", "Password ", data.Get("password"), 18)
            ->AddTextInput("verify_password", "Password Verify ", data.Get("verify_password"), 18)
            ->AddTextbox("Your `wemail`` will only be used for account verification and support. If you enter a fake email, you can't verify your account, recover or change your password.")
            ->AddTextInput("email", "Email ", data.Get("email"), 50)
            ->AddTextbox("We will never ask you for your password or email, never share it with anyone!")
            ->AddSpacer()
            ->EndDialog("growid_apply", "Cancel", "`wGet My GrowID!``");
    } break;

    case DIALOG_TYPE_TRASH_ITEM: {

        bool isSupporterTier = data.Get<int32_t>("accountTier") != 0;
        int32_t boxWidth = std::max<int32_t>(3, data.Get<int32_t>("have"));
        db.SetDefaultColor('o')
            ->AddLabelWithIcon(fmt::format("`{}`` `w{}``", isSupporterTier ? "2Recycle" : "4Trash", data.Get("itemName")), data.Get<int32_t>("itemId"), DIR_BIG, BIG);
        if (isSupporterTier)
            db.AddTextbox("You will get 0-1 gems per item.");
        db.AddTextbox(fmt::format("How many to `4destroy``? (you have {})", data.Get("have")))
            ->AddTextInput("count", "", "0", boxWidth)
            ->EmbedData("itemID", data.Get<int32_t>("itemId"))
            ->EndDialog("trash_item", "Cancel", "OK");
    } break;
    case DIALOG_TYPE_DROP_ITEM: {
        int32_t boxWidth = std::max<int32_t>(3, data.Get<int32_t>("have"));
        db.SetDefaultColor('o')
            ->AddLabelWithIcon(fmt::format("`wDrop {}``", data.Get("itemName")), data.Get<int32_t>("itemId"), LEFT, BIG)
            ->AddTextbox("How many to drop?")
            ->AddTextInput("count", "", data.Get("have"), boxWidth)
            ->EmbedData("itemID", data.Get<int32_t>("itemId"))
            ->AddTextbox("If you are trying to trade an item with another player, use your wrench on them instead to use our Trade System! `4Dropping items is not safe!``")
            ->EndDialog("drop_item", "Cancel", "OK");
    } break;
    case DIALOG_TYPE_ITEM_INFO: {
        db.SetDefaultColor('o')
            ->AddLabelWithEleIcon(fmt::format("`wAbout {}``", data.Get("itemName")), data.Get<int32_t>("itemId"), LEFT, BIG)
            ->AddSpacer()
            ->AddTextbox(data.Get("description"))
            ->AddSpacer()
            ->AddTextbox(fmt::format("Rarity: `w{}``", data.Get("rarity")))
            ->AddSpacer();

        for (int i = 0; ; i++) {
            std::string line = data.Get(fmt::format("prop{}", i));
            if (line.empty())
                break;
            db.AddTextbox(line);
        }
        db.AddSpacer()
            ->EmbedData("itemID", data.Get<int32_t>("itemId"))
            ->EndDialog("continue", "", "OK");
    } break;
    default:
        break;
    }

    int32_t delayMS = data.Get("delayMS") == "" ? 0 : data.Get<int32_t>("delayMS");
    this->SendData(delayMS, &db);
}
