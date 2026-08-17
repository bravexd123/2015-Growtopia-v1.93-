#include <World/World.hpp>
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <queue>
#include <unordered_set>
#include <Player/Player.hpp>
#include <ENetWrapper/ENetWrapper.hpp>
#include <Packet/PacketFactory.hpp>
#include <Packet/VariantFunction.hpp>
#include <Utils/BinaryWriter.hpp>
#include <Manager/Item/ItemComponent.hpp>
#include <Manager/Item/ItemManager.hpp>
#include <Manager/Item/ItemInfo.hpp>
#include <Manager/Trade/TradeManager.hpp>
#include <Logger/Logger.hpp>

void World::Init(const std::string& name, uint32_t width, uint32_t height) {
    m_name = name;
    m_width = width;
    m_height = height;
    this->Generate();
}

bool World::LoadSnapshot(const std::string& name, uint32_t width, uint32_t height, std::vector<Tile> tiles) {
    if (tiles.size() != static_cast<size_t>(width) * height)
        return false;
    m_name = name;
    m_width = width;
    m_height = height;
    m_tiles = std::move(tiles);
    return true;
}

void World::GenerateEmpty() {
    m_tiles.assign(static_cast<size_t>(m_width) * m_height, Tile{});

    uint32_t bedrockRows = std::min<uint32_t>(6, m_height);
    uint32_t bedrockStart = m_height - bedrockRows;
    for (uint32_t y = bedrockStart; y < m_height; y++)
        for (uint32_t x = 0; x < m_width; x++)
            m_tiles[static_cast<size_t>(y) * m_width + x].m_foregroundId = ITEM_BEDROCK;

    if (bedrockStart > 0 && m_width > 0) {
        uint32_t doorX = static_cast<uint32_t>(std::rand()) % m_width;
        uint32_t doorY = bedrockStart - 1;
        Tile& door = m_tiles[static_cast<size_t>(doorY) * m_width + doorX];
        door.m_foregroundId = ITEM_MAIN_DOOR;
        door.m_hasDoorExtra = true;
        door.m_doorExtra.m_label = "EXIT";
    }
}

void World::Generate() {
    m_tiles.assign(static_cast<size_t>(m_width) * m_height, Tile{});

    uint32_t surface = m_height * 4 / 10;

    uint32_t bedrockRows = std::min<uint32_t>(6, m_height);
    uint32_t bedrockStart = m_height - bedrockRows;

    uint32_t lavaZoneStart = (bedrockStart >= 4) ? bedrockStart - 4 : surface;
    for (uint32_t y = 0; y < m_height; y++) {
        for (uint32_t x = 0; x < m_width; x++) {
            Tile& tile = m_tiles[static_cast<size_t>(y) * m_width + x];
            if (y < surface)
                continue;

            tile.m_backgroundId = ITEM_CAVE_BACKGROUND;
            if (y >= bedrockStart) {
                tile.m_foregroundId = ITEM_BEDROCK;
            } else if (y >= lavaZoneStart && (std::rand() % 100) < 20) {
                tile.m_foregroundId = ITEM_LAVA;
            } else if ((std::rand() % 100) < 4) {
                tile.m_foregroundId = ITEM_ROCK;
            } else {
                tile.m_foregroundId = ITEM_DIRT;
            }
        }
    }

    if (surface > 0 && m_width > 0) {
        uint32_t doorX = static_cast<uint32_t>(std::rand()) % m_width;
        uint32_t doorY = surface - 1;
        Tile& door = m_tiles[static_cast<size_t>(doorY) * m_width + doorX];
        door.m_foregroundId = ITEM_MAIN_DOOR;
        door.m_backgroundId = ITEM_BLANK;
        door.m_hasDoorExtra = true;
        door.m_doorExtra.m_label = "EXIT";

        if (surface < m_height) {
            Tile& groundUnderDoor = m_tiles[static_cast<size_t>(surface) * m_width + doorX];
            groundUnderDoor.m_foregroundId = ITEM_BEDROCK;
            groundUnderDoor.m_backgroundId = ITEM_CAVE_BACKGROUND;
        }
    }
}

const std::string& World::GetName() const {
    return m_name;
}
uint32_t World::GetWidth() const {
    return m_width;
}
uint32_t World::GetHeight() const {
    return m_height;
}

