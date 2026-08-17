#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <enet/enet.h>
#include <World/Tile.hpp>

class Player;
class ISPacket;

struct LockInfo {
    uint32_t m_ownerId = 0;
    std::string m_ownerName;
    uint16_t m_itemId = 0;
    uint32_t m_anchorX = 0, m_anchorY = 0;
    bool m_isPublic = false;
    bool m_ignoreAir = false;

    bool m_disableCustomMusic = false;
    bool m_hideCustomMusic = false;
    std::vector<uint32_t> m_accessUserIds;

    std::vector<uint32_t> m_coveredTiles;

    uint32_t m_capacity = 0;
};

struct DroppedItem {
    int32_t  m_uid = 0;
    uint16_t m_itemId = ITEM_BLANK;
    uint8_t  m_count = 1;
    float    m_x = 0;
    float    m_y = 0;
};

struct BulletinPost {
    uint32_t m_x = 0;
    uint32_t m_y = 0;
    std::string m_name;
    std::string m_text;
};

class World {
public:
    World() = default;
    ~World() = default;

    void Init(const std::string& name, uint32_t width = 100, uint32_t height = 60);

    bool LoadSnapshot(const std::string& name, uint32_t width, uint32_t height, std::vector<Tile> tiles);

    const std::string& GetName() const;
    uint32_t GetWidth() const;
    uint32_t GetHeight() const;

    bool IsValidPosition(uint32_t tileX, uint32_t tileY) const;
    Tile* GetTile(uint32_t tileX, uint32_t tileY);
    bool SetTile(uint32_t tileX, uint32_t tileY, const Tile& tile);
    const std::vector<Tile>& GetTiles() const;

    bool FindDoorTile(uint32_t& outX, uint32_t& outY) const;

    void AddPlayer(Player* pPlayer);
    void RemovePlayer(Player* pPlayer);
    std::vector<Player*> GetPlayers() const;
    size_t GetPlayerCount() const;

    int32_t AssignNetId(Player* pPlayer);
    void ReleaseNetId(int32_t netId);

    void BroadcastPacket(ISPacket& packet, Player* pExclude = nullptr);

    void BroadcastPlayerLeft(Player* pLeaving);

    void BroadcastTileUpdate(uint32_t tileX, uint32_t tileY);

    DroppedItem* SpawnDrop(uint16_t itemId, uint8_t count, float x, float y);

    DroppedItem* FindMergeableDrop(uint16_t itemId, uint8_t count, float x, float y);

    bool RemoveDrop(int32_t uid, DroppedItem& out);

    const std::vector<DroppedItem>& GetDroppedItems() const;

    void RenumberDroppedItemsForJoin();

    int32_t GetWeather() const { return m_weatherId; }
    void SetWeather(int32_t weatherId, uint32_t anchorX, uint32_t anchorY) {
        m_weatherId = weatherId;
        m_weatherAnchorX = anchorX;
        m_weatherAnchorY = anchorY;
    }
    uint32_t GetWeatherAnchorX() const { return m_weatherAnchorX; }
    uint32_t GetWeatherAnchorY() const { return m_weatherAnchorY; }

    uint32_t AgeWorld(int64_t seconds);

    std::vector<uint8_t> SerializeMapData() const;

    void SendForeground(ENetPeer* peer) const;

public:

    LockInfo* AddTileLock(uint32_t anchorX, uint32_t anchorY, uint32_t ownerId, const std::string& ownerName, uint16_t itemId, uint32_t capacity);

    LockInfo* FindLockAt(uint32_t tileX, uint32_t tileY);

    LockInfo* FindLockByAnchor(uint32_t anchorX, uint32_t anchorY);

    void RemoveTileLockAt(uint32_t anchorX, uint32_t anchorY);

    bool RecomputeTileLock(uint32_t anchorX, uint32_t anchorY);
    const std::vector<LockInfo>& GetTileLocks() const;

    void RestoreTileLock(LockInfo lock);

    bool HasWorldLock() const;
    LockInfo* GetWorldLock();
    const LockInfo* GetWorldLock() const;

    bool TrySetWorldLock(uint32_t ownerId, const std::string& ownerName, uint16_t itemId, uint32_t anchorX, uint32_t anchorY);
    void ClearWorldLock();
    void RestoreWorldLock(LockInfo lock);

    bool IsUserBanned(uint32_t userId) const;
    void BanUser(uint32_t userId);
    void UnbanUser(uint32_t userId);
    const std::vector<uint32_t>& GetBannedUserIds() const;

    void RestoreBannedUsers(std::vector<uint32_t> bannedUserIds);

    static constexpr std::size_t kMaxBulletinPosts = 20;
    const std::vector<BulletinPost>& GetBulletinPosts() const;
    void AddBulletinPost(uint32_t tileX, uint32_t tileY, const std::string& name, const std::string& text);
    void ClearBulletinPosts(uint32_t tileX, uint32_t tileY);
    void RemoveBulletinPost(std::size_t index);
    void RestoreBulletinPosts(std::vector<BulletinPost> posts);

    bool CanEdit(uint32_t tileX, uint32_t tileY, uint32_t userId) const;

    std::vector<uint32_t> ComputeLockArea(uint32_t anchorX, uint32_t anchorY, uint32_t capacity) const;

    std::vector<uint32_t> ComputeFloodFillArea(uint32_t anchorX, uint32_t anchorY, uint32_t capacity) const;

    void GenerateEmpty();

    void SetName(const std::string& name) { m_name = name; }

private:
    void Generate();

private:
    std::string m_name;
    uint32_t m_width = 0;
    uint32_t m_height = 0;
    std::vector<Tile> m_tiles;

    std::vector<Player*> m_players;
    int32_t m_nextNetId = 0;

    std::vector<DroppedItem> m_droppedItems;

    int32_t m_nextDropUid = 1;

private:
    std::vector<LockInfo> m_tileLocks;

    std::unordered_map<uint32_t, size_t> m_tileToLockIndex;

    std::unordered_map<uint32_t, size_t> m_anchorToLockIndex;

    int32_t m_weatherId = 0;
    uint32_t m_weatherAnchorX = 0, m_weatherAnchorY = 0;

    bool m_hasWorldLock = false;
    LockInfo m_worldLock;

    std::vector<uint32_t> m_bannedUserIds;
    std::vector<BulletinPost> m_bulletinPosts;
};
