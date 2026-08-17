#include <Manager/Database/Table/JsonWorldTable.hpp>
#include <fstream>
#include <algorithm>
#include <cctype>
#include <nlohmann/json.hpp>
#include <Logger/Logger.hpp>

static std::string ToLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) { return std::tolower(c); });
    return value;
}

JsonWorldTable::JsonWorldTable(std::filesystem::path directory) : m_directory(std::move(directory)) {}

std::filesystem::path JsonWorldTable::PathFor(const std::string& lowerName) const {
    return m_directory / (lowerName + ".json");
}

bool JsonWorldTable::Exists(const std::string& name) const {
    return std::filesystem::exists(this->PathFor(ToLower(name)));
}

bool JsonWorldTable::Load(const std::string& name, WorldSnapshot& outSnapshot) const {
    auto path = this->PathFor(ToLower(name));
    if (!std::filesystem::exists(path))
        return false;

    std::ifstream file(path);
    if (!file.is_open())
        return false;

    try {
        nlohmann::json j;
        file >> j;

        uint32_t width = j.value("width", 0u);
        uint32_t height = j.value("height", 0u);
        if (width == 0 || height == 0 || !j.contains("foreground") || !j.contains("background"))
            return false;

        const auto& fg = j["foreground"];
        const auto& bg = j["background"];
        size_t expected = static_cast<size_t>(width) * height;
        if (fg.size() != expected || bg.size() != expected)
            return false;

        outSnapshot.name = j.value("name", name);
        outSnapshot.width = width;
        outSnapshot.height = height;
        outSnapshot.tiles.assign(expected, Tile{});
        for (size_t i = 0; i < expected; i++) {
            outSnapshot.tiles[i].m_foregroundId = fg[i].get<uint16_t>();
            outSnapshot.tiles[i].m_backgroundId = bg[i].get<uint16_t>();
        }

        if (j.contains("tileFlags")) {
            for (const auto& entry : j["tileFlags"]) {
                size_t index = entry.value("i", static_cast<size_t>(0));
                if (index >= expected)
                    continue;
                outSnapshot.tiles[index].m_flags = entry.value("f", static_cast<uint16_t>(0));
            }
        }

        if (j.contains("tileState")) {
            for (const auto& entry : j["tileState"]) {
                size_t index = entry.value("i", static_cast<size_t>(0));
                if (index >= expected)
                    continue;
                outSnapshot.tiles[index].m_lastHarvestUnix = entry.value("h", static_cast<int64_t>(0));
                outSnapshot.tiles[index].m_randomValue = entry.value("r", static_cast<uint8_t>(0));

                outSnapshot.tiles[index].m_bulletinPublicCanAdd = entry.value("bp", false);
                outSnapshot.tiles[index].m_bulletinHideNames = entry.value("bh", false);
            }
        }
        if (j.contains("seeds")) {
            for (const auto& entry : j["seeds"]) {
                size_t index = entry.value("i", static_cast<size_t>(0));
                if (index >= expected)
                    continue;
                outSnapshot.tiles[index].m_plantedAtUnix = entry.value("t", static_cast<int64_t>(0));
                outSnapshot.tiles[index].m_fruitCount = entry.value("f", static_cast<uint8_t>(1));
            }
        }
        if (j.contains("doors")) {
            for (const auto& entry : j["doors"]) {
                size_t index = entry.value("i", static_cast<size_t>(0));
                if (index >= expected)
                    continue;
                outSnapshot.tiles[index].m_hasDoorExtra = true;
                outSnapshot.tiles[index].m_doorExtra.m_label = entry.value("l", std::string("EXIT"));

                outSnapshot.tiles[index].m_doorExtra.m_target = entry.value("d", std::string());
                outSnapshot.tiles[index].m_doorExtra.m_targetId = entry.value("did", std::string());
            }
        }

        if (j.contains("signs")) {
            for (const auto& entry : j["signs"]) {
                size_t index = entry.value("i", static_cast<size_t>(0));
                if (index >= expected)
                    continue;
                outSnapshot.tiles[index].m_hasSignExtra = true;
                outSnapshot.tiles[index].m_signExtra.m_text = entry.value("t", std::string());
            }
        }
        if (j.contains("locks")) {
            for (const auto& entry : j["locks"]) {
                LockInfo lock;
                lock.m_ownerId = entry.value("owner", 0u);
                lock.m_ownerName = entry.value("ownerName", std::string());
                lock.m_itemId = entry.value("item", static_cast<uint16_t>(0));
                lock.m_anchorX = entry.value("ax", 0u);
                lock.m_anchorY = entry.value("ay", 0u);
                lock.m_isPublic = entry.value("public", false);
                lock.m_ignoreAir = entry.value("ignoreAir", false);
                if (entry.contains("access"))
                    for (const auto& id : entry["access"])
                        lock.m_accessUserIds.push_back(id.get<uint32_t>());
                if (entry.contains("tiles"))
                    for (const auto& t : entry["tiles"])
                        lock.m_coveredTiles.push_back(t.get<uint32_t>());

                lock.m_capacity = entry.value("capacity", static_cast<uint32_t>(lock.m_coveredTiles.size()));
                outSnapshot.tileLocks.push_back(std::move(lock));
            }
        }
        if (j.contains("worldLock")) {
            const auto& wl = j["worldLock"];
            LockInfo lock;
            lock.m_ownerId = wl.value("owner", 0u);
            lock.m_ownerName = wl.value("ownerName", std::string());
            lock.m_itemId = wl.value("item", static_cast<uint16_t>(0));
            lock.m_isPublic = wl.value("public", false);
            lock.m_anchorX = wl.value("ax", 0u);
            lock.m_anchorY = wl.value("ay", 0u);
            lock.m_disableCustomMusic = wl.value("disableCustomMusic", false);
            lock.m_hideCustomMusic = wl.value("hideCustomMusic", false);
            if (wl.contains("access"))
                for (const auto& id : wl["access"])
                    lock.m_accessUserIds.push_back(id.get<uint32_t>());
            outSnapshot.worldLock = std::move(lock);
            outSnapshot.hasWorldLock = true;
        }
        if (j.contains("bannedUsers"))
            for (const auto& id : j["bannedUsers"])
                outSnapshot.bannedUserIds.push_back(id.get<uint32_t>());
        if (j.contains("bulletin")) {
            for (const auto& entry : j["bulletin"]) {
                BulletinPost post;
                post.m_x = entry.value("x", 0u);
                post.m_y = entry.value("y", 0u);
                post.m_name = entry.value("n", std::string());
                post.m_text = entry.value("t", std::string());
                outSnapshot.bulletinPosts.push_back(std::move(post));
            }
        }
        outSnapshot.weather = j.value("weather", 0);
        outSnapshot.weatherAnchorX = j.value("weatherAX", 0u);
        outSnapshot.weatherAnchorY = j.value("weatherAY", 0u);
        return true;
    }
    catch (const nlohmann::json::exception& e) {
        Logger::Print(WARNING, "JsonWorldTable::Load: failed to parse '{}': {}", path.string(), e.what());
        return false;
    }
}