bool World::IsValidPosition(uint32_t tileX, uint32_t tileY) const {
    return tileX < m_width && tileY < m_height;
}
const std::vector<Tile>& World::GetTiles() const {
    return m_tiles;
}
Tile* World::GetTile(uint32_t tileX, uint32_t tileY) {
    if (!this->IsValidPosition(tileX, tileY))
        return nullptr;
    return &m_tiles[static_cast<size_t>(tileY) * m_width + tileX];
}
bool World::FindDoorTile(uint32_t& outX, uint32_t& outY) const {
    for (uint32_t y = 0; y < m_height; y++) {
        for (uint32_t x = 0; x < m_width; x++) {
            if (m_tiles[static_cast<size_t>(y) * m_width + x].m_hasDoorExtra) {
                outX = x;
                outY = y;
                return true;
            }
        }
    }
    return false;
}
uint32_t World::AgeWorld(int64_t seconds) {
    uint32_t affected = 0;
    for (size_t index = 0; index < m_tiles.size(); index++) {
        Tile& tile = m_tiles[index];
        if (tile.IsForegroundEmpty() || tile.m_plantedAtUnix == 0)
            continue;
        tile.m_plantedAtUnix -= seconds;
        affected++;

        uint32_t tileX = static_cast<uint32_t>(index % m_width);
        uint32_t tileY = static_cast<uint32_t>(index / m_width);
        int64_t elapsed = static_cast<int64_t>(std::time(nullptr)) - tile.m_plantedAtUnix;

        TankPacketData tileUpdate{};
        tileUpdate.m_type = NET_GAME_PACKET_SEND_TILE_UPDATE_DATA;
        tileUpdate.m_netId = -1;
        tileUpdate.m_tilePositionX = tileX;
        tileUpdate.m_tilePositionY = tileY;

        BinaryWriter tw(14);
        tw.Write<uint16_t>(tile.m_foregroundId);
        tw.Write<uint16_t>(0);
        tw.Write<uint16_t>(0);
        tw.Write<uint16_t>(0x11);
        tw.Write<uint8_t>(4);
        tw.Write<uint32_t>(static_cast<uint32_t>(elapsed < 0 ? 0 : elapsed));
        tw.Write<uint8_t>(tile.m_fruitCount);
        std::vector<uint8_t> tileData(tw.Get(), tw.Get() + tw.GetPosition());

        SExtendedTankPacket tileUpdatePacket(tileUpdate, tileData);
        this->BroadcastPacket(tileUpdatePacket);
    }
    return affected;
}
bool World::SetTile(uint32_t tileX, uint32_t tileY, const Tile& tile) {
    if (!this->IsValidPosition(tileX, tileY))
        return false;
    m_tiles[static_cast<size_t>(tileY) * m_width + tileX] = tile;
    return true;
}

void World::AddPlayer(Player* pPlayer) {
    if (!pPlayer)
        return;
    if (std::find(m_players.begin(), m_players.end(), pPlayer) != m_players.end())
        return;

    const std::string& newName = pPlayer->GetRawName();
    if (!newName.empty()) {
        m_players.erase(std::remove_if(m_players.begin(), m_players.end(),
            [&](Player* p) { return p != pPlayer && p->GetRawName() == newName; }), m_players.end());
    }

    m_players.push_back(pPlayer);
}
void World::RemovePlayer(Player* pPlayer) {
    m_players.erase(std::remove(m_players.begin(), m_players.end(), pPlayer), m_players.end());
}
std::vector<Player*> World::GetPlayers() const {
    return m_players;
}
size_t World::GetPlayerCount() const {
    return m_players.size();
}

int32_t World::AssignNetId(Player* pPlayer) {
    return m_nextNetId++;
}
void World::ReleaseNetId(int32_t netId) {

}

DroppedItem* World::FindMergeableDrop(uint16_t itemId, uint8_t count, float x, float y) {

    if (itemId == ITEM_GEMS)
        return nullptr;

    constexpr uint8_t kMaxGroundStack = 200;
    uint32_t tileX = static_cast<uint32_t>(x / 32.0f);
    uint32_t tileY = static_cast<uint32_t>(y / 32.0f);
    for (auto& drop : m_droppedItems) {
        if (drop.m_itemId != itemId)
            continue;
        if (static_cast<uint32_t>(drop.m_x / 32.0f) != tileX || static_cast<uint32_t>(drop.m_y / 32.0f) != tileY)
            continue;
        if (drop.m_count + count > kMaxGroundStack)
            continue;
        return &drop;
    }
    return nullptr;
}

