#include <Event/TankEvents/TankDispatcher.hpp>
#include <fmt/core.h>
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <Player/Player.hpp>
#include <Server/Server.hpp>
#include <World/World.hpp>
#include <World/WorldMenu.hpp>
#include <Manager/Item/ItemManager.hpp>
#include <Manager/Item/ItemInfo.hpp>
#include <Manager/Item/ItemComponent.hpp>
#include <Manager/Item/SpliceRecipes.hpp>
#include <Manager/Item/CombineRecipes.hpp>
#include <Commands/CommandUtils.hpp>
#include <Server/ServerPool.hpp>
#include <cctype>
#include <Manager/Item/ItemEffects.hpp>
#include <Manager/Item/WeatherMachines.hpp>
#include <Manager/Item/Providers.hpp>
#include <Manager/Database/Database.hpp>
#include <Manager/Trade/TradeManager.hpp>
#include <Event/Dialogs/LockHelpers.hpp>
#include <Event/Dialogs/TileDialogHelpers.hpp>
#include <Event/EventPool.hpp>
#include <Packet/PacketFactory.hpp>
#include <Packet/VariantFunction.hpp>
#include <Packet/TextFunction.hpp>
#include <ENetWrapper/ENetWrapper.hpp>
#include <Utils/BinaryWriter.hpp>
#include <Logger/Logger.hpp>

static void SendInventoryState(Player* pAvatar) {
    PlayerItems::SendInventoryState(pAvatar);
}

static void BroadcastDropSpawn(std::shared_ptr<World> pWorld, uint16_t itemId, uint8_t count, float x, float y) {
    TankPacketData t{};
    t.m_type = NET_GAME_PACKET_ITEM_CHANGE_OBJECT;
    t.m_netId = -1;
    t.m_item = -1;
    t.m_floatVariable = static_cast<float>(count);
    t.m_mainData = itemId;
    t.m_vectorX = x;
    t.m_vectorY = y;

    STankPacket packet(t);
    pWorld->BroadcastPacket(packet);
}

static void SpawnDropJittered(std::shared_ptr<World> pWorld, uint16_t itemId, uint8_t count, float baseX, float baseY) {

    if (DroppedItem* pExisting = pWorld->FindMergeableDrop(itemId, count, baseX, baseY)) {
        pExisting->m_count = static_cast<uint8_t>(pExisting->m_count + count);
        BroadcastDropSpawn(pWorld, itemId, pExisting->m_count, pExisting->m_x, pExisting->m_y);
        return;
    }

    float x = baseX + static_cast<float>((std::rand() % 5) - 2);
    float y = baseY + static_cast<float>((std::rand() % 5) - 2);
    pWorld->SpawnDrop(itemId, count, x, y);
    BroadcastDropSpawn(pWorld, itemId, count, x, y);
}

static void BroadcastDropPickedUp(std::shared_ptr<World> pWorld, int32_t pickerNetId, int32_t uid) {
    TankPacketData t{};
    t.m_type = NET_GAME_PACKET_ITEM_CHANGE_OBJECT;
    t.m_netId = pickerNetId;
    t.m_item = -2;
    t.m_mainData = uid;

    STankPacket packet(t);
    pWorld->BroadcastPacket(packet);
}

static float RandFloat() {
    return static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
}

static void SpawnGemsDenominated(std::shared_ptr<World> pWorld, uint32_t total, float x, float y) {
    static constexpr uint32_t kTiers[] = {100, 50, 10, 5, 1};
    for (uint32_t tier : kTiers) {
        while (total >= tier) {
            SpawnDropJittered(pWorld, ITEM_GEMS, static_cast<uint8_t>(tier), x, y);
            total -= tier;
        }
    }
}

static uint32_t XpForRarity(uint16_t rarity) {
    if (rarity >= 999)
        return 0;

    uint32_t xp = static_cast<uint32_t>(rarity) / 5;
    return xp > 0 ? xp : 1;
}

static constexpr float kNothingDropChance = 0.20f;

static constexpr float kSeedDropChance = 0.22f;

static float AverageGemsForRarity(uint16_t rarity) {
    if (rarity == 999)
        return 0.0f;
    float avg = (rarity < 31)
        ? 0.053872119f * rarity - 0.0000498141f
        : 0.0808082133f * rarity - 0.0000354996498f;
    return avg > 0.0f ? avg : 0.0f;
}

static uint8_t RollGems(const ItemInfo& item, std::shared_ptr<World> pWorld, float x, float y) {
    float avg = AverageGemsForRarity(item.m_rarity);
    if (avg <= 0.0f)
        return 0;

    constexpr float kTriggerChance = 0.125f;
    if (RandFloat() >= kTriggerChance)
        return 0;

    float meanWhenTriggered = avg / kTriggerChance;

    uint32_t gemCount = std::max<uint32_t>(1, static_cast<uint32_t>(std::lround(meanWhenTriggered * (0.5f + RandFloat()))));
    SpawnGemsDenominated(pWorld, gemCount, x, y);
    return static_cast<uint8_t>(std::min<uint32_t>(gemCount, 255));
}

static void BroadcastParticleEffect(std::shared_ptr<World> pWorld, int32_t effectId, float pixelX, float pixelY, float colour, float visual, float offset = 0.0f) {
    TankPacketData t{};
    t.m_type = NET_GAME_PACKET_SEND_PARTICLE_EFFECT;
    t.m_netId = effectId;
    t.m_mainData = effectId;
    t.m_vectorX = pixelX;
    t.m_vectorY = pixelY;
    t.m_vectorX2 = colour;
    t.m_vectorY2 = visual;
    t.m_particleRotation = offset;

    STankPacket packet(t);
    pWorld->BroadcastPacket(packet);
}

static void SpawnFireworks(std::shared_ptr<World> pWorld, float pixelX, float pixelY) {
    static constexpr float kColours[3] = { 0xb3, 0xbe, 0x7c };
    for (int i = 0; i < 3; i++) {
        float type = static_cast<float>(0x25 + (std::rand() % 4));
        float offset = static_cast<float>(260 + (std::rand() % 1941));
        BroadcastParticleEffect(pWorld, 0xc8 * i, pixelX, pixelY, kColours[i], type, offset);
    }
}

static void BroadcastTileConfirm(std::shared_ptr<World> pWorld, int32_t netId, uint32_t tileX, uint32_t tileY, uint16_t itemId, uint8_t fruitCount = 0, bool includeSeedExtra = false) {
    TankPacketData confirm{};
    confirm.m_type = NET_GAME_PACKET_TILE_CHANGE_REQUEST;
    confirm.m_netId = netId;
    confirm.m_tilePositionX = tileX;
    confirm.m_tilePositionY = tileY;
    confirm.m_itemId = itemId;

    confirm.m_fruitCount = fruitCount;

    if (includeSeedExtra) {

        std::vector<uint8_t> extra(6);
        extra[0] = 4;
        uint32_t elapsed = 0;
        std::memcpy(extra.data() + 1, &elapsed, 4);
        extra[5] = fruitCount;
        SExtendedTankPacket confirmPacket(confirm, std::move(extra));
        pWorld->BroadcastPacket(confirmPacket);
        return;
    }

    STankPacket confirmPacket(confirm);
    pWorld->BroadcastPacket(confirmPacket);
}

static void HandleLockPlaced(Player* pAvatar, std::shared_ptr<World> pWorld, uint32_t tileX, uint32_t tileY, uint16_t itemId) {
    auto* pItem = GetItemManager()->GetItem(itemId);
    if (!pItem)
        return;

    if (LockHelpers::IsWorldLockItem(pItem->m_name)) {

        if (!pWorld->TrySetWorldLock(pAvatar->GetUserId(), pAvatar->GetRawName(), itemId, tileX, tileY)) {
            pAvatar->GetItems()->AddItem(itemId, 1);
            GetDatabase()->GetPlayerTable()->Save(pAvatar);
            SendInventoryState(pAvatar);

            auto* pTile = pWorld->GetTile(tileX, tileY);
            if (pTile)
                pTile->ClearForeground();

            const LockInfo* pExisting = pWorld->GetWorldLock();
            std::string message = (pExisting && pExisting->m_ownerId == pAvatar->GetUserId())
                ? fmt::format("`wOnly one `${}`` can be placed in a world, you'd have to remove the other one first.", pItem->m_name)
                : "This world already has a lock on it that isn't yours.";

            auto vList = VariantList::Create("OnTalkBubble");
            vList.Insert(pAvatar->GetNetId());
            vList.Insert(message);
            vList.Insert(static_cast<int32_t>(0));
            vList.Insert(static_cast<int32_t>(1));
            ENetWrapper::SendVariantList(pAvatar->Get(), vList);
            GetDatabase()->GetWorldTable()->Save(*pWorld);
            return;
        }

        GetDatabase()->GetWorldTable()->Save(*pWorld);

        std::string worldLockMessage = fmt::format("`5[```w{}`` has been `$World Locked`` by {}`5]``", pWorld->GetName(), pAvatar->GetFormattedName());

        auto consoleList = VariantList::Create("OnConsoleMessage");
        consoleList.Insert(worldLockMessage);
        SVariantPacket consolePacket(consoleList);
        pWorld->BroadcastPacket(consolePacket);

        auto bubbleList = VariantList::Create("OnTalkBubble");
        bubbleList.Insert(pAvatar->GetNetId());
        bubbleList.Insert(worldLockMessage);
        bubbleList.Insert(static_cast<uint32_t>(0));
        bubbleList.Insert(static_cast<uint32_t>(0));
        SVariantPacket bubblePacket(bubbleList);
        pWorld->BroadcastPacket(bubblePacket);

        VarList::OnPlayPositioned(pAvatar->Get(), pAvatar->GetNetId(), "audio/use_lock.wav");

        VarList::OnNameChanged(pAvatar->Get(), pAvatar->GetNetId(), pAvatar->GetFormattedName());

        Logger::Print(INFO, "Player {} world-locked '{}' with {}", pAvatar->GetRawName(), pWorld->GetName(), pItem->m_name);
        return;
    }

    uint32_t capacity = LockHelpers::GetLockCapacity(pItem->m_name);
    if (capacity == 0)
        return;

    LockInfo* pLock = pWorld->AddTileLock(tileX, tileY, pAvatar->GetUserId(), pAvatar->GetRawName(), itemId, capacity);
    if (!pLock)
        return;

    LockHelpers::SendLockStateBroadcast(pWorld, *pLock);

    VarList::OnTalkBubble(pAvatar->Get(), pAvatar->GetNetId(), "Area locked.");

    VarList::OnPlayPositioned(pAvatar->Get(), pAvatar->GetNetId(), "audio/use_lock.wav");
    GetDatabase()->GetWorldTable()->Save(*pWorld);
    Logger::Print(INFO, "Player {} placed a {} at ({},{}), covering {} tiles", pAvatar->GetRawName(), pItem->m_name, tileX, tileY, pLock->m_coveredTiles.size());
}

