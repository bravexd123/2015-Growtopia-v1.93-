#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include <World/Tile.hpp>
#include <World/World.hpp>

struct WorldSnapshot {
    std::string name;
    uint32_t width = 0, height = 0;
    std::vector<Tile> tiles;
    std::vector<LockInfo> tileLocks;
    bool hasWorldLock = false;
    LockInfo worldLock;
    std::vector<uint32_t> bannedUserIds;

    std::vector<BulletinPost> bulletinPosts;

    int32_t weather = 0;
    uint32_t weatherAnchorX = 0, weatherAnchorY = 0;
};

class JsonWorldTable {
public:
    explicit JsonWorldTable(std::filesystem::path directory);
    ~JsonWorldTable() = default;

public:
    bool Exists(const std::string& name) const;
    bool Load(const std::string& name, WorldSnapshot& outSnapshot) const;
    bool Save(const World& world) const;

    std::vector<std::string> FindWorldsOwnedBy(uint32_t userId) const;

private:
    std::filesystem::path PathFor(const std::string& lowerName) const;

private:
    std::filesystem::path m_directory;
};