DroppedItem* World::SpawnDrop(uint16_t itemId, uint8_t count, float x, float y) {
    DroppedItem drop;
    drop.m_uid = m_nextDropUid++;
    drop.m_itemId = itemId;
    drop.m_count = count;
    drop.m_x = x;
    drop.m_y = y;
    m_droppedItems.push_back(drop);
    return &m_droppedItems.back();
}
bool World::RemoveDrop(int32_t uid, DroppedItem& out) {
    auto it = std::find_if(m_droppedItems.begin(), m_droppedItems.end(),
        [uid](const DroppedItem& d) { return d.m_uid == uid; });
    if (it == m_droppedItems.end())
        return false;
    out = *it;
    m_droppedItems.erase(it);
    return true;
}
const std::vector<DroppedItem>& World::GetDroppedItems() const {
    return m_droppedItems;
}
void World::RenumberDroppedItemsForJoin() {
    int32_t nextUid = 1;
    for (auto& drop : m_droppedItems)
        drop.m_uid = nextUid++;
    m_nextDropUid = nextUid;
}

void World::BroadcastPacket(ISPacket& packet, Player* pExclude) {
    for (auto* pPlayer : m_players) {
        if (pPlayer == pExclude)
            continue;
        ENetWrapper::SendPacket(pPlayer->Get(), packet);
    }
}

std::vector<uint8_t> World::SerializeMapData() const {

    uint32_t square = m_width * m_height;
    const std::size_t trailer = 4 + 4 + 2 + 2 + 4 + 4;

    auto findLockAtIndex = [&](std::size_t index) -> const LockInfo* {
        if (auto it = m_anchorToLockIndex.find(static_cast<uint32_t>(index)); it != m_anchorToLockIndex.end())
            return &m_tileLocks[it->second];
        if (m_hasWorldLock) {
            uint32_t wlIndex = m_worldLock.m_anchorY * m_width + m_worldLock.m_anchorX;
            if (wlIndex == index)
                return &m_worldLock;
        }
        return nullptr;
    };
    auto isLockTile = [&](const Tile& tile) {
        if (tile.m_foregroundId == ITEM_BLANK)
            return false;
        auto* pItem = GetItemManager()->GetItem(tile.m_foregroundId);
        return pItem && pItem->IsLock();
    };

    std::size_t tilesSize = 0;
    for (std::size_t i = 0; i < m_tiles.size(); i++) {
        const auto& tile = m_tiles[i];
        tilesSize += 8;
        if (tile.m_hasDoorExtra)
            tilesSize += 1 + 2 + tile.m_doorExtra.m_label.size() + 1;
        else if (tile.m_hasSignExtra)
            tilesSize += 1 + 2 + tile.m_signExtra.m_text.size() + 4;
        else if (tile.m_plantedAtUnix != 0)
            tilesSize += 1 + 4 + 1;
        else if (isLockTile(tile)) {
            const LockInfo* pLock = findLockAtIndex(i);
            std::size_t accessCount = pLock ? pLock->m_accessUserIds.size() : 0;
            tilesSize += 1 + 1 + 4 + 4 + 4 * accessCount;
        }
    }
    std::size_t bufSize = 6 + 2 + m_name.size() + 4 + 4 + 4 + tilesSize + trailer;
    BinaryWriter bw(bufSize);

    bw.Write<uint16_t>(3);
    bw.Write<uint32_t>(0);
    bw.Write(m_name, sizeof(uint16_t));
    bw.Write<uint32_t>(m_width);
    bw.Write<uint32_t>(m_height);
    bw.Write<uint32_t>(square);
    for (std::size_t i = 0; i < m_tiles.size(); i++) {
        const auto& tile = m_tiles[i];

        bool hasSeedExtra = !tile.m_hasDoorExtra && !tile.m_hasSignExtra && tile.m_plantedAtUnix != 0;
        bool hasLockExtra = !tile.m_hasDoorExtra && !tile.m_hasSignExtra && !hasSeedExtra && isLockTile(tile);
        const LockInfo* pLockExtra = hasLockExtra ? findLockAtIndex(i) : nullptr;
        bw.Write<uint16_t>(tile.m_foregroundId);
        bw.Write<uint16_t>(tile.m_backgroundId);
        bw.Write<uint16_t>(0);

        uint16_t tileFlags = static_cast<uint16_t>(tile.m_flags & kTilePersistedFlags);
        if (tile.m_hasDoorExtra) tileFlags |= TILEFLAG_EXTRA;
        else if (tile.m_hasSignExtra) tileFlags |= TILEFLAG_EXTRA;
        else if (hasSeedExtra) tileFlags |= TILEFLAG_EXTRA | TILEFLAG_LOCKED;
        else if (hasLockExtra) tileFlags |= TILEFLAG_EXTRA;
        bw.Write<uint16_t>(tileFlags);
        if (tile.m_hasDoorExtra) {
            bw.Write<uint8_t>(1);
            bw.Write(tile.m_doorExtra.m_label, sizeof(uint16_t));
            bw.Write<uint8_t>(0);
        } else if (tile.m_hasSignExtra) {

            bw.Write<uint8_t>(2);
            bw.Write(tile.m_signExtra.m_text, sizeof(uint16_t));
            bw.Write<int32_t>(-1);
        } else if (hasSeedExtra) {

            uint32_t elapsed = static_cast<uint32_t>(std::time(nullptr) - tile.m_plantedAtUnix);
            bw.Write<uint8_t>(4);
            bw.Write<uint32_t>(elapsed);
            bw.Write<uint8_t>(tile.m_fruitCount);
        } else if (hasLockExtra) {

            uint8_t settings = 0;
            uint32_t owner = 0;
            uint32_t accessCount = 0;
            if (pLockExtra) {
                if (pLockExtra->m_isPublic) settings |= 0x1;
                if (pLockExtra->m_ignoreAir) settings |= 0x2;
                owner = pLockExtra->m_ownerId;
                accessCount = static_cast<uint32_t>(pLockExtra->m_accessUserIds.size());
            }
            bw.Write<uint8_t>(3);
            bw.Write<uint8_t>(settings);
            bw.Write<uint32_t>(owner);
            bw.Write<uint32_t>(accessCount);
            if (pLockExtra) {
                for (uint32_t accessUserId : pLockExtra->m_accessUserIds)
                    bw.Write<uint32_t>(accessUserId);
            }
        }
    }
    bw.Write<uint32_t>(0);
    bw.Write<uint32_t>(0);
    bw.Write<uint16_t>(4);
    bw.Write<uint16_t>(4);
    bw.Write<uint32_t>(0);
    bw.Write<uint32_t>(0);

    if (bw.GetPosition() != bufSize) {
        Logger::Print(WARNING, "SerializeMapData: SIZE MISMATCH! computed bufSize={} but actually wrote {} bytes - buffer {}",
            bufSize, bw.GetPosition(), bw.GetPosition() > bufSize ? "OVERFLOWED (heap corruption!)" : "underflowed");
    }

    return std::vector<uint8_t>(bw.Get(), bw.Get() + bw.GetPosition());
}

