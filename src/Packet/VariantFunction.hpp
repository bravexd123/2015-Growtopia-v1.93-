#pragma once
#include <array>
#include <fmt/core.h>
#include <ENetWrapper/ENetWrapper.hpp>
#include <Packet/VariantList.hpp>
#include <Manager/Item/ItemType.hpp>
#include <Manager/Item/ItemManager.hpp>
#include <config.hpp>

class VarList {
public:
    static VariantList OnConsoleMessage(ENetPeer* peer, std::string message, int32_t delayMS = 0) {
        auto vList = VariantList::Create(__func__, delayMS);
        vList.Insert(message);

        ENetWrapper::SendVariantList(peer, vList);
        return vList;
    }

    static VariantList OnTalkBubble(ENetPeer* peer, int32_t netId, std::string message, int32_t delayMS = 0) {
        auto vList = VariantList::Create(__func__, delayMS);
        vList.Insert(netId);
        vList.Insert(message);
        vList.Insert(static_cast<int32_t>(0));

        ENetWrapper::SendVariantList(peer, vList);
        return vList;
    }
    static VariantList OnTextOverlay(ENetPeer* peer, std::string message, int32_t delayMS = 0) {
        auto vList = VariantList::Create(__func__, delayMS);
        vList.Insert(message);

        ENetWrapper::SendVariantList(peer, vList);
        return vList;
    }
    static VariantList SetHasGrowID(ENetPeer* peer, bool checkboxEnable, std::string tankIDName, std::string tankIDPass, int32_t delayMS = 0) {
        auto vList = VariantList::Create(__func__, delayMS);
        vList.Insert(checkboxEnable ? 1 : 0);
        vList.Insert(tankIDName);
        vList.Insert(tankIDPass);

        ENetWrapper::SendVariantList(peer, vList);
        return vList;
    }
    static VariantList OnSpawn(ENetPeer* peer, std::string spawnData, int32_t delayMS = 0) {
        auto vList = VariantList::Create(__func__, delayMS);
        vList.Insert(spawnData);

        ENetWrapper::SendVariantList(peer, vList);
        return vList;
    }
    static VariantList OnRequestWorldSelectMenu(ENetPeer* peer, std::string menuData, int32_t delayMS = 0) {
        auto vList = VariantList::Create(__func__, delayMS);
        vList.Insert(menuData);

        ENetWrapper::SendVariantList(peer, vList);
        return vList;
    }
    static VariantList OnFailedToEnterWorld(ENetPeer* peer, int32_t value = 1, int32_t delayMS = 0) {
        auto vList = VariantList::Create(__func__, delayMS);
        vList.Insert(value);

        ENetWrapper::SendVariantList(peer, vList);
        return vList;
    }

    static VariantList OnSetBux(ENetPeer* peer, int32_t gemCount, int32_t delayMS = 0) {
        auto vList = VariantList::Create(__func__, delayMS);
        vList.Insert(gemCount);
        vList.Insert(static_cast<int32_t>(1));
        vList.Insert(static_cast<uint32_t>(1));

        ENetWrapper::SendVariantList(peer, vList);
        return vList;
    }

    static VariantList OnLogonAccepted(ENetPeer* peer, int32_t delayMS = 0) {
        auto vList = VariantList::Create(__func__, delayMS);
        vList.Insert(static_cast<int32_t>(GetItemManager()->GetItemsDatHash()));
        vList.Insert(Configuration::GetBaseHost());
        vList.Insert(std::string("cache/"));
        vList.Insert(std::string(
            "cc.cz.madkite.freedom org.aqua.gg idv.aqua.bulldog com.cih.gamecih2 com.cih.gamecih "
            "com.cih.game_cih cn.maocai.gamekiller com.gmd.speedtime org.dax.attack com.x0.strai.frep "
            "com.x0.strai.free org.cheatengine.cegui org.sbtools.gamehack com.skgames.traffikrider "
            "org.sbtoods.gamehaca com.skype.ralder org.cheatengine.cegui.xx.multi1458919170111 "
            "com.prohiro.macro me.autotouch.autotouch com.cygery.repetitouch.free com.cygery.repetitouch.pro "
            "com.proziro.zacro com.slash.gamebuster"));
        vList.Insert(std::string("proto=17|choosemusic=audio/mp3/about_theme.mp3|"));

        ENetWrapper::SendVariantList(peer, vList);
        return vList;
    }

    static VariantList OnStoreRequest(ENetPeer* peer, std::string content, int32_t delayMS = 0) {
        auto vList = VariantList::Create(__func__, delayMS);
        vList.Insert(content);

        ENetWrapper::SendVariantList(peer, vList);
        return vList;
    }

    static VariantList OnStorePurchaseResult(ENetPeer* peer, std::string message, int32_t delayMS = 0) {
        auto vList = VariantList::Create(__func__, delayMS);
        vList.Insert(message);

        ENetWrapper::SendVariantList(peer, vList);
        return vList;
    }

    static VariantList OnPlayPositioned(ENetPeer* peer, int32_t netId, std::string soundPath, int32_t delayMS = 0) {
        auto vList = VariantList::Create(__func__, delayMS, netId);
        vList.Insert(soundPath);

        ENetWrapper::SendVariantList(peer, vList);
        return vList;
    }