static bool HandlePunchInteraction(Player* pAvatar, std::shared_ptr<World> pWorld, Tile* pTile,
                                   const ItemInfo& item, uint32_t tileX, uint32_t tileY) {
    float centreX = tileX * 32.0f + 16.0f;
    float centreY = tileY * 32.0f + 16.0f;

    switch (item.m_itemType) {
    case ITEMTYPE_WEATHER_MACHINE: {

        bool turningOn = !pTile->HasFlag(TILEFLAG_OPEN);
        if (turningOn) {
            Tile* pPrevious = pWorld->GetTile(pWorld->GetWeatherAnchorX(), pWorld->GetWeatherAnchorY());
            if (pPrevious && pPrevious != pTile)
                pPrevious->ClearFlag(TILEFLAG_OPEN);
        }
        pTile->ToggleFlag(TILEFLAG_OPEN);

        int32_t weatherId = turningOn ? GetWeatherIdForItem(pTile->m_foregroundId) : 0;
        pWorld->SetWeather(weatherId, turningOn ? tileX : 0, turningOn ? tileY : 0);
        for (auto* pOther : pWorld->GetPlayers())
            VarList::OnSetCurrentWeather(pOther->Get(), weatherId);

        pWorld->BroadcastTileUpdate(tileX, tileY);
        GetDatabase()->GetWorldTable()->Save(*pWorld);
        Logger::Print(INFO, "Player {} set weather {} in '{}' via {}", pAvatar->GetRawName(), weatherId, pWorld->GetName(), item.m_name);
        return false;
    }
    case ITEMTYPE_PROVIDER: {
        ProviderInfo provider = GetProviderInfo(pTile->m_foregroundId);
        if (provider.m_kind == ProviderYield::None)
            return false;

        if (!pTile->IsProviderReady(item.m_bloomTime))
            return false;

        switch (provider.m_kind) {
        case ProviderYield::Gems: {

            SpawnGemsDenominated(pWorld, static_cast<uint32_t>(1 + (std::rand() % 100)), centreX, centreY);
            break;
        }
        case ProviderYield::Chemical: {
            uint16_t chemical = RollScienceStationChemical(std::rand() % 16, std::rand() % 8, std::rand() % 6, std::rand() % 4);
            SpawnDropJittered(pWorld, chemical, 1, centreX, centreY);
            break;
        }
        case ProviderYield::Item: {
            SpawnDropJittered(pWorld, provider.m_itemId, static_cast<uint8_t>(1 + (std::rand() % 2)), centreX, centreY);
            break;
        }
        default:
            break;
        }

        pTile->m_lastHarvestUnix = static_cast<int64_t>(std::time(nullptr));
        pWorld->BroadcastTileUpdate(tileX, tileY);
        GetDatabase()->GetWorldTable()->Save(*pWorld);

        Logger::Print(INFO, "Player {} harvested {} at ({},{})", pAvatar->GetRawName(), item.m_name, tileX, tileY);
        return true;
    }
    case ITEMTYPE_DICE: {

        pTile->m_randomValue = pTile->m_foregroundId == 1300
            ? static_cast<uint8_t>(1 + (std::rand() % 3))
            : static_cast<uint8_t>(std::rand() % 6);
        pWorld->BroadcastTileUpdate(tileX, tileY);
        GetDatabase()->GetWorldTable()->Save(*pWorld);
        return false;
    }
    case ITEMTYPE_CHEST:
    case ITEMTYPE_SWITCHEROO:
    case ITEMTYPE_DEADLY_IF_ON:
    case ITEMTYPE_FOREGROUND_WITH_EXTRA_FRAME: {

        pTile->ToggleFlag(TILEFLAG_OPEN);
        pWorld->BroadcastTileUpdate(tileX, tileY);
        GetDatabase()->GetWorldTable()->Save(*pWorld);
        return false;
    }
    case ITEMTYPE_SFX_WITH_EXTRA_FRAME: {

        if (pTile->m_foregroundId != 758)
            return false;
        int number = std::rand() % 37;
        char colour = number == 0 ? '2' : (std::rand() % 3) < 2 ? 'b' : '4';
        std::string message = fmt::format("[`{}`` spun the wheel and got `{}{}``!]", pAvatar->GetFormattedName(), colour, number);
        for (auto* pOther : pWorld->GetPlayers()) {
            VarList::OnTalkBubble(pOther->Get(), pAvatar->GetNetId(), message);
            VarList::OnConsoleMessage(pOther->Get(), message);
        }
        return false;
    }
    default:
        return false;
    }
}

static void BroadcastAppearance(Player* pAvatar) {
    auto pWorld = pAvatar->GetWorld();
    if (!pWorld)
        return;

    TankPacketData state{};
    state.m_type = NET_GAME_PACKET_SET_CHARACTER_STATE;
    state.m_netId = pAvatar->GetNetId();
    state.m_jumpCount = 128;
    state.m_punchIndex = ItemEffects::GetActivePunchEffect(pAvatar->GetItems());
    state.m_animationType = 128;
    state.m_floatVariable = 150.0f;
    state.m_vectorX = 1000.0f;
    state.m_vectorY = 350.0f;
    state.m_vectorX2 = 200.0f;
    state.m_vectorY2 = 1000.0f;
    state.m_effectFlags = static_cast<int32_t>(ItemEffects::ComputeCharacterStateFlags(pAvatar));

    int32_t skin = ItemEffects::ComputeSkinColor(pAvatar);
    for (auto* pOther : pWorld->GetPlayers()) {
        VarList::OnSetClothing(pOther->Get(), pAvatar->GetNetId(), pAvatar->GetItems()->GetClothes(), skin);
        STankPacket statePacket(state);
        ENetWrapper::SendPacket(pOther->Get(), statePacket);
    }
}

static void BroadcastItemEffect(std::shared_ptr<World> pWorld, Player* pUser, int32_t targetNetId,
                                uint16_t itemId, float pixelX, float pixelY) {
    TankPacketData effect{};
    effect.m_type = NET_GAME_PACKET_ITEM_EFFECT;
    effect.m_animationType = 5;
    effect.m_netId = 0;
    effect.m_targetNetId = targetNetId;
    effect.m_intX = itemId;
    effect.m_intY = static_cast<uint32_t>(pUser->GetNetId());
    effect.m_vectorX = pixelX;
    effect.m_vectorY = pixelY;

    STankPacket packet(effect);
    pWorld->BroadcastPacket(packet);
}

static Player* FindPlayerOnTile(std::shared_ptr<World> pWorld, uint32_t tileX, uint32_t tileY) {
    for (auto* pOther : pWorld->GetPlayers()) {
        if (static_cast<uint32_t>(pOther->GetX() / 32.0f) == tileX &&
            static_cast<uint32_t>(pOther->GetY() / 32.0f) == tileY)
            return pOther;
    }
    return nullptr;
}