void World::BroadcastPlayerLeft(Player* pLeaving) {
    if (!pLeaving)
        return;

    if (pLeaving->GetTradingWithNetId() != -1)
        TradeManager::CancelTrade(pLeaving);

    int32_t leavingNetId = pLeaving->GetNetId();
    std::size_t remaining = m_players.size() > 0 ? m_players.size() - 1 : 0;
    std::string formattedName = pLeaving->GetFormattedName();

    auto bubbleList = VariantList::Create("OnTalkBubble");
    bubbleList.Insert(leavingNetId);
    bubbleList.Insert(fmt::format("`w`5<`w{}`` `5left, `w{}`` `5others here>```w", formattedName, remaining));
    SVariantPacket bubblePacket(bubbleList);
    BroadcastPacket(bubblePacket, pLeaving);

    auto leftList = VariantList::Create("OnConsoleMessage");
    leftList.Insert(fmt::format("`w`5<`w{}`` `5left, `w{} `5others here>``", formattedName, remaining));
    SVariantPacket leftPacket(leftList);
    BroadcastPacket(leftPacket, pLeaving);

    for (auto* pOther : m_players) {
        if (pOther == pLeaving)
            continue;
        VarList::OnRemove(pOther->Get(), leavingNetId);
    }
}

void World::BroadcastTileUpdate(uint32_t tileX, uint32_t tileY) {
    Tile* pTile = GetTile(tileX, tileY);
    if (!pTile)
        return;

    BinaryWriter bw(64 + pTile->m_doorExtra.m_label.size() + pTile->m_signExtra.m_text.size());
    bw.Write<uint16_t>(pTile->m_foregroundId);
    bw.Write<uint16_t>(pTile->m_backgroundId);
    bw.Write<uint16_t>(0);

    {
        uint16_t flags = static_cast<uint16_t>(pTile->m_flags & kTilePersistedFlags);
        if (pTile->m_hasDoorExtra || pTile->m_hasSignExtra)
            flags |= TILEFLAG_EXTRA;
        bw.Write<uint16_t>(flags);
    }
    if (pTile->m_hasDoorExtra) {
        bw.Write<uint8_t>(1);
        bw.Write(pTile->m_doorExtra.m_label, sizeof(uint16_t));
        bw.Write<uint8_t>(0);
    } else if (pTile->m_hasSignExtra) {
        bw.Write<uint8_t>(2);
        bw.Write(pTile->m_signExtra.m_text, sizeof(uint16_t));
        bw.Write<int32_t>(-1);
    }

    TankPacketData t{};
    t.m_type = NET_GAME_PACKET_SEND_TILE_UPDATE_DATA;
    t.m_netId = -1;
    t.m_tilePositionX = tileX;
    t.m_tilePositionY = tileY;

    std::vector<uint8_t> tileData(bw.Get(), bw.Get() + bw.GetPosition());
    SExtendedTankPacket packet(t, tileData);
    BroadcastPacket(packet);
}