std::vector<std::string> JsonWorldTable::FindWorldsOwnedBy(uint32_t userId) const {
    std::vector<std::string> owned;
    if (!std::filesystem::exists(m_directory))
        return owned;

    for (const auto& entry : std::filesystem::directory_iterator(m_directory)) {
        if (!entry.is_regular_file() || entry.path().extension() != ".json")
            continue;

        std::ifstream file(entry.path());
        if (!file.is_open())
            continue;

        try {
            nlohmann::json j;
            file >> j;
            if (!j.contains("worldLock"))
                continue;
            if (j["worldLock"].value("owner", 0u) == userId)
                owned.push_back(j.value("name", entry.path().stem().string()));
        }
        catch (const nlohmann::json::exception&) {
            continue;
        }
    }
    return owned;
}

bool JsonWorldTable::Save(const World& world) const {
    const auto& tiles = world.GetTiles();

    nlohmann::json fg = nlohmann::json::array();
    nlohmann::json bg = nlohmann::json::array();
    nlohmann::json seeds = nlohmann::json::array();
    nlohmann::json doors = nlohmann::json::array();
    nlohmann::json signs = nlohmann::json::array();
    nlohmann::json tileFlags = nlohmann::json::array();
    nlohmann::json tileState = nlohmann::json::array();
    for (size_t i = 0; i < tiles.size(); i++) {
        const auto& tile = tiles[i];
        fg.push_back(tile.m_foregroundId);
        bg.push_back(tile.m_backgroundId);
        if ((tile.m_flags & kTilePersistedFlags) != 0) {
            nlohmann::json entry;
            entry["i"] = i;
            entry["f"] = static_cast<uint16_t>(tile.m_flags & kTilePersistedFlags);
            tileFlags.push_back(std::move(entry));
        }
        if (tile.m_lastHarvestUnix != 0 || tile.m_randomValue != 0 ||
            tile.m_bulletinPublicCanAdd || tile.m_bulletinHideNames) {
            nlohmann::json entry;
            entry["i"] = i;
            entry["h"] = tile.m_lastHarvestUnix;
            entry["r"] = tile.m_randomValue;
            if (tile.m_bulletinPublicCanAdd) entry["bp"] = true;
            if (tile.m_bulletinHideNames) entry["bh"] = true;
            tileState.push_back(std::move(entry));
        }
        if (tile.m_plantedAtUnix != 0) {
            nlohmann::json entry;
            entry["i"] = i;
            entry["t"] = tile.m_plantedAtUnix;
            entry["f"] = tile.m_fruitCount;
            seeds.push_back(std::move(entry));
        }
        if (tile.m_hasDoorExtra) {
            nlohmann::json entry;
            entry["i"] = i;
            entry["l"] = tile.m_doorExtra.m_label;
            entry["d"] = tile.m_doorExtra.m_target;
            entry["did"] = tile.m_doorExtra.m_targetId;
            doors.push_back(std::move(entry));
        }
        if (tile.m_hasSignExtra) {
            nlohmann::json entry;
            entry["i"] = i;
            entry["t"] = tile.m_signExtra.m_text;
            signs.push_back(std::move(entry));
        }
    }

    nlohmann::json locks = nlohmann::json::array();
    for (const auto& lock : world.GetTileLocks()) {
        nlohmann::json entry;
        entry["owner"] = lock.m_ownerId;
        entry["ownerName"] = lock.m_ownerName;
        entry["item"] = lock.m_itemId;
        entry["ax"] = lock.m_anchorX;
        entry["ay"] = lock.m_anchorY;
        entry["public"] = lock.m_isPublic;
        entry["ignoreAir"] = lock.m_ignoreAir;
        entry["access"] = lock.m_accessUserIds;
        entry["tiles"] = lock.m_coveredTiles;
        entry["capacity"] = lock.m_capacity;
        locks.push_back(std::move(entry));
    }

    nlohmann::json j;
    j["name"] = world.GetName();
    j["width"] = world.GetWidth();
    j["height"] = world.GetHeight();
    j["foreground"] = std::move(fg);
    j["background"] = std::move(bg);
    j["seeds"] = std::move(seeds);
    j["doors"] = std::move(doors);
    j["signs"] = std::move(signs);
    j["tileFlags"] = std::move(tileFlags);
    j["tileState"] = std::move(tileState);
    j["weather"] = world.GetWeather();
    j["weatherAX"] = world.GetWeatherAnchorX();
    j["weatherAY"] = world.GetWeatherAnchorY();
    j["locks"] = std::move(locks);
    if (world.HasWorldLock()) {
        const auto& wl = *world.GetWorldLock();
        nlohmann::json wlj;
        wlj["owner"] = wl.m_ownerId;
        wlj["ownerName"] = wl.m_ownerName;
        wlj["item"] = wl.m_itemId;
        wlj["public"] = wl.m_isPublic;
        wlj["access"] = wl.m_accessUserIds;

        wlj["ax"] = wl.m_anchorX;
        wlj["ay"] = wl.m_anchorY;
        wlj["disableCustomMusic"] = wl.m_disableCustomMusic;
        wlj["hideCustomMusic"] = wl.m_hideCustomMusic;
        j["worldLock"] = std::move(wlj);
    }
    j["bannedUsers"] = world.GetBannedUserIds();
    {
        nlohmann::json posts = nlohmann::json::array();
        for (const auto& post : world.GetBulletinPosts()) {
            nlohmann::json entry;
            entry["x"] = post.m_x;
            entry["y"] = post.m_y;
            entry["n"] = post.m_name;
            entry["t"] = post.m_text;
            posts.push_back(std::move(entry));
        }
        j["bulletin"] = std::move(posts);
    }

    std::string lowerName = ToLower(world.GetName());
    std::ofstream file(this->PathFor(lowerName));
    if (!file.is_open())
        return false;
    file << j.dump();
    return true;
}