static bool HandlePlaymodItem(Player* pAvatar, std::shared_ptr<Server> pServer, std::shared_ptr<World> pWorld,
                              const Playmods::Info& info, const ItemInfo& item, uint32_t tileX, uint32_t tileY) {
    float centreX = tileX * 32.0f + 16.0f;
    float centreY = tileY * 32.0f + 16.0f;
    Player* pTarget = FindPlayerOnTile(pWorld, tileX, tileY);

    if (info.m_action != Playmods::Action::Throw && info.m_action != Playmods::Action::Pet && !pTarget) {
        VarList::OnTalkBubble(pAvatar->Get(), pAvatar->GetNetId(), "Must be used on a person.");
        SendInventoryState(pAvatar);
        return true;
    }

    auto* pItems = pAvatar->GetItems();
    uint16_t itemId = static_cast<uint16_t>(item.m_Id);

    if (info.m_action == Playmods::Action::Pet) {
        auto needed = static_cast<uint8_t>(info.m_durationSeconds);
        auto it = pItems->m_bpItems.find(itemId);
        if (it == pItems->m_bpItems.end() || it->second < needed) {

            VarList::OnTalkBubble(pAvatar->Get(), pAvatar->GetNetId(),
                fmt::format("Legend says that you need {} {}!", needed, item.m_name));
            SendInventoryState(pAvatar);
            return true;
        }
        auto rewardId = static_cast<uint16_t>(std::atoi(info.m_name));
        if (!pItems->AddItem(rewardId, 1)) {
            VarList::OnTalkBubble(pAvatar->Get(), pAvatar->GetNetId(), "You don't have enough inventory space!");
            SendInventoryState(pAvatar);
            return true;
        }
        pItems->RemoveItem(itemId, needed);
        GetDatabase()->GetPlayerTable()->Save(pAvatar);
        SendInventoryState(pAvatar);
        auto* pReward = GetItemManager()->GetItem(rewardId);
        VarList::OnTalkBubble(pAvatar->Get(), pAvatar->GetNetId(),
            fmt::format("`5{} combine to create a`` `2{}```5!``", item.m_name, pReward ? pReward->m_name : std::string()));
        CAction::PlaySFX(pAvatar->Get(), "terraform", 0);
        Logger::Print(INFO, "Player {} combined {} {} into a {}", pAvatar->GetRawName(), needed, item.m_name, pReward ? pReward->m_name : std::string());
        return true;
    }

    if (!pItems->RemoveItem(itemId, 1)) {
        SendInventoryState(pAvatar);
        return true;
    }

    if (info.m_action == Playmods::Action::CuteWords) {

        std::vector<std::string> words = { "`4'ADORE ME'``", "`4'SAY YES'``", "`4'MISS YOU'``" };
        bool kills = false;
        if (itemId == 618) { words = { "`4'ARGH!!!'``" }; kills = true; }
        else if (itemId == 616) words = { "`4'Awwwww!'``" };
        else if (itemId == 752) words = { "HEADS", "TAILS" };
        if (itemId == 276) kills = true;

        std::string phrase = words[std::rand() % words.size()];

        bool shouts = itemId != 276;

        std::string console;
        bool broadcastConsole = false;
        if (itemId == 618) {
            console = fmt::format("{} snuggled the wrong bunny!", pTarget->GetRawName());
            broadcastConsole = true;
        } else if (itemId != 616 && itemId != 752 && itemId != 2734 && itemId != 276) {
            console = fmt::format("{} shouts {} uncontrollably!", pTarget->GetRawName(), phrase);
            broadcastConsole = true;
        } else {
            console = fmt::format("{} used a {} on {}.", pAvatar->GetRawName(), item.m_name, pTarget->GetRawName());
        }

        for (auto* pOther : pWorld->GetPlayers()) {
            VarList::OnPlayPositioned(pOther->Get(), pTarget->GetNetId(), "audio/eat.wav");
            if (shouts && itemId != 2734)
                VarList::OnTalkBubble(pOther->Get(), pTarget->GetNetId(), phrase, itemId == 752 ? 500 : 0);

            if (itemId != 618 && itemId != 752 && itemId != 2734 && itemId != 276)
                VarList::OnAction(pOther->Get(), pTarget->GetNetId(), "/love");
            if (broadcastConsole)
                VarList::OnConsoleMessage(pOther->Get(), console);
        }
        if (!broadcastConsole) {
            VarList::OnConsoleMessage(pAvatar->Get(), console);
            if (pTarget != pAvatar)
                VarList::OnConsoleMessage(pTarget->Get(), console);
        }
        if (itemId == 2734)
            VarList::OnTalkBubble(pAvatar->Get(), pTarget->GetNetId(), ":D`#YUM!``");

        if (kills) {
            if (auto* pRespawn = GetEventPool()->ActionManager::GetEventIfExists("respawn")) {
                TextParse respawnParser(std::string("action|respawn\n"));
                pRespawn->sig_function(pTarget, pServer, std::string(), respawnParser, nullptr);
            }
        }
        BroadcastItemEffect(pWorld, pAvatar, pTarget->GetNetId(), itemId, pAvatar->GetX() + 10.0f, pAvatar->GetY() + 16.0f);
        GetDatabase()->GetPlayerTable()->Save(pAvatar);
        SendInventoryState(pAvatar);
        Logger::Print(INFO, "Player {} used {} on {}", pAvatar->GetRawName(), item.m_name, pTarget->GetRawName());
        return true;
    }

    if (info.m_action == Playmods::Action::Consume) {
        for (auto* pOther : pWorld->GetPlayers()) {
            VarList::OnPlayPositioned(pOther->Get(), pTarget->GetNetId(), "audio/eat.wav");
            VarList::OnTalkBubble(pOther->Get(), pTarget->GetNetId(), info.m_name);
        }

        VarList::OnConsoleMessage(pTarget->Get(), fmt::format("{} fed you a {}. {}", pAvatar->GetRawName(), item.m_name, info.m_name));
        if (pTarget != pAvatar)
            VarList::OnConsoleMessage(pAvatar->Get(), fmt::format("You fed {} a {}.", pTarget->GetRawName(), item.m_name));
        BroadcastItemEffect(pWorld, pAvatar, pTarget->GetNetId(), itemId, pAvatar->GetX() + 10.0f, pAvatar->GetY() + 16.0f);
        GetDatabase()->GetPlayerTable()->Save(pAvatar);
        SendInventoryState(pAvatar);
        Logger::Print(INFO, "Player {} fed {} to {}", pAvatar->GetRawName(), item.m_name, pTarget->GetRawName());
        return true;
    }

    if (info.m_action == Playmods::Action::Throw) {
        BroadcastParticleEffect(pWorld, info.m_effect,
            tileX * 32.0f + (std::rand() % 17), tileY * 32.0f + (std::rand() % 17), 0.0f, 0.0f);
        if (!pTarget) {
            GetDatabase()->GetPlayerTable()->Save(pAvatar);
            SendInventoryState(pAvatar);
            return true;
        }
    }

    if (itemId == 782) {
        for (uint16_t afflictionId : Playmods::GetAfflictionIds())
            pTarget->RemovePlaymod(afflictionId);
        std::string bubble = fmt::format("`w[{} `wfeels strangely better!]", pTarget->GetRawName());
        std::string console = fmt::format("`7[`w{} `ofeels strangely better!`7]", pTarget->GetRawName());
        for (auto* pOther : pWorld->GetPlayers()) {
            VarList::OnTalkBubble(pOther->Get(), pAvatar->GetNetId(), bubble);
            VarList::OnConsoleMessage(pOther->Get(), console);
        }
    }

    if (itemId == 732 || itemId == 278) {
        std::string modName(info.m_name);
        std::string lowered = modName;
        std::transform(lowered.begin(), lowered.end(), lowered.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        std::string suffix = (info.m_playmodId == 76) ? "" : "d";

        std::string announce = fmt::format("`#**`` `$The Gods`` have {}{} {}`o{} `#**`` (`4/rules`` to see the rules!)",
            lowered, suffix, pTarget->GetFormattedName(), info.m_playmodId == 11 ? "'s mouth" : "");
        for (auto& [serverId, pServerInst] : GetServerPool()->GetServers()) {
            auto pPool = pServerInst ? pServerInst->GetPlayerPool() : nullptr;
            if (!pPool)
                continue;
            for (auto& [peerId, pAny] : pPool->GetPlayers())
                if (pAny)
                    VarList::OnConsoleMessage(pAny->Get(), announce);
        }

        VarList::OnConsoleMessage(pTarget->Get(), fmt::format("`oWarning from `4System``: You've been `4{}{}`` for {}",
            modName, suffix, Playmods::FormatDuration(info.m_durationSeconds)));

        pTarget->AddPlaymod(info.m_playmodId, info.m_durationSeconds, pAvatar->GetRawName());

        if (itemId == 732) {

            pTarget->SetBan(std::time(nullptr) + info.m_durationSeconds, "Wand Effect");
            GetDatabase()->GetPlayerTable()->Save(pTarget);
            GetDatabase()->GetPlayerTable()->Save(pAvatar);
            SendInventoryState(pAvatar);
            pTarget->RequestDisconnect();
            Logger::Print(INFO, "Player {} ban-wanded {}", pAvatar->GetRawName(), pTarget->GetRawName());
            return true;
        }

        BroadcastAppearance(pTarget);
        GetDatabase()->GetPlayerTable()->Save(pAvatar);
        SendInventoryState(pAvatar);
        CommandUtils::SendToWorld(pTarget, "HELL");
        Logger::Print(INFO, "Player {} curse-wanded {} to HELL", pAvatar->GetRawName(), pTarget->GetRawName());
        return true;
    }

    if (itemId == 408) {
        pTarget->SetDuctTapedFor(info.m_durationSeconds);
        for (auto* pOther : pWorld->GetPlayers())
            VarList::OnTalkBubble(pOther->Get(), pTarget->GetNetId(), "`4mfmm mmfmfm!``");
    }

    bool applied = pTarget->AddPlaymod(info.m_playmodId, info.m_durationSeconds, pAvatar->GetRawName());

    if (applied && info.m_name[0]) {
        std::string message = info.m_onUsed[0]
            ? fmt::format("{} (`${}`` mod added, `${}`` left)", info.m_onUsed, info.m_name, Playmods::FormatDuration(info.m_durationSeconds))
            : fmt::format("`${}`` mod added, `${}`` left)", info.m_name, Playmods::FormatDuration(info.m_durationSeconds));
        VarList::OnConsoleMessage(pTarget->Get(), message);
    } else if (!applied) {

        VarList::OnConsoleMessage(pAvatar->Get(), fmt::format("{} already has that effect.", pTarget->GetRawName()));
    }

    if (itemId == 384 && applied) {
        std::string announce = fmt::format("`4>> `#{} is now `#{}``'s valentine!``",
            pTarget->GetFormattedName(), pAvatar->GetFormattedName());
        for (auto* pOther : pWorld->GetPlayers())
            VarList::OnConsoleMessage(pOther->Get(), announce);
    }

    if (info.m_sfx[0]) {
        std::string sound = fmt::format("audio/{}", info.m_sfx);
        for (auto* pOther : pWorld->GetPlayers())
            VarList::OnPlayPositioned(pOther->Get(), pTarget->GetNetId(), sound);
    }

    BroadcastAppearance(pTarget);
    BroadcastItemEffect(pWorld, pAvatar, pTarget->GetNetId(), itemId, pAvatar->GetX() + 10.0f, pAvatar->GetY() + 16.0f);
    GetDatabase()->GetPlayerTable()->Save(pAvatar);
    SendInventoryState(pAvatar);
    Logger::Print(INFO, "Player {} used {} on {}", pAvatar->GetRawName(), item.m_name, pTarget->GetRawName());
    return true;
}

struct CombineRecipe {
    uint16_t m_itemId;
    uint8_t m_needed;
    uint16_t m_rewardId;
    const char* m_message;
};
static const CombineRecipe kCombineRecipes[] = {
    { 1152,  10, 1150, nullptr },
    { 1212,  25, 1190, nullptr },
    { 1234,   4, 1206, nullptr },
    { 2000,  20, 1998, nullptr },
    { 2034, 200, 2035, nullptr },
    { 2036, 200, 2037, nullptr },
    { 2290, 100, 2282, nullptr },
    { 2410, 200, 2408, nullptr },
    { 2412,   4,  528, nullptr },

    { 2400,   4, 2402, "You've tamed an Irish Sport Horse!" },

    { 1976,  10, 1974, "SQUISH! You can only see what you made with the help of a Nightmare Magnifying Glass!" },

    { 2288, 100, 2284, nullptr },
};

static bool HandleCombineConsumable(Player* pAvatar, const ItemInfo& item) {
    const CombineRecipe* pRecipe = nullptr;
    for (const auto& recipe : kCombineRecipes) {
        if (recipe.m_itemId == item.m_Id) {
            pRecipe = &recipe;
            break;
        }
    }
    if (!pRecipe)
        return false;

    auto* pItems = pAvatar->GetItems();
    auto itemId = static_cast<uint16_t>(item.m_Id);
    auto it = pItems->m_bpItems.find(itemId);
    uint32_t held = (it == pItems->m_bpItems.end()) ? 0 : it->second;

    if (held < pRecipe->m_needed) {
        VarList::OnTalkBubble(pAvatar->Get(), pAvatar->GetNetId(),
            fmt::format("`4You'll need more {} than that!``", item.m_name));
        SendInventoryState(pAvatar);
        return true;
    }
    if (!pItems->AddItem(pRecipe->m_rewardId, 1)) {
        VarList::OnTalkBubble(pAvatar->Get(), pAvatar->GetNetId(), "You don't have enough inventory space!");
        SendInventoryState(pAvatar);
        return true;
    }
    pItems->RemoveItem(itemId, pRecipe->m_needed);
    GetDatabase()->GetPlayerTable()->Save(pAvatar);
    SendInventoryState(pAvatar);

    auto* pReward = GetItemManager()->GetItem(pRecipe->m_rewardId);
    VarList::OnTalkBubble(pAvatar->Get(), pAvatar->GetNetId(), pRecipe->m_message
        ? std::string(pRecipe->m_message)
        : fmt::format("`5{} combine to create a`` `2{}```5!``", item.m_name, pReward ? pReward->m_name : std::string()));
    Logger::Print(INFO, "Player {} combined {} {} into a {}", pAvatar->GetRawName(),
        pRecipe->m_needed, item.m_name, pReward ? pReward->m_name : std::string());
    return true;
}

static bool HandleTileConsumable(Player* pAvatar, std::shared_ptr<Server> pServer, std::shared_ptr<World> pWorld, Tile* pTile,
                                 const ItemInfo& item, uint32_t tileX, uint32_t tileY) {
    if (HandleCombineConsumable(pAvatar, item))
        return true;

    if (const auto* pPlaymod = Playmods::GetForItem(static_cast<uint16_t>(item.m_Id)))
        return HandlePlaymodItem(pAvatar, pServer, pWorld, *pPlaymod, item, tileX, tileY);

    float centreX = tileX * 32.0f + 16.0f;
    float centreY = tileY * 32.0f + 16.0f;

    bool consumed = false;
    switch (item.m_Id) {
    case 822: {
        pTile->ToggleFlag(TILEFLAG_WATER);
        consumed = true;
        break;
    }
    case 1866: {
        pTile->ToggleFlag(TILEFLAG_GLUE);
        consumed = true;
        break;
    }
    case 834:
    case 1406: {
        SpawnFireworks(pWorld, pAvatar->GetX() + 16.0f, pAvatar->GetY());

        if (item.m_Id == 834 && (std::rand() % 100) < 2) {
            VarList::OnTalkBubble(pAvatar->Get(), pAvatar->GetNetId(), "Received 1 Summer Surprise!");
            SpawnDropJittered(pWorld, 836, 1, pAvatar->GetX(), pAvatar->GetY());
        }
        consumed = true;
        break;
    }
    case 1680: {

        auto it = pAvatar->GetItems()->m_bpItems.find(static_cast<uint16_t>(ITEM_FIREWORKS));
        if (it == pAvatar->GetItems()->m_bpItems.end() || it->second < 200) {
            VarList::OnTalkBubble(pAvatar->Get(), pAvatar->GetNetId(), "Requires 200 Fireworks to launch!");
            SendInventoryState(pAvatar);
            return true;
        }
        pAvatar->GetItems()->RemoveItem(static_cast<uint16_t>(ITEM_FIREWORKS), 200);
        SpawnFireworks(pWorld, pAvatar->GetX() + 16.0f, pAvatar->GetY() - 32.0f);
        consumed = true;
        break;
    }
    case 722: {

        if (!pTile->IsForegroundEmpty()) {
            SendInventoryState(pAvatar);
            return true;
        }
        pTile->m_foregroundId = 718;
        for (auto* pOther : pWorld->GetPlayers()) {
            VarList::OnConsoleMessage(pOther->Get(), "`2`4Pi`wna`2ta`` `wBash``!:`` `oSmash the `wUltra`` `4Pi`wna`2ta``! `#Get some help``!``");
            CAction::PlaySFX(pOther->Get(), "cumbia_horns", 0);
        }
        consumed = true;
        break;
    }
    case 1066: {
        BroadcastParticleEffect(pWorld, 50, centreX, centreY, 0.0f, 0.0f);
        consumed = true;
        break;
    }
    case 1826: {
        BroadcastParticleEffect(pWorld, 92, centreX, centreY, 0.0f, 0.0f);
        consumed = true;
        break;
    }
    case 1828: {

        VarList::OnTalkBubble(pAvatar->Get(), pAvatar->GetNetId(), "Ew, it has raisins! I'm not eating that.");
        SendInventoryState(pAvatar);
        return true;
    }
    case 2306:

    case 1360: {

        VarList::OnTalkBubble(pAvatar->Get(), pAvatar->GetNetId(), "This does nothing right now.");
        SendInventoryState(pAvatar);
        return true;
    }
    case 1220: {

        VarList::OnTalkBubble(pAvatar->Get(), pAvatar->GetNetId(), "The sigil fizzles... try using it near Growganoth!");
        SendInventoryState(pAvatar);
        return true;
    }
    case 1488: {
        pAvatar->AddXp(10000);
        VarList::OnTalkBubble(pAvatar->Get(), pAvatar->GetNetId(), "`#GULP! You got smarter!");
        consumed = true;
        break;
    }
    case 1050: {

        static const char* kSecrets[] = {
            "Rarity affects how many gems a block drops, but not its seed chance",
            "Breaking blocks and harvesting trees is what earns you experience",
            "A tree's fruit count is set when you plant it, not when it grows",
            "Splicing two different seeds together can make a third kind of block",
            "100 World Locks compress into a single Diamond Lock",
        };
        VarList::OnConsoleMessage(pAvatar->Get(),
            fmt::format("`6The mysterious secret is...``\n `2{}``", kSecrets[std::rand() % (sizeof(kSecrets) / sizeof(kSecrets[0]))]));
        consumed = true;
        break;
    }
    case 2480: {

        VarList::OnDialogRequest(pAvatar->Get(),
            "set_default_color|`o\n"
            "add_label_with_icon|big|`wMegaphone``|left|2480|\n"
            "add_textbox|Enter a message you want to broadcast to every player in Growtopia! This will use up 1 Megaphone.|left|\n"
            "add_text_input|words|||128|\n"
            "embed_data|itemID|2480\n"
            "end_dialog|megaphone|Nevermind|Broadcast|\n");
        SendInventoryState(pAvatar);
        return true;
    }
    case 1280: {

        VarList::OnDialogRequest(pAvatar->Get(),
            "set_default_color|`o\n"
            "add_label_with_icon|big|`wChange your GrowID``|left|1280|\n"
            "add_smalltext|This will change your GrowID `4permanently``.<CR>Your `wBirth Certificate`` will be consumed if you press `5Change It``.<CR>NOTE: The birth certificate only will change your name case (you can not change your whole GrowID)!``|left|\n"
            "add_textbox|Enter your new name:|left|\n"
            "add_text_input|name_box|||32|\n"
            "add_spacer|small|\n"
            "end_dialog|name_change|Cancel|Change it!|");
        SendInventoryState(pAvatar);
        return true;
    }
    case 2580: {
        VarList::OnDialogRequest(pAvatar->Get(),
            "set_default_color|`o\n"
            "add_label_with_icon|big|`wSwap World Names``|left|2580|\n"
            "add_smalltext|This will swap the name of the world you are standing in with another world `4permanently``.  You must own both worlds, with a World Lock in place.<CR>Your `wChange of Address`` will be consumed if you press `5Swap 'Em``.|left|\n"
            "add_textbox|Enter the other world's name:|left|\n"
            "add_text_input|name_box|||32|\n"
            "add_spacer|small|\n"
            "end_dialog|world_swap|Cancel|Swap 'Em!|");
        SendInventoryState(pAvatar);
        return true;
    }
    case 830:
    case 942:
    case 1060:
    case 1136:
    case 1402:
    case 1532: {

        if (item.m_Id == 830) {
            auto it = pAvatar->GetItems()->m_bpItems.find(static_cast<uint16_t>(ITEM_FIREWORKS));
            if (it == pAvatar->GetItems()->m_bpItems.end() || it->second < 100) {
                VarList::OnTalkBubble(pAvatar->Get(), pAvatar->GetNetId(), "Beach blast requires 100 Fireworks.");
                SendInventoryState(pAvatar);
                return true;
            }
        }
        pAvatar->SetPendingBlastItem(static_cast<uint16_t>(item.m_Id));
        VarList::OnDialogRequest(pAvatar->Get(), fmt::format(
            "set_default_color|`o\n"
            "add_label_with_icon|big|`0{}``|left|{}|\n"
            "add_textbox|This item creates a new world! Enter a unique name for it.|left||\n"
            "add_text_input|name|New World Name||256|\n"
            "end_dialog|blast|Cancel|`5Create!``|\n", item.m_name, item.m_Id));
        SendInventoryState(pAvatar);
        return true;
    }
    default:

        VarList::OnTalkBubble(pAvatar->Get(), pAvatar->GetNetId(), "This does nothing right now.");
        VarList::OnConsoleMessage(pAvatar->Get(), "This does nothing right now.");
        SendInventoryState(pAvatar);
        return true;
    }

    if (!consumed)
        return true;

    if (!pAvatar->GetItems()->RemoveItem(static_cast<uint16_t>(item.m_Id), 1)) {
        SendInventoryState(pAvatar);
        return true;
    }
    GetDatabase()->GetPlayerTable()->Save(pAvatar);
    pWorld->BroadcastTileUpdate(tileX, tileY);
    GetDatabase()->GetWorldTable()->Save(*pWorld);

    Logger::Print(INFO, "Player {} used {} on tile ({},{})", pAvatar->GetRawName(), item.m_name, tileX, tileY);
    return true;
}

static void HandleTileChangeRequest(Player* pAvatar, std::shared_ptr<Server> pServer, TankPacketData* pTankData) {

    if (pAvatar->GetTradingWithNetId() != -1) {
        auto* pTradePartner = TradeManager::FindTradePartner(pAvatar);
        VarList::OnConsoleMessage(pAvatar->Get(), "Trade cancelled - you can't break/place blocks while trading.");
        if (pTradePartner)
            VarList::OnConsoleMessage(pTradePartner->Get(), fmt::format("Trade with {} was cancelled.", pAvatar->GetFormattedName()));
        TradeManager::CancelTrade(pAvatar);
    }

    auto pWorld = pAvatar->GetWorld();
    uint32_t tileX = pTankData->m_tilePositionX;
    uint32_t tileY = pTankData->m_tilePositionY;

    Logger::Print(INFO, "TileChangeRequest from {}: tile=({},{}) requestedItem={}", pAvatar->GetRawName(), tileX, tileY, pTankData->m_itemId);

    auto* pTile = pWorld->GetTile(tileX, tileY);
    if (!pTile) {
        Logger::Print(WARNING, "TileChangeRequest: tile ({},{}) out of bounds", tileX, tileY);
        return;
    }

    int32_t requestedItemId = pTankData->m_itemId;

    if (requestedItemId == ITEM_WRENCH) {
        LockInfo* pAnchorLock = pWorld->FindLockByAnchor(tileX, tileY);
        if (pAnchorLock && pWorld->CanEdit(tileX, tileY, pAvatar->GetUserId())) {
            LockHelpers::SendLockEditDialog(pAvatar, tileX, tileY, *pAnchorLock);
            return;
        }

        LockInfo* pWorldLock = pWorld->GetWorldLock();
        if (pWorldLock && pWorldLock->m_anchorX == tileX && pWorldLock->m_anchorY == tileY && pWorld->CanEdit(tileX, tileY, pAvatar->GetUserId())) {
            LockHelpers::SendWorldLockEditDialog(pAvatar, *pWorldLock);
            return;
        }

        if (TileDialogs::TryOpenTileDialog(pAvatar, tileX, tileY))
            return;
    }

    const LockInfo* pAnchorLockHere = pWorld->FindLockByAnchor(tileX, tileY);
    if (!pAnchorLockHere) {
        const LockInfo* pWorldLockAnchorHere = pWorld->GetWorldLock();
        if (pWorldLockAnchorHere && pWorldLockAnchorHere->m_anchorX == tileX && pWorldLockAnchorHere->m_anchorY == tileY)
            pAnchorLockHere = pWorldLockAnchorHere;
    }
    bool sentOwnershipBubble = pAnchorLockHere && pAnchorLockHere->m_ownerId != pAvatar->GetUserId();
    if (sentOwnershipBubble)
        LockHelpers::SendLockOwnershipBubble(pAvatar, *pAnchorLockHere);

    bool punchingPublicItem = false;
    if (requestedItemId == ITEM_BLANK || requestedItemId == ITEM_FIST) {
        uint16_t standingId = pTile->IsForegroundEmpty() ? pTile->m_backgroundId : pTile->m_foregroundId;
        auto* pStanding = standingId != ITEM_BLANK ? GetItemManager()->GetItem(standingId) : nullptr;
        punchingPublicItem = pStanding && pStanding->IsPublic();
    }

    if (!punchingPublicItem && !pWorld->CanEdit(tileX, tileY, pAvatar->GetUserId())) {

        if (!sentOwnershipBubble)
            VarList::OnPlayPositioned(pAvatar->Get(), pAvatar->GetNetId(), "audio/punch_locked.wav");
        return;
    }

    {
        auto* pRequestedItem = GetItemManager()->GetItem(requestedItemId);
        if (pRequestedItem && (pRequestedItem->m_name == "Grow Spray Fertilizer" || pRequestedItem->m_name == "Deluxe Grow Spray" || pRequestedItem->m_name == "Ultra Grow Spray")) {
            int64_t ageSeconds = pRequestedItem->m_name == "Ultra Grow Spray" ? 604800
                : pRequestedItem->m_name == "Deluxe Grow Spray" ? 86400 : 3600;
            if (!pTile->IsForegroundEmpty() && pTile->m_plantedAtUnix != 0 &&
                !pTile->IsFullyGrown(GetItemManager()->GetItem(pTile->m_foregroundId)->m_bloomTime)) {
                if (!pAvatar->GetItems()->RemoveItem(static_cast<uint16_t>(requestedItemId), 1))
                    return;
                pTile->m_plantedAtUnix -= ageSeconds;
                GetDatabase()->GetPlayerTable()->Save(pAvatar);
                SendInventoryState(pAvatar);
                GetDatabase()->GetWorldTable()->Save(*pWorld);

                {
                    int64_t elapsed = static_cast<int64_t>(std::time(nullptr)) - pTile->m_plantedAtUnix;
                    TankPacketData tileUpdate{};
                    tileUpdate.m_type = NET_GAME_PACKET_SEND_TILE_UPDATE_DATA;
                    tileUpdate.m_netId = -1;
                    tileUpdate.m_tilePositionX = tileX;
                    tileUpdate.m_tilePositionY = tileY;

                    BinaryWriter tw(14);
                    tw.Write<uint16_t>(pTile->m_foregroundId);
                    tw.Write<uint16_t>(0);
                    tw.Write<uint16_t>(0);
                    tw.Write<uint16_t>(0x11);
                    tw.Write<uint8_t>(4);
                    tw.Write<uint32_t>(static_cast<uint32_t>(elapsed < 0 ? 0 : elapsed));
                    tw.Write<uint8_t>(pTile->m_fruitCount);
                    std::vector<uint8_t> tileData(tw.Get(), tw.Get() + tw.GetPosition());

                    SExtendedTankPacket tileUpdatePacket(tileUpdate, tileData);
                    pWorld->BroadcastPacket(tileUpdatePacket);
                }

                VarList::OnPlayPositioned(pAvatar->Get(), pAvatar->GetNetId(), "audio/use_lock.wav");
                std::string ageLabel = ageSeconds == 604800 ? "1 week" : ageSeconds == 86400 ? "1 day" : "1 hour";
                VarList::OnTalkBubble(pAvatar->Get(), pAvatar->GetNetId(), fmt::format("Aged this plant by {}.", ageLabel));
                Logger::Print(INFO, "Player {} used {} on tile ({},{})", pAvatar->GetRawName(), pRequestedItem->m_name, tileX, tileY);
            }
            return;
        }
    }

    if (requestedItemId == ITEM_BLANK || requestedItemId == ITEM_FIST) {

        const LockInfo* pWorldLockHere = pWorld->GetWorldLock();
        bool isWorldLockAnchor = pWorldLockHere && pWorldLockHere->m_anchorX == tileX && pWorldLockHere->m_anchorY == tileY;
        const LockInfo* pRelevantLock = pAnchorLockHere ? pAnchorLockHere : (isWorldLockAnchor ? pWorldLockHere : nullptr);
        if (pRelevantLock && !punchingPublicItem) {
            bool isOwner = pRelevantLock->m_ownerId == pAvatar->GetUserId();
            if (!isOwner) {
                if (!sentOwnershipBubble)
                    VarList::OnPlayPositioned(pAvatar->Get(), pAvatar->GetNetId(), "audio/punch_locked.wav");
                return;
            }
        }

        bool targetIsBackground = pTile->IsForegroundEmpty();
        if (targetIsBackground && pTile->IsBackgroundEmpty()) {

            Logger::Print(INFO, "TileChangeRequest: tile ({},{}) has nothing to break", tileX, tileY);
            return;
        }
        uint16_t& targetId = targetIsBackground ? pTile->m_backgroundId : pTile->m_foregroundId;

        auto* pCurrentItem = GetItemManager()->GetItem(targetId);
        if (!pCurrentItem) {
            Logger::Print(WARNING, "TileChangeRequest: no ItemInfo for {} id {}", targetIsBackground ? "background" : "foreground", targetId);
            return;
        }

        if (!targetIsBackground && HandlePunchInteraction(pAvatar, pWorld, pTile, *pCurrentItem, tileX, tileY))
            return;

        bool staffBypass = pAvatar->GetRole() != PlayerRole::Default && targetId != ITEM_MAIN_DOOR;
        if (!targetIsBackground && !staffBypass && pCurrentItem->IsModOnly()) {
            std::string bubbleMsg = fmt::format("`w{}``", pCurrentItem->IsDoor()
                ? "(stand over and punch to use)"
                : "It's too strong to break.");
            CAction::PlaySFX(pAvatar->Get(), "cant_break_tile", 0);
            auto bubbleList = VariantList::Create("OnTalkBubble");
            bubbleList.Insert(pAvatar->GetNetId());
            bubbleList.Insert(bubbleMsg);
            bubbleList.Insert(static_cast<uint32_t>(1));
            ENetWrapper::SendVariantList(pAvatar->Get(), bubbleList);
            return;
        }

        if (!targetIsBackground && pCurrentItem->IsSeed()) {
            if (!pTile->IsFullyGrown(pCurrentItem->m_bloomTime)) {
                Logger::Print(INFO, "TileChangeRequest: seed at ({},{}) not fully grown yet", tileX, tileY);
                TankPacketData notReady{};
                notReady.m_type = NET_GAME_PACKET_TILE_APPLY_DAMAGE;
                notReady.m_netId = pAvatar->GetNetId();
                notReady.m_tilePositionX = tileX;
                notReady.m_tilePositionY = tileY;
                notReady.m_tileDamage = 7;
                STankPacket notReadyPacket(notReady);
                pWorld->BroadcastPacket(notReadyPacket);
                return;
            }

            uint16_t seedId = targetId;
            uint16_t blockId = seedId - 1;
            uint8_t plantedFruitCount = pTile->m_fruitCount;
            pTile->ClearForeground();

            float harvestX = tileX * 32.0f + 16.0f;
            float harvestY = tileY * 32.0f + 16.0f;

            const auto* pBlockItem = GetItemManager()->GetItem(blockId);

            uint16_t itemRarity = pBlockItem ? pBlockItem->m_rarity : 1;
            uint32_t yieldMin = itemRarity + 4;
            uint32_t yieldMax = itemRarity * 3 + 11;
            uint8_t blockCount = static_cast<uint8_t>(yieldMin + (std::rand() % (yieldMax - yieldMin + 1)));
            SpawnDropJittered(pWorld, blockId, blockCount, harvestX, harvestY);

            bool dropsExtraSeed = false;
            if (pBlockItem && !pBlockItem->NeverDropsSeed()) {
                dropsExtraSeed = RandFloat() <= kSeedDropChance;
                if (dropsExtraSeed) {
                    SpawnDropJittered(pWorld, seedId, 1, harvestX, harvestY);

                    auto* pSeedInfo = GetItemManager()->GetItem(seedId);
                    VarList::OnTalkBubble(pAvatar->Get(), pAvatar->GetNetId(),
                        fmt::format("A {} falls out!", pSeedInfo ? pSeedInfo->m_name : "Seed"));
                }
            }
            uint8_t gemCount = pBlockItem ? RollGems(*pBlockItem, pWorld, harvestX, harvestY) : 0;

            Logger::Print(INFO, "TileChangeRequest: harvested seed {} (fruit={}) at ({},{}) -> block {} x{} (extraSeed={}, gems={})",
                seedId, plantedFruitCount, tileX, tileY, blockId, blockCount, dropsExtraSeed, gemCount);

            TankPacketData treeState{};
            treeState.m_type = NET_GAME_PACKET_SEND_TILE_TREE_STATE;
            treeState.m_netId = pAvatar->GetNetId();
            treeState.m_item = -1;
            treeState.m_mainData = 7;
            treeState.m_tilePositionX = tileX;
            treeState.m_tilePositionY = tileY;
            STankPacket treeStatePacket(treeState);
            pWorld->BroadcastPacket(treeStatePacket);
            GetDatabase()->GetWorldTable()->Save(*pWorld);

            pAvatar->AddXp(XpForRarity(pBlockItem ? pBlockItem->m_rarity : 0));
            return;
        }

        constexpr uint8_t kFistPunchPower = 5;

        uint8_t effectiveBreakHits = ItemEffects::HasEnhancedDigging(pAvatar->GetItems()) && pCurrentItem->m_breakHits > kFistPunchPower
            ? pCurrentItem->m_breakHits - kFistPunchPower
            : pCurrentItem->m_breakHits;

        pTile->ExpireBreakProgressIfStale();

        if (pAvatar->IsOneHitEnabled())
            pTile->m_currentBreakHits = effectiveBreakHits;
        else
            pTile->m_currentBreakHits += kFistPunchPower;
        pTile->m_lastHitUnix = static_cast<int64_t>(std::time(nullptr));
        Logger::Print(INFO, "TileChangeRequest: break damage {}/{} on {} item {}", pTile->m_currentBreakHits, effectiveBreakHits, targetIsBackground ? "background" : "foreground", targetId);

        if (pTile->m_currentBreakHits < effectiveBreakHits) {

            TankPacketData damage{};
            damage.m_type = NET_GAME_PACKET_TILE_APPLY_DAMAGE;
            damage.m_netId = pAvatar->GetNetId();
            damage.m_tilePositionX = tileX;
            damage.m_tilePositionY = tileY;

            damage.m_tileDamage = 7;
            STankPacket damagePacket(damage);
            pWorld->BroadcastPacket(damagePacket);

            if (ItemEffects::HasLaserVisor(pAvatar->GetItems()))
                VarList::OnPlayPositioned(pAvatar->Get(), pAvatar->GetNetId(), "audio/laser.wav");

            return;
        }

        uint16_t brokenItemId = targetId;
        if (targetIsBackground) {
            targetId = ITEM_BLANK;
            pTile->ResetBreakProgress();
        } else {
            pTile->ClearForeground();
        }
        Logger::Print(INFO, "TileChangeRequest: broke tile ({},{})", tileX, tileY);

        float dropX = tileX * 32.0f + 16.0f;
        float dropY = tileY * 32.0f + 16.0f;

        if (pCurrentItem->IsPermanent() || pCurrentItem->IsLock()) {
            pAvatar->GetItems()->AddItem(brokenItemId, 1);
            GetDatabase()->GetPlayerTable()->Save(pAvatar);
            SendInventoryState(pAvatar);

            if (LockHelpers::IsWorldLockItem(pCurrentItem->m_name))
                pWorld->ClearWorldLock();
            else if (LockHelpers::GetLockCapacity(pCurrentItem->m_name) > 0)
                pWorld->RemoveTileLockAt(tileX, tileY);

            Logger::Print(INFO, "TileChangeRequest: lock {} broken, returned directly to {}'s inventory", brokenItemId, pAvatar->GetRawName());
        } else {

            bool canDropSeed = !pCurrentItem->NeverDropsSeed() && GetItemManager()->GetItem(brokenItemId + 1) != nullptr;
            bool canDropSelf = !pCurrentItem->NeverDropsSelf();

            if (pCurrentItem->m_Id == 542) {
                if ((std::rand() % 200) < 1)
                    SpawnDropJittered(pWorld, 2574, 1, dropX, dropY);
                else
                    SpawnDropJittered(pWorld, ITEM_GEMS, static_cast<uint8_t>(std::rand() % 101), dropX, dropY);
                pAvatar->AddXp(XpForRarity(pCurrentItem->m_rarity));
                pWorld->BroadcastTileUpdate(tileX, tileY);
                GetDatabase()->GetWorldTable()->Save(*pWorld);
                Logger::Print(INFO, "Player {} broke a Pot O' Gold at ({},{})", pAvatar->GetRawName(), tileX, tileY);
                return;
            }
            if (pCurrentItem->m_Id == 120) {
                SpawnDropJittered(pWorld, ITEM_GEMS, static_cast<uint8_t>(std::rand() % 51), dropX, dropY);
                pAvatar->AddXp(XpForRarity(pCurrentItem->m_rarity));
                pWorld->BroadcastTileUpdate(tileX, tileY);
                GetDatabase()->GetWorldTable()->Save(*pWorld);
                Logger::Print(INFO, "Player {} broke a Mystery Block at ({},{})", pAvatar->GetRawName(), tileX, tileY);
                return;
            }

            if (pCurrentItem->m_itemType == ITEMTYPE_SPOTLIGHT && !pTile->m_spotlightName.empty()) {
                for (auto* pOther : pWorld->GetPlayers()) {
                    if (pOther->GetRawName() != pTile->m_spotlightName)
                        continue;
                    pOther->RemovePlaymod(901);
                    VarList::OnConsoleMessage(pOther->Get(), "Back to anonymity. (`$In the Spotlight`` mod removed)");
                    VarList::OnTalkBubble(pOther->Get(), pOther->GetNetId(), "Lights out!");
                    BroadcastAppearance(pOther);
                }
                pTile->m_spotlightName.clear();
            }

            uint8_t gemsDropped = RollGems(*pCurrentItem, pWorld, dropX, dropY);

            pAvatar->AddXp(XpForRarity(pCurrentItem->m_rarity));

            float seedBand = canDropSeed ? kSeedDropChance : 0.0f;
            float selfBand = canDropSelf ? (1.0f - kSeedDropChance - kNothingDropChance) : 0.0f;

            float roll = RandFloat();
            const char* outcome;
            if (roll < seedBand) {
                uint16_t seedId = brokenItemId + 1;
                SpawnDropJittered(pWorld, seedId, 1, dropX, dropY);
                outcome = "seed";
            } else if (roll < seedBand + selfBand) {
                SpawnDropJittered(pWorld, brokenItemId, 1, dropX, dropY);
                outcome = "block";
            } else {
                outcome = "nothing extra";
            }
            Logger::Print(INFO, "TileChangeRequest: block {} broken (outcome={}, gems={})", brokenItemId, outcome, gemsDropped);
        }

        BroadcastTileConfirm(pWorld, pAvatar->GetNetId(), tileX, tileY, ITEM_FIST);
        GetDatabase()->GetWorldTable()->Save(*pWorld);
        return;
    } else {

        auto* pRequestedItem = GetItemManager()->GetItem(requestedItemId);
        if (!pRequestedItem)
            return;

        if (pRequestedItem->IsModOnly() && pAvatar->GetRole() == PlayerRole::Default) {
            VarList::OnPlayPositioned(pAvatar->Get(), pAvatar->GetNetId(), "audio/punch_locked.wav");
            return;
        }

        if (pRequestedItem->RequiresWorldLock() && !pWorld->HasWorldLock()) {
            VarList::OnTalkBubble(pAvatar->Get(), pAvatar->GetNetId(),
                fmt::format("`w{}`` can only be used in World-Locked worlds.", pRequestedItem->m_name));
            SendInventoryState(pAvatar);
            return;
        }

        if (requestedItemId == ITEM_DOOR_MOVER) {
            const LockInfo* pWorldLock = pWorld->GetWorldLock();
            if (!pWorldLock || pWorldLock->m_ownerId != pAvatar->GetUserId()) {
                VarList::OnTalkBubble(pAvatar->Get(), pAvatar->GetNetId(), "You can only use this on a world you have World Locked.");
                return;
            }

            Tile* pBelowTile = pWorld->GetTile(tileX, tileY + 1);
            if (!pTile->IsForegroundEmpty() || !pBelowTile || !pBelowTile->IsForegroundEmpty()) {
                CAction::PlaySFX(pAvatar->Get(), "cant_break_tile", 0);
                VarList::OnTalkBubble(pAvatar->Get(), pAvatar->GetNetId(), "It's too tight for a door here.");
                return;
            }

            bool hadOldDoor = false;
            uint32_t oldDoorX = 0, oldDoorY = 0;
            if (pWorld->FindDoorTile(oldDoorX, oldDoorY)) {
                hadOldDoor = true;
                Tile* pOldDoorTile = pWorld->GetTile(oldDoorX, oldDoorY);
                if (pOldDoorTile)
                    pOldDoorTile->ClearForeground();

                Tile* pOldBelowTile = pWorld->GetTile(oldDoorX, oldDoorY + 1);
                if (pOldBelowTile && pOldBelowTile->m_foregroundId == ITEM_BEDROCK)
                    pOldBelowTile->ClearForeground();
            }

            if (!pAvatar->GetItems()->RemoveItem(static_cast<uint16_t>(requestedItemId), 1)) {
                SendInventoryState(pAvatar);
                return;
            }

            pTile->m_foregroundId = ITEM_MAIN_DOOR;
            pTile->m_hasDoorExtra = true;
            pTile->m_doorExtra.m_label = "EXIT";
            pBelowTile->m_foregroundId = ITEM_BEDROCK;

            GetDatabase()->GetPlayerTable()->Save(pAvatar);
            GetDatabase()->GetWorldTable()->Save(*pWorld);

            if (hadOldDoor) {
                pWorld->BroadcastTileUpdate(oldDoorX, oldDoorY);
                pWorld->BroadcastTileUpdate(oldDoorX, oldDoorY + 1);
            }
            pWorld->BroadcastTileUpdate(tileX, tileY);
            pWorld->BroadcastTileUpdate(tileX, tileY + 1);
            Logger::Print(INFO, "Player {} moved the main door to ({},{}) in world '{}'", pAvatar->GetRawName(), tileX, tileY, pWorld->GetName());
            return;
        }

        if (pRequestedItem->m_itemType == ITEMTYPE_CONSUMABLE &&
            HandleTileConsumable(pAvatar, pServer, pWorld, pTile, *pRequestedItem, tileX, tileY))
            return;

        bool isBackground = pRequestedItem->IsBackground();
        if (isBackground ? !pTile->IsBackgroundEmpty() : !pTile->IsForegroundEmpty()) {

            if (!isBackground && pRequestedItem->IsSeed() && !pTile->IsForegroundEmpty()) {
                auto* pExistingItem = GetItemManager()->GetItem(pTile->m_foregroundId);
                if (pExistingItem && pExistingItem->IsSeed()) {
                    uint16_t existingSeedId = pTile->m_foregroundId;
                    uint16_t newSeedId = static_cast<uint16_t>(requestedItemId);
                    uint16_t resultSeedId = FindSpliceResult(existingSeedId, newSeedId);
                    auto* pExistingInfo = GetItemManager()->GetItem(existingSeedId);
                    auto* pNewInfo = GetItemManager()->GetItem(newSeedId);

                    if (resultSeedId == 0) {

                        SendInventoryState(pAvatar);

                        VarList::OnTalkBubble(pAvatar->Get(), pAvatar->GetNetId(),
                            fmt::format("Hmm, it looks like `w{}`` and `w{}`` can't be spliced.",
                                pExistingInfo ? pExistingInfo->m_name : "?", pNewInfo ? pNewInfo->m_name : "?"));
                        return;
                    }

                    if (!pAvatar->GetItems()->RemoveItem(newSeedId, 1)) {
                        SendInventoryState(pAvatar);
                        return;
                    }

                    GetDatabase()->GetPlayerTable()->Save(pAvatar);

                    CAction::PlaySFX(pAvatar->Get(), "success", 0);

                    BroadcastTileConfirm(pWorld, pAvatar->GetNetId(), tileX, tileY, newSeedId);

                    pTile->m_foregroundId = resultSeedId;
                    pTile->SetPlanted();
                    pTile->ResetBreakProgress();

                    {
                        TankPacketData tileUpdate{};
                        tileUpdate.m_type = NET_GAME_PACKET_SEND_TILE_UPDATE_DATA;
                        tileUpdate.m_netId = -1;
                        tileUpdate.m_tilePositionX = tileX;
                        tileUpdate.m_tilePositionY = tileY;

                        BinaryWriter tw(14);
                        tw.Write<uint16_t>(resultSeedId);
                        tw.Write<uint16_t>(0);
                        tw.Write<uint16_t>(0);
                        tw.Write<uint16_t>(0x11);
                        tw.Write<uint8_t>(4);
                        tw.Write<uint32_t>(0);
                        tw.Write<uint8_t>(pTile->m_fruitCount);
                        std::vector<uint8_t> tileData(tw.Get(), tw.Get() + tw.GetPosition());

                        SExtendedTankPacket tileUpdatePacket(tileUpdate, tileData);
                        pWorld->BroadcastPacket(tileUpdatePacket);
                    }

                    auto* pResultBlockInfo = GetItemManager()->GetItem(resultSeedId - 1);
                    std::string resultBlockName = pResultBlockInfo ? pResultBlockInfo->m_name : "?";

                    {
                        std::string bubbleMsg = fmt::format("{} and {} have been spliced to make a `${} Tree``!",
                            pExistingInfo ? pExistingInfo->m_name : "?", pNewInfo ? pNewInfo->m_name : "?",
                            resultBlockName);
                        VarList::OnTalkBubble(pAvatar->Get(), pAvatar->GetNetId(), bubbleMsg);
                    }

                    GetDatabase()->GetWorldTable()->Save(*pWorld);
                    return;
                }
            }

            SendInventoryState(pAvatar);
            return;
        }

        if (!isBackground && pRequestedItem->IsLock() && LockHelpers::IsWorldLockItem(pRequestedItem->m_name)) {
            const LockInfo* pExistingWL = pWorld->GetWorldLock();
            bool blocked = pExistingWL && (pExistingWL->m_ownerId != pAvatar->GetUserId() ||
                pExistingWL->m_anchorX != tileX || pExistingWL->m_anchorY != tileY);
            if (blocked) {
                std::string message = (pExistingWL->m_ownerId == pAvatar->GetUserId())
                    ? fmt::format("`wOnly one `${}`` can be placed in a world, you'd have to remove the other one first.", pRequestedItem->m_name)
                    : "This world already has a lock on it that isn't yours.";
                auto vList = VariantList::Create("OnTalkBubble");
                vList.Insert(pAvatar->GetNetId());
                vList.Insert(message);
                vList.Insert(static_cast<int32_t>(0));
                vList.Insert(static_cast<int32_t>(1));
                ENetWrapper::SendVariantList(pAvatar->Get(), vList);
                return;
            }
        }

        if (!pAvatar->GetItems()->RemoveItem(static_cast<uint16_t>(requestedItemId), 1)) {
            SendInventoryState(pAvatar);
            return;
        }

        uint8_t plantedFruitCount = 0;
        bool plantedSeed = false;
        if (isBackground) {
            pTile->m_backgroundId = static_cast<uint16_t>(requestedItemId);
        } else {
            pTile->m_foregroundId = static_cast<uint16_t>(requestedItemId);

            if (pAvatar->IsFacingLeft())
                pTile->SetFlag(TILEFLAG_FLIPPED);
            else
                pTile->ClearFlag(TILEFLAG_FLIPPED);

            if (pRequestedItem->m_itemType == ITEMTYPE_GATEWAY)
                pTile->SetFlag(TILEFLAG_PUBLIC);

            if (pRequestedItem->m_itemType == ITEMTYPE_PROVIDER)
                pTile->m_lastHarvestUnix = static_cast<int64_t>(std::time(nullptr));

            if (pRequestedItem->IsSeed()) {
                pTile->SetPlanted();
                plantedFruitCount = pTile->m_fruitCount;
                plantedSeed = true;
            }

            else if (pRequestedItem->m_itemType == ITEMTYPE_SIGN) {
                pTile->m_hasSignExtra = true;
                pTile->m_signExtra.m_text.clear();
            }

            else if (pRequestedItem->IsDoor()) {
                pTile->m_hasDoorExtra = true;
                pTile->m_doorExtra = {};
            }
        }
        pTile->ResetBreakProgress();

        BroadcastTileConfirm(pWorld, pAvatar->GetNetId(), tileX, tileY, static_cast<uint16_t>(requestedItemId), plantedFruitCount, plantedSeed);
        GetDatabase()->GetWorldTable()->Save(*pWorld);
        GetDatabase()->GetPlayerTable()->Save(pAvatar);

        if (!isBackground && pRequestedItem->IsLock())
            HandleLockPlaced(pAvatar, pWorld, tileX, tileY, static_cast<uint16_t>(requestedItemId));
    }
}

static void HandleItemActivateObjectRequest(Player* pAvatar, TankPacketData* pTankData) {
    auto pWorld = pAvatar->GetWorld();

    int32_t uid = pTankData->m_itemId;

    DroppedItem drop;
    if (!pWorld->RemoveDrop(uid, drop)) {
        Logger::Print(INFO, "ItemActivateObjectRequest from {}: no drop with uid={} (already picked up?)", pAvatar->GetRawName(), uid);
        return;
    }

    if (drop.m_itemId == ITEM_GEMS) {
        pAvatar->GetItems()->m_gems += drop.m_count;

        VarList::OnSetBux(pAvatar->Get(), pAvatar->GetItems()->GetGems());

    } else {

        uint8_t taken = pAvatar->GetItems()->AddItemPartial(drop.m_itemId, drop.m_count);
        if (taken == 0) {

            pWorld->SpawnDrop(drop.m_itemId, drop.m_count, drop.m_x, drop.m_y);
            BroadcastDropSpawn(pWorld, drop.m_itemId, drop.m_count, drop.m_x, drop.m_y);
            VarList::OnTextOverlay(pAvatar->Get(), "Your backpack is full!");
            return;
        }
        if (taken < drop.m_count) {
            uint8_t left = static_cast<uint8_t>(drop.m_count - taken);
            pWorld->SpawnDrop(drop.m_itemId, left, drop.m_x, drop.m_y);
            BroadcastDropSpawn(pWorld, drop.m_itemId, left, drop.m_x, drop.m_y);
        }
        drop.m_count = taken;

    }
    GetDatabase()->GetPlayerTable()->Save(pAvatar);

    BroadcastDropPickedUp(pWorld, pAvatar->GetNetId(), uid);

    auto* pDropInfo = GetItemManager()->GetItem(drop.m_itemId);
    std::string dropName = pDropInfo ? pDropInfo->m_name : "Item";
    if (drop.m_itemId == ITEM_GEMS)
        VarList::OnConsoleMessage(pAvatar->Get(), fmt::format("Collected  `w{} {}``.", drop.m_count, dropName));
    else
        VarList::OnConsoleMessage(pAvatar->Get(), fmt::format("Collected  `w{} {}``. Rarity: `w{}``",
            drop.m_count, dropName, pDropInfo ? pDropInfo->m_rarity : 0));

    Logger::Print(INFO, "Player {} picked up drop uid={} item={} count={}", pAvatar->GetRawName(), uid, drop.m_itemId, drop.m_count);
}

static void EquipClothingItem(Player* pAvatar, uint16_t itemId) {
    auto* pItem = GetItemManager()->GetItem(itemId);
    if (!pItem || !pItem->IsClothing()) {
        Logger::Print(WARNING, "EquipClothingItem: item {} requested by {} isn't a clothing item", itemId, pAvatar->GetRawName());
        return;
    }

    auto* pItems = pAvatar->GetItems();
    if (pItems->m_bpItems.find(itemId) == pItems->m_bpItems.end()) {
        Logger::Print(WARNING, "EquipClothingItem: {} doesn't own item {}", pAvatar->GetRawName(), itemId);
        return;
    }

    auto slot = static_cast<eClothTypes>(pItem->m_clothingType);
    uint16_t& currentInSlot = pItems->GetCloth(slot);

    currentInSlot = (currentInSlot == itemId) ? ITEM_BLANK : itemId;

    GetDatabase()->GetPlayerTable()->Save(pAvatar);

    CAction::PlaySFX(pAvatar->Get(), "change_clothes", 0);
    VarList::OnSetClothing(pAvatar->Get(), pAvatar->GetNetId(), pItems->GetClothes(), static_cast<int32_t>(pItems->GetSkinColor().GetInt()));

    uint32_t effectFlags = ItemEffects::ComputeCharacterStateFlags(pAvatar);
    for (int i = 0; i < 2; i++) {
        TankPacketData t{};
        t.m_type = NET_GAME_PACKET_SET_CHARACTER_STATE;
        t.m_netId = 0;
        t.m_jumpCount = 128;

        t.m_punchIndex = ItemEffects::GetActivePunchEffect(pAvatar->GetItems());
        t.m_animationType = 128;
        t.m_floatVariable = 150.0f;
        t.m_vectorX = 1000.0f;
        t.m_vectorY = 350.0f;
        t.m_vectorX2 = 200.0f;
        t.m_vectorY2 = 1000.0f;
        t.m_effectFlags = static_cast<int32_t>(effectFlags);
        STankPacket packet(t);
        ENetWrapper::SendPacket(pAvatar->Get(), packet);
    }
    Logger::Print(INFO, "Player {} {} clothing slot {} with item {}", pAvatar->GetRawName(),
        currentInSlot == ITEM_BLANK ? "cleared" : "set", static_cast<int>(slot), itemId);
}

static void HandleModifyItemInventory(Player* pAvatar, TankPacketData* pTankData) {
    uint16_t itemId = static_cast<uint16_t>(static_cast<uint32_t>(pTankData->m_mainData) >> 8);
    EquipClothingItem(pAvatar, itemId);
}

static void HandleItemActivateRequest(Player* pAvatar, TankPacketData* pTankData) {
    uint16_t itemId = static_cast<uint16_t>(pTankData->m_mainData);

    auto* pActivatedItem = GetItemManager()->GetItem(itemId);
    if (pActivatedItem && pActivatedItem->m_name == "World Lock") {
        constexpr uint8_t kRequired = 100;
        auto* pItems = pAvatar->GetItems();
        auto it = pItems->m_bpItems.find(itemId);
        if (it == pItems->m_bpItems.end() || it->second < kRequired) {
            VarList::OnConsoleMessage(pAvatar->Get(), fmt::format("You need {} World Locks to make a Diamond Lock.", kRequired));
            return;
        }
        auto* pDiamondLock = GetItemManager()->GetItemByName("Diamond Lock");
        if (!pDiamondLock)
            return;
        pItems->RemoveItem(itemId, kRequired);
        pItems->AddItem(static_cast<uint16_t>(pDiamondLock->m_Id), 1);
        GetDatabase()->GetPlayerTable()->Save(pAvatar);
        SendInventoryState(pAvatar);

        VarList::OnPlayPositioned(pAvatar->Get(), pAvatar->GetNetId(), "audio/success.wav");
        std::string message = "You compressed 100 `2World Lock`` into a `2Diamond Lock``!";
        VarList::OnConsoleMessage(pAvatar->Get(), message);
        VarList::OnTalkBubble(pAvatar->Get(), pAvatar->GetNetId(), message);
        Logger::Print(INFO, "Player {} compressed 100 World Locks into a Diamond Lock", pAvatar->GetRawName());
        return;
    }
    if (pActivatedItem && pActivatedItem->m_name == "Diamond Lock") {
        constexpr uint8_t kGranted = 100;
        auto* pItems = pAvatar->GetItems();
        auto it = pItems->m_bpItems.find(itemId);
        if (it == pItems->m_bpItems.end() || it->second < 1)
            return;
        auto* pWorldLock = GetItemManager()->GetItemByName("World Lock");
        if (!pWorldLock)
            return;
        pItems->RemoveItem(itemId, 1);
        pItems->AddItem(static_cast<uint16_t>(pWorldLock->m_Id), kGranted);
        GetDatabase()->GetPlayerTable()->Save(pAvatar);
        SendInventoryState(pAvatar);

        VarList::OnPlayPositioned(pAvatar->Get(), pAvatar->GetNetId(), "audio/success.wav");

        std::string message = "You shattered a `2Diamond Lock`` into 100 `2World Lock``!";
        VarList::OnConsoleMessage(pAvatar->Get(), message);
        VarList::OnTalkBubble(pAvatar->Get(), pAvatar->GetNetId(), message);
        Logger::Print(INFO, "Player {} shattered a Diamond Lock into 100 World Locks", pAvatar->GetRawName());
        return;
    }

    if (pActivatedItem && pActivatedItem->m_itemType == ITEMTYPE_CONSUMABLE) {

        VarList::OnTalkBubble(pAvatar->Get(), pAvatar->GetNetId(), "Must be used on a person.");
        return;
    }

    EquipClothingItem(pAvatar, itemId);
}

static void HandleState(Player* pAvatar, std::shared_ptr<Server> pServer, TankPacketData* pTankData) {

    pAvatar->SetPosition(pTankData->m_vectorX, pTankData->m_vectorY);
    pAvatar->SetFacingLeft(pTankData->m_flags.bFacingLeft);

    if (pTankData->m_flags.bLavaHit && pAvatar->ApplyDamage(Player::kLavaDamage)) {
        Logger::Print(INFO, "Player {} burned to death in lava", pAvatar->GetRawName());
        if (auto* pRespawn = GetEventPool()->ActionManager::GetEventIfExists("respawn")) {
            TextParse respawnParser(std::string("action|respawn\n"));
            pRespawn->sig_function(pAvatar, pServer, std::string(), respawnParser, nullptr);
        }
        return;
    }

    TankPacketData update = *pTankData;
    update.m_netId = pAvatar->GetNetId();

    update.m_flags.bExtended = false;
    update.m_dataLength = 0;

    STankPacket packet(update);
    pAvatar->GetWorld()->BroadcastPacket(packet, pAvatar);
}

static void HandleSetIconState(Player* pAvatar, TankPacketData* pTankData) {
    TankPacketData update = *pTankData;
    update.m_netId = pAvatar->GetNetId();
    update.m_flags.bExtended = false;
    update.m_dataLength = 0;

    STankPacket packet(update);
    pAvatar->GetWorld()->BroadcastPacket(packet, pAvatar);
}

static void HandleTileActivateRequest(Player* pAvatar, std::shared_ptr<Server> pServer, TankPacketData* pTankData) {
    auto pWorld = pAvatar->GetWorld();
    auto* pTile = pWorld->GetTile(pTankData->m_tilePositionX, pTankData->m_tilePositionY);
    if (!pTile || pTile->IsForegroundEmpty())
        return;

    auto* pItem = GetItemManager()->GetItem(pTile->m_foregroundId);
    if (!pItem)
        return;

    if (pItem->m_itemType == ITEMTYPE_LAB) {
        uint32_t tileX = pTankData->m_tilePositionX;
        uint32_t tileY = pTankData->m_tilePositionY;

        std::vector<DroppedItem> piles;
        for (const auto& drop : pWorld->GetDroppedItems()) {
            if (static_cast<uint32_t>(drop.m_x / 32.0f) == tileX &&
                static_cast<uint32_t>(drop.m_y / 32.0f) == tileY)
                piles.push_back(drop);
        }

        if (piles.size() < 3) {
            VarList::OnTalkBubble(pAvatar->Get(), pAvatar->GetNetId(), "I need 3 ingredients!");
            return;
        }
        if (piles.size() > 3) {
            VarList::OnTalkBubble(pAvatar->Get(), pAvatar->GetNetId(), "It won't mix more than 3 items at once!");
            return;
        }

        for (const auto& recipe : CombineRecipes::GetTable()) {

            std::array<const DroppedItem*, 3> matched{ nullptr, nullptr, nullptr };
            bool ok = true;
            for (std::size_t i = 0; i < 3 && ok; i++) {
                ok = false;
                for (const auto& pile : piles) {
                    if (pile.m_itemId != recipe.m_inputs[i].m_itemId)
                        continue;
                    matched[i] = &pile;
                    ok = true;
                    break;
                }
            }
            if (!ok)
                continue;

            uint32_t batches = 0xFFFFFFFFu;
            for (std::size_t i = 0; i < 3; i++)
                batches = std::min<uint32_t>(batches, matched[i]->m_count / recipe.m_inputs[i].m_count);
            if (batches == 0)
                continue;
            batches = std::min<uint32_t>(batches, 200u / recipe.m_resultCount);
            if (batches == 0)
                continue;

            for (std::size_t i = 0; i < 3; i++) {
                DroppedItem removed{};
                if (!pWorld->RemoveDrop(matched[i]->m_uid, removed))
                    continue;
                auto spent = static_cast<uint8_t>(recipe.m_inputs[i].m_count * batches);
                if (removed.m_count > spent)
                    pWorld->SpawnDrop(removed.m_itemId, static_cast<uint8_t>(removed.m_count - spent), removed.m_x, removed.m_y);
            }

            for (uint32_t b = 0; b < batches; b++) {
                uint16_t result = recipe.m_results[std::rand() % recipe.m_results.size()];
                SpawnDropJittered(pWorld, result, recipe.m_resultCount, tileX * 32.0f, tileY * 32.0f);
            }

            BroadcastParticleEffect(pWorld, 44, tileX * 32.0f + 16.0f, tileY * 32.0f + 16.0f, 0.0f, 0.0f);
            for (auto* pOther : pWorld->GetPlayers())
                CAction::PlaySFX(pOther->Get(), "terraform", 0);
            VarList::OnTalkBubble(pAvatar->Get(), pAvatar->GetNetId(), "SCIENCE!");
            GetDatabase()->GetWorldTable()->Save(*pWorld);
            Logger::Print(INFO, "Player {} combined {} batch(es) in a {} at ({},{})",
                pAvatar->GetRawName(), batches, pItem->m_name, tileX, tileY);
            return;
        }

        VarList::OnTalkBubble(pAvatar->Get(), pAvatar->GetNetId(), "Those items don't seem to combine!");
        return;
    }

    if (pItem->m_itemType == ITEMTYPE_SPOTLIGHT) {
        uint32_t tileX = pTankData->m_tilePositionX;
        uint32_t tileY = pTankData->m_tilePositionY;

        if (!pTile->m_spotlightName.empty()) {
            bool stillHere = false;
            for (auto* pOther : pWorld->GetPlayers())
                if (pOther->GetRawName() == pTile->m_spotlightName)
                    stillHere = true;
            if (!stillHere)
                pTile->m_spotlightName.clear();
        }

        VarList::OnDialogRequest(pAvatar->Get(), fmt::format(
            "set_default_color|`o\nembed_data|tilex|{}\nembed_data|tiley|{}\n"
            "add_label_with_icon|big|`wShine the Spotlight!``|left|2646|\nadd_spacer|small|\n"
            "add_textbox|{}|left|\nadd_spacer|small|\n"
            "add_player_picker|ID|`wChoose a {}``|{}\n"
            "end_dialog|2646|Nevermind||",
            tileX, tileY,
            pTile->m_spotlightName.empty()
                ? std::string("The light is currently off.")
                : fmt::format("The light is shining on {}.", pTile->m_spotlightName),
            pTile->m_spotlightName.empty() ? "superstar" : "new star",
            pTile->m_spotlightName.empty() ? "" : "\nadd_button|off|Turn it off|noflags|0|0|"));
        return;
    }

    if (pItem->m_itemType == ITEMTYPE_CHECKPOINT) {
        pAvatar->SetCheckpoint(pTankData->m_tilePositionX, pTankData->m_tilePositionY);
        VarList::OnPlayPositioned(pAvatar->Get(), pAvatar->GetNetId(), "audio/bubble.wav");
        Logger::Print(INFO, "Player {} set checkpoint at ({},{})", pAvatar->GetRawName(), pTankData->m_tilePositionX, pTankData->m_tilePositionY);
        return;
    }

    if (!pItem->IsDoor())
        return;

    std::string target = pTile->m_hasDoorExtra ? pTile->m_doorExtra.m_target : std::string();
    if (!target.empty() && target != pWorld->GetName()) {
        Logger::Print(INFO, "Player {} took door at ({},{}) from '{}' to '{}'",
            pAvatar->GetRawName(), pTankData->m_tilePositionX, pTankData->m_tilePositionY, pWorld->GetName(), target);

        pWorld->BroadcastPlayerLeft(pAvatar);
        pWorld->ReleaseNetId(pAvatar->GetNetId());
        pWorld->RemovePlayer(pAvatar);
        pAvatar->SetWorld(nullptr);
        pAvatar->SetNetId(-1);
        pAvatar->GetDetail().RemoveFlag(CLIENTFLAG_IS_IN);

        if (auto* pJoinEvent = GetEventPool()->ActionManager::GetEventIfExists("join_request")) {
            TextParse joinParser(fmt::format("action|join_request\nname|{}\n", target));
            pJoinEvent->sig_function(pAvatar, pServer, std::string(), joinParser, nullptr);
        }
        return;
    }

    Logger::Print(INFO, "Player {} activated door at ({},{}), leaving world '{}'",
        pAvatar->GetRawName(), pTankData->m_tilePositionX, pTankData->m_tilePositionY, pWorld->GetName());

    pWorld->BroadcastPlayerLeft(pAvatar);
    pWorld->ReleaseNetId(pAvatar->GetNetId());
    pWorld->RemovePlayer(pAvatar);
    pAvatar->SetWorld(nullptr);
    pAvatar->SetNetId(-1);
    pAvatar->GetDetail().RemoveFlag(CLIENTFLAG_IS_IN);

    std::string menu = WorldMenu::Build(pAvatar, pServer->GetWorldPool(), pWorld->GetName());
    VarList::OnRequestWorldSelectMenu(pAvatar->Get(), menu);
}

void HandleTankPacket(Player* pAvatar, std::shared_ptr<Server> pServer, TankPacketData* pTankData) {
    if (!pAvatar->GetDetail().IsFlagOn(CLIENTFLAG_IS_IN) || !pAvatar->GetWorld())
        return;

    switch (pTankData->m_type) {
    case NET_GAME_PACKET_STATE:
        HandleState(pAvatar, pServer, pTankData);
        break;
    case NET_GAME_PACKET_TILE_CHANGE_REQUEST:
        HandleTileChangeRequest(pAvatar, pServer, pTankData);
        break;
    case NET_GAME_PACKET_TILE_ACTIVATE_REQUEST:
        HandleTileActivateRequest(pAvatar, pServer, pTankData);
        break;
    case NET_GAME_PACKET_ITEM_ACTIVATE_OBJECT_REQUEST:
        HandleItemActivateObjectRequest(pAvatar, pTankData);
        break;
    case NET_GAME_PACKET_MODIFY_ITEM_INVENTORY:
        HandleModifyItemInventory(pAvatar, pTankData);
        break;
    case NET_GAME_PACKET_ITEM_ACTIVATE_REQUEST:
        HandleItemActivateRequest(pAvatar, pTankData);
        break;
    case NET_GAME_PACKET_PING_REPLY:
        break;
    case NET_GAME_PACKET_SET_ICON_STATE:
        HandleSetIconState(pAvatar, pTankData);
        break;
    default:
        Logger::Print(WARNING, "Unhandled tank packet type {}", static_cast<int>(pTankData->m_type));
        break;
    }
}