namespace {

    std::vector<uint32_t> ComputeSmallLockArea(uint32_t anchorX, uint32_t anchorY, uint32_t width, uint32_t height, uint32_t capacity) {
        std::vector<uint32_t> result;
        if (capacity == 0)
            return result;

        int64_t ax = static_cast<int64_t>(anchorX);
        int64_t ay = static_cast<int64_t>(anchorY);

        int32_t radius = 1;
        while (static_cast<uint32_t>((2 * radius + 1) * (2 * radius + 1)) < capacity * 4 + 16)
            radius++;

        struct Candidate {
            int64_t m_dist2;
            int32_t m_dy;
            int32_t m_dx;
        };
        std::vector<Candidate> candidates;
        candidates.reserve(static_cast<size_t>(2 * radius + 1) * (2 * radius + 1));
        for (int32_t dy = -radius; dy <= radius; dy++) {
            for (int32_t dx = -radius; dx <= radius; dx++) {
                if (dx == 0 && dy == 0)
                    continue;
                candidates.push_back({ static_cast<int64_t>(dx) * dx + static_cast<int64_t>(dy) * dy, dy, dx });
            }
        }
        std::sort(candidates.begin(), candidates.end(), [](const Candidate& a, const Candidate& b) {
            if (a.m_dist2 != b.m_dist2) return a.m_dist2 < b.m_dist2;
            if (a.m_dy != b.m_dy) return a.m_dy < b.m_dy;
            return a.m_dx < b.m_dx;
        });

        for (const auto& candidate : candidates) {
            if (result.size() >= capacity)
                break;
            int64_t x = ax + candidate.m_dx;
            int64_t y = ay + candidate.m_dy;
            if (x < 0 || x >= static_cast<int64_t>(width) || y < 0 || y >= static_cast<int64_t>(height))
                continue;
            result.push_back(static_cast<uint32_t>(y) * width + static_cast<uint32_t>(x));
        }
        return result;
    }

    std::vector<uint32_t> ComputeCircularArea(uint32_t anchorX, uint32_t anchorY, uint32_t width, uint32_t height, uint32_t capacity) {
        std::vector<uint32_t> result;

        int32_t radius = 1;
        while (static_cast<uint32_t>((2 * radius + 1) * (2 * radius + 1)) < capacity + 8)
            radius++;

        std::vector<std::tuple<int64_t, int32_t, int32_t>> magnitudes;
        for (int32_t adx = 0; adx <= radius; adx++) {
            for (int32_t ady = 0; ady <= radius; ady++)
                magnitudes.emplace_back(static_cast<int64_t>(adx) * adx + static_cast<int64_t>(ady) * ady, adx, ady);
        }
        std::stable_sort(magnitudes.begin(), magnitudes.end(), [](const auto& a, const auto& b) { return std::get<0>(a) < std::get<0>(b); });

        auto tryAdd = [&](int64_t dx, int64_t dy) {
            if (result.size() >= capacity)
                return;
            int64_t x = static_cast<int64_t>(anchorX) + dx;
            int64_t y = static_cast<int64_t>(anchorY) + dy;
            if (x < 0 || x >= static_cast<int64_t>(width) || y < 0 || y >= static_cast<int64_t>(height))
                return;
            result.push_back(static_cast<uint32_t>(y) * width + static_cast<uint32_t>(x));
        };

        for (const auto& [dist2, adx, ady] : magnitudes) {
            int32_t groupSize = (adx == 0 && ady == 0) ? 1 : (adx == 0 || ady == 0) ? 2 : 4;
            if (result.size() + static_cast<size_t>(groupSize) > capacity)
                break;

            tryAdd(adx, ady);
            if (adx != 0)
                tryAdd(-adx, ady);
            if (ady != 0)
                tryAdd(adx, -ady);
            if (adx != 0 && ady != 0)
                tryAdd(-adx, -ady);
        }
        return result;
    }
}