    static VariantList OnNameChanged(ENetPeer* peer, int32_t netId, std::string coloredName, int32_t delayMS = 0) {
        auto vList = VariantList::Create(__func__, delayMS, netId);
        vList.Insert(coloredName);

        ENetWrapper::SendVariantList(peer, vList);
        return vList;
    }
    static VariantList OnSetClothing(ENetPeer* peer, int32_t netId, const std::array<uint16_t, NUM_BODY_PARTS>& clothes, int32_t skinColor, int32_t delayMS = 0) {
        auto vList = VariantList::Create(__func__, delayMS, netId);
        vList.Insert(CL_Vec3<float>{
            static_cast<float>(clothes[CLOTHTYPE_HAIR]),
            static_cast<float>(clothes[CLOTHTYPE_SHIRT]),
            static_cast<float>(clothes[CLOTHTYPE_PANTS])});
        vList.Insert(CL_Vec3<float>{
            static_cast<float>(clothes[CLOTHTYPE_FEET]),
            static_cast<float>(clothes[CLOTHTYPE_FACE]),
            static_cast<float>(clothes[CLOTHTYPE_HAND])});
        vList.Insert(CL_Vec3<float>{
            static_cast<float>(clothes[CLOTHTYPE_BACK]),
            static_cast<float>(clothes[CLOTHTYPE_MASK]),
            static_cast<float>(clothes[CLOTHTYPE_NECKLACE])});
        vList.Insert(skinColor);

        ENetWrapper::SendVariantList(peer, vList);
        return vList;
    }

    static VariantList OnKilled(ENetPeer* peer, int32_t netId, int32_t value = 1, int32_t delayMS = 0) {
        auto vList = VariantList::Create(__func__, delayMS, netId);
        vList.Insert(value);

        ENetWrapper::SendVariantList(peer, vList);
        return vList;
    }

    static VariantList OnSetFreezeState(ENetPeer* peer, int32_t netId, int32_t value, int32_t delayMS = 0) {
        auto vList = VariantList::Create(__func__, delayMS, netId);
        vList.Insert(value);

        ENetWrapper::SendVariantList(peer, vList);
        return vList;
    }

    static VariantList OnSetPos(ENetPeer* peer, int32_t netId, float x, float y, int32_t delayMS = 0) {
        auto vList = VariantList::Create(__func__, delayMS, netId);
        vList.Insert(CL_Vec2<float>{x, y});

        ENetWrapper::SendVariantList(peer, vList);
        return vList;
    }

    static VariantList OnCountdownEnd(ENetPeer* peer, int32_t netId, int32_t value = 0, int32_t delayMS = 0) {
        auto vList = VariantList::Create(__func__, delayMS, netId);
        vList.Insert(value);

        ENetWrapper::SendVariantList(peer, vList);
        return vList;
    }

    static VariantList OnRemove(ENetPeer* peer, int32_t netId, int32_t delayMS = 0) {
        auto vList = VariantList::Create(__func__, delayMS);
        vList.Insert(fmt::format("netID|{}\r\n", netId));

        ENetWrapper::SendVariantList(peer, vList);
        return vList;
    }

    static VariantList OnStartTrade(ENetPeer* peer, std::string partnerName, int32_t partnerNetId, int32_t delayMS = 0) {
        auto vList = VariantList::Create(__func__, delayMS);
        vList.Insert(partnerName);
        vList.Insert(partnerNetId);

        ENetWrapper::SendVariantList(peer, vList);
        return vList;
    }

    static VariantList OnTradeStatus(ENetPeer* peer, int32_t ownerNetId, std::string title, std::string body, int32_t delayMS = 0) {
        auto vList = VariantList::Create(__func__, delayMS);
        vList.Insert(ownerNetId);
        vList.Insert(std::string(""));
        vList.Insert(title);
        vList.Insert(body);

        ENetWrapper::SendVariantList(peer, vList);
        return vList;
    }

    static VariantList OnDialogRequest(ENetPeer* peer, std::string content, int32_t delayMS = 0) {
        auto vList = VariantList::Create(__func__, delayMS);
        vList.Insert(content);

        ENetWrapper::SendVariantList(peer, vList);
        return vList;
    }

    static VariantList OnSetCurrentWeather(ENetPeer* peer, int32_t weatherId, int32_t delayMS = 0) {
        auto vList = VariantList::Create(__func__, delayMS);
        vList.Insert(weatherId);

        ENetWrapper::SendVariantList(peer, vList);
        return vList;
    }

    static VariantList OnAction(ENetPeer* peer, int32_t netId, std::string action, int32_t delayMS = 0) {
        auto vList = VariantList::Create(__func__, delayMS, netId);
        vList.Insert(action);

        ENetWrapper::SendVariantList(peer, vList);
        return vList;
    }

    static VariantList OnEmoticonDataChanged(ENetPeer* peer, int32_t version, std::string data, int32_t delayMS = 0) {
        auto vList = VariantList::Create(__func__, delayMS);
        vList.Insert(version);
        vList.Insert(data);

        ENetWrapper::SendVariantList(peer, vList);
        return vList;
    }

    static VariantList OnFavItemUpdated(ENetPeer* peer, int32_t itemId, int32_t favourited, int32_t delayMS = 0) {
        auto vList = VariantList::Create(__func__, delayMS);
        vList.Insert(itemId);
        vList.Insert(favourited);

        ENetWrapper::SendVariantList(peer, vList);
        return vList;
    }

    static VariantList OnCountryState(ENetPeer* peer, int32_t netId, std::string state, int32_t delayMS = 0) {
        auto vList = VariantList::Create(__func__, delayMS, netId);
        vList.Insert(state);

        ENetWrapper::SendVariantList(peer, vList);
        return vList;
    }

    static VariantList OnForceTradeEnd(ENetPeer* peer, int32_t delayMS = 0) {
        auto vList = VariantList::Create(__func__, delayMS);

        ENetWrapper::SendVariantList(peer, vList);
        return vList;
    }
};
