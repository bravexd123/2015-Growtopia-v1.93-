#include <World/WorldPool.hpp>
#include <string>
#include <unordered_map>
#include <Manager/Database/Database.hpp>

WorldPool::~WorldPool() {

    m_worlds.clear();
}

std::unordered_map<std::string, std::shared_ptr<World>> WorldPool::GetWorlds() {
    return this->m_worlds;
}
size_t WorldPool::GetActiveWorlds() const {
    return m_worlds.size();
}

std::shared_ptr<World> WorldPool::NewWorld(std::string vName) {
    if (auto it = m_worlds.find(vName); it != m_worlds.end())
        return it->second;

    auto world = std::make_shared<World>();

    WorldSnapshot snapshot;
    if (GetDatabase()->GetWorldTable()->Load(vName, snapshot) &&
        world->LoadSnapshot(vName, snapshot.width, snapshot.height, std::move(snapshot.tiles))) {

        for (auto& lock : snapshot.tileLocks)
            world->RestoreTileLock(std::move(lock));
        if (snapshot.hasWorldLock)
            world->RestoreWorldLock(std::move(snapshot.worldLock));
        world->RestoreBannedUsers(std::move(snapshot.bannedUserIds));
        world->RestoreBulletinPosts(std::move(snapshot.bulletinPosts));
        world->SetWeather(snapshot.weather, snapshot.weatherAnchorX, snapshot.weatherAnchorY);
    } else {
        world->Init(vName);

        GetDatabase()->GetWorldTable()->Save(*world);
    }

    m_worlds.insert_or_assign(vName, world);
    return world;
}
void WorldPool::RemoveWorld(std::string vName) {
    m_worlds.erase(vName);
}
std::shared_ptr<World> WorldPool::GetWorld(std::string vName) {
    auto it = m_worlds.find(vName);
    return it != m_worlds.end() ? it->second : nullptr;
}

bool WorldPool::SwapWorldNames(const std::string& a, const std::string& b) {
    auto itA = m_worlds.find(a);
    auto itB = m_worlds.find(b);
    if (a == b || itA == m_worlds.end() || itB == m_worlds.end())
        return false;

    std::shared_ptr<World> worldA = itA->second;
    std::shared_ptr<World> worldB = itB->second;
    worldA->SetName(b);
    worldB->SetName(a);
    m_worlds.insert_or_assign(a, worldB);
    m_worlds.insert_or_assign(b, worldA);

    GetDatabase()->GetWorldTable()->Save(*worldA);
    GetDatabase()->GetWorldTable()->Save(*worldB);
    return true;
}