std::vector<uint32_t> World::ComputeLockArea(uint32_t anchorX, uint32_t anchorY, uint32_t capacity) const {
    if (capacity == 0)
        return {};

    if (capacity == 10)
        return ComputeSmallLockArea(anchorX, anchorY, m_width, m_height, capacity);
    return ComputeCircularArea(anchorX, anchorY, m_width, m_height, capacity);
}

std::vector<uint32_t> World::ComputeFloodFillArea(uint32_t anchorX, uint32_t anchorY, uint32_t capacity) const {
    std::vector<uint32_t> result;
    if (capacity == 0)
        return result;

    bool allowDiagonals = capacity != 10;

    std::unordered_set<uint32_t> visited;
    std::queue<std::pair<uint32_t, uint32_t>> pending;

    auto tryEnqueue = [&](int64_t x, int64_t y) {
        if (x < 0 || x >= static_cast<int64_t>(m_width) || y < 0 || y >= static_cast<int64_t>(m_height))
            return;
        uint32_t idx = static_cast<uint32_t>(y) * m_width + static_cast<uint32_t>(x);
        if (!visited.insert(idx).second)
            return;
        const Tile& t = m_tiles[idx];
        if (t.IsForegroundEmpty())
            return;
        pending.push({ static_cast<uint32_t>(x), static_cast<uint32_t>(y) });
    };

    auto expandFrom = [&](int64_t x, int64_t y) {
        tryEnqueue(x - 1, y);
        tryEnqueue(x + 1, y);
        tryEnqueue(x, y - 1);
        tryEnqueue(x, y + 1);
        if (allowDiagonals) {
            tryEnqueue(x - 1, y - 1);
            tryEnqueue(x + 1, y - 1);
            tryEnqueue(x - 1, y + 1);
            tryEnqueue(x + 1, y + 1);
        }
    };

    visited.insert(anchorY * m_width + anchorX);
    expandFrom(anchorX, anchorY);

    while (!pending.empty() && result.size() < capacity) {
        auto [x, y] = pending.front();
        pending.pop();
        result.push_back(y * m_width + x);
        if (result.size() >= capacity)
            break;
        expandFrom(x, y);
    }
    return result;
}

LockInfo* World::AddTileLock(uint32_t anchorX, uint32_t anchorY, uint32_t ownerId, const std::string& ownerName, uint16_t itemId, uint32_t capacity) {
    uint32_t anchorIndex = anchorY * m_width + anchorX;
    if (m_anchorToLockIndex.count(anchorIndex))
        return nullptr;

    LockInfo lock;
    lock.m_ownerId = ownerId;
    lock.m_ownerName = ownerName;
    lock.m_itemId = itemId;
    lock.m_anchorX = anchorX;
    lock.m_anchorY = anchorY;
    lock.m_capacity = capacity;
    lock.m_coveredTiles = this->ComputeLockArea(anchorX, anchorY, capacity);

    size_t index = m_tileLocks.size();
    m_tileLocks.push_back(std::move(lock));
    m_anchorToLockIndex[anchorIndex] = index;
    for (uint32_t tileIndex : m_tileLocks[index].m_coveredTiles)
        m_tileToLockIndex[tileIndex] = index;
    return &m_tileLocks[index];
}

LockInfo* World::FindLockAt(uint32_t tileX, uint32_t tileY) {
    auto it = m_tileToLockIndex.find(tileY * m_width + tileX);
    if (it == m_tileToLockIndex.end())
        return nullptr;
    return &m_tileLocks[it->second];
}

LockInfo* World::FindLockByAnchor(uint32_t anchorX, uint32_t anchorY) {
    auto it = m_anchorToLockIndex.find(anchorY * m_width + anchorX);
    if (it == m_anchorToLockIndex.end())
        return nullptr;
    return &m_tileLocks[it->second];
}

void World::RemoveTileLockAt(uint32_t anchorX, uint32_t anchorY) {
    uint32_t anchorIndex = anchorY * m_width + anchorX;
    auto it = m_anchorToLockIndex.find(anchorIndex);
    if (it == m_anchorToLockIndex.end())
        return;
    size_t removedIndex = it->second;

    for (uint32_t tileIndex : m_tileLocks[removedIndex].m_coveredTiles)
        m_tileToLockIndex.erase(tileIndex);
    m_anchorToLockIndex.erase(it);

    m_tileLocks.erase(m_tileLocks.begin() + removedIndex);

    for (auto& [tileIndex, lockIndex] : m_tileToLockIndex) {
        if (lockIndex > removedIndex)
            lockIndex--;
    }
    for (auto& [tileIndex, lockIndex] : m_anchorToLockIndex) {
        if (lockIndex > removedIndex)
            lockIndex--;
    }
}

bool World::RecomputeTileLock(uint32_t anchorX, uint32_t anchorY) {
    uint32_t anchorIndex = anchorY * m_width + anchorX;
    auto it = m_anchorToLockIndex.find(anchorIndex);
    if (it == m_anchorToLockIndex.end())
        return false;
    size_t index = it->second;
    LockInfo& lock = m_tileLocks[index];

    for (uint32_t oldTile : lock.m_coveredTiles)
        m_tileToLockIndex.erase(oldTile);

    lock.m_coveredTiles = lock.m_ignoreAir
        ? this->ComputeFloodFillArea(anchorX, anchorY, lock.m_capacity)
        : this->ComputeLockArea(anchorX, anchorY, lock.m_capacity);
    for (uint32_t newTile : lock.m_coveredTiles)
        m_tileToLockIndex[newTile] = index;
    Logger::Print(INFO, "RecomputeTileLock: anchor=({},{}) nominalCapacity={} ignoreAir={} tilesFound={}", anchorX, anchorY, lock.m_capacity, lock.m_ignoreAir, lock.m_coveredTiles.size());
    return true;
}

const std::vector<LockInfo>& World::GetTileLocks() const {
    return m_tileLocks;
}

void World::RestoreTileLock(LockInfo lock) {
    uint32_t anchorIndex = lock.m_anchorY * m_width + lock.m_anchorX;
    size_t index = m_tileLocks.size();
    m_anchorToLockIndex[anchorIndex] = index;
    for (uint32_t tileIndex : lock.m_coveredTiles)
        m_tileToLockIndex[tileIndex] = index;
    m_tileLocks.push_back(std::move(lock));
}

bool World::HasWorldLock() const {
    return m_hasWorldLock;
}
LockInfo* World::GetWorldLock() {
    return m_hasWorldLock ? &m_worldLock : nullptr;
}
const LockInfo* World::GetWorldLock() const {
    return m_hasWorldLock ? &m_worldLock : nullptr;
}
bool World::TrySetWorldLock(uint32_t ownerId, const std::string& ownerName, uint16_t itemId, uint32_t anchorX, uint32_t anchorY) {
    if (m_hasWorldLock && m_worldLock.m_ownerId != ownerId)
        return false;

    if (m_hasWorldLock && (m_worldLock.m_anchorX != anchorX || m_worldLock.m_anchorY != anchorY))
        return false;
    for (const auto& lock : m_tileLocks) {
        if (lock.m_ownerId != ownerId)
            return false;
    }
    if (!m_hasWorldLock) {
        m_worldLock = LockInfo{};
        m_worldLock.m_ownerId = ownerId;
        m_worldLock.m_ownerName = ownerName;
    }
    m_worldLock.m_itemId = itemId;

    m_worldLock.m_anchorX = anchorX;
    m_worldLock.m_anchorY = anchorY;
    m_hasWorldLock = true;
    return true;
}
void World::ClearWorldLock() {
    m_hasWorldLock = false;
    m_worldLock = LockInfo{};
}
void World::RestoreWorldLock(LockInfo lock) {
    m_worldLock = std::move(lock);
    m_hasWorldLock = true;
}

bool World::IsUserBanned(uint32_t userId) const {
    return std::find(m_bannedUserIds.begin(), m_bannedUserIds.end(), userId) != m_bannedUserIds.end();
}
void World::BanUser(uint32_t userId) {
    if (!IsUserBanned(userId))
        m_bannedUserIds.push_back(userId);
}
void World::UnbanUser(uint32_t userId) {
    m_bannedUserIds.erase(std::remove(m_bannedUserIds.begin(), m_bannedUserIds.end(), userId), m_bannedUserIds.end());
}
const std::vector<uint32_t>& World::GetBannedUserIds() const {
    return m_bannedUserIds;
}
void World::RestoreBannedUsers(std::vector<uint32_t> bannedUserIds) {
    m_bannedUserIds = std::move(bannedUserIds);
}

const std::vector<BulletinPost>& World::GetBulletinPosts() const {
    return m_bulletinPosts;
}
void World::AddBulletinPost(uint32_t tileX, uint32_t tileY, const std::string& name, const std::string& text) {

    std::size_t held = 0;
    for (const auto& post : m_bulletinPosts)
        if (post.m_x == tileX && post.m_y == tileY)
            held++;
    if (held >= kMaxBulletinPosts) {
        for (auto it = m_bulletinPosts.begin(); it != m_bulletinPosts.end(); ++it) {
            if (it->m_x == tileX && it->m_y == tileY) {
                m_bulletinPosts.erase(it);
                break;
            }
        }
    }
    m_bulletinPosts.push_back({ tileX, tileY, name, text });
}
void World::ClearBulletinPosts(uint32_t tileX, uint32_t tileY) {
    m_bulletinPosts.erase(std::remove_if(m_bulletinPosts.begin(), m_bulletinPosts.end(),
        [tileX, tileY](const BulletinPost& post) { return post.m_x == tileX && post.m_y == tileY; }),
        m_bulletinPosts.end());
}
void World::RemoveBulletinPost(std::size_t index) {
    if (index < m_bulletinPosts.size())
        m_bulletinPosts.erase(m_bulletinPosts.begin() + index);
}
void World::RestoreBulletinPosts(std::vector<BulletinPost> posts) {
    m_bulletinPosts = std::move(posts);
}

bool World::CanEdit(uint32_t tileX, uint32_t tileY, uint32_t userId) const {

    uint32_t tileIndex = tileY * m_width + tileX;
    const LockInfo* pTileLock = nullptr;
    auto tileIt = m_tileToLockIndex.find(tileIndex);
    if (tileIt != m_tileToLockIndex.end()) {
        pTileLock = &m_tileLocks[tileIt->second];
    } else {

        auto anchorIt = m_anchorToLockIndex.find(tileIndex);
        if (anchorIt != m_anchorToLockIndex.end())
            pTileLock = &m_tileLocks[anchorIt->second];
    }
    if (pTileLock) {
        if (pTileLock->m_ownerId == userId || pTileLock->m_isPublic)
            return true;
        return std::find(pTileLock->m_accessUserIds.begin(), pTileLock->m_accessUserIds.end(), userId) != pTileLock->m_accessUserIds.end();
    }

    if (m_hasWorldLock) {
        if (m_worldLock.m_ownerId == userId || m_worldLock.m_isPublic)
            return true;
        return std::find(m_worldLock.m_accessUserIds.begin(), m_worldLock.m_accessUserIds.end(), userId) != m_worldLock.m_accessUserIds.end();
    }
    return true;
}

void World::SendForeground(ENetPeer* peer) const {

    for (uint32_t y = 0; y < m_height; y++) {
        for (uint32_t x = 0; x < m_width; x++) {
            const Tile& tile = m_tiles[static_cast<size_t>(y) * m_width + x];

            if (tile.m_foregroundId != 0 && !tile.m_hasDoorExtra) {
                TankPacketData t{};
                t.m_type = NET_GAME_PACKET_SEND_TILE_UPDATE_DATA;
                t.m_netId = -1;
                t.m_itemId = tile.m_foregroundId;
                t.m_tilePositionX = x;
                t.m_tilePositionY = y;

                STankPacket packet(t);
                ENetWrapper::SendPacket(peer, packet, false);
            }
            if (tile.m_backgroundId != 0) {
                TankPacketData t{};
                t.m_type = NET_GAME_PACKET_SEND_TILE_UPDATE_DATA;
                t.m_netId = -1;
                t.m_itemId = tile.m_backgroundId;
                t.m_tilePositionX = x;
                t.m_tilePositionY = y;

                STankPacket packet(t);
                ENetWrapper::SendPacket(peer, packet, false);
            }
        }
    }
}
