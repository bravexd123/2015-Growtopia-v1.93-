#include <Manager/Item/ItemManager.hpp>
#include <fstream>
#include <Logger/Logger.hpp>
#include <Utils/BinaryReader.hpp>
#include <Utils/FileManager.hpp>

ItemManager g_itemManager;
ItemManager* GetItemManager() {
    return &g_itemManager;
}

bool ItemManager::Serialize() {
    if (!std::filesystem::exists("cache/items.dat"))
        return false;
    auto itemData = FileManager::ReadAsByteArray("cache/items.dat");
    if (!itemData.data() || itemData.empty())
        return false;

    BinaryReader br(itemData.data());
    m_version = br.Read<uint16_t>();
    m_itemCount = br.Read<uint32_t>();

    m_items.reserve(m_itemCount);
    for (auto index = 0; index < m_itemCount; index++) {
        auto* item = m_items.emplace_back(new ItemInfo{});
        item->Serialize(br);

        if (index != item->m_Id)
            break;
    }

    m_clientItemsData = std::move(itemData);

    uint32_t hash = 2166136261u;
    for (uint8_t byte : m_clientItemsData) {
        hash ^= byte;
        hash *= 16777619u;
    }
    m_itemsDatHash = hash;

    this->LoadDescriptions();

    Logger::Print(INFO, "{} >> Serialized items.dat with {} items loaded.", fmt::format(fmt::emphasis::bold | fg(fmt::color::cornsilk), "ItemManager"), this->GetItemsLoaded());
    return true;
}

void ItemManager::LoadDescriptions() {
    std::ifstream file("cache/item_descriptions.txt");
    if (!file.is_open()) {
        Logger::Print(WARNING, "ItemManager::LoadDescriptions: cache/item_descriptions.txt not found - INFO dialog will use generic descriptions.");
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        auto sep = line.find('|');
        if (sep == std::string::npos)
            continue;
        try {
            uint32_t itemId = static_cast<uint32_t>(std::stoul(line.substr(0, sep)));
            m_descriptions.emplace(itemId, line.substr(sep + 1));
        }
        catch (const std::exception&) {
            continue;
        }
    }

    Logger::Print(INFO, "{} >> Loaded {} item descriptions.", fmt::format(fmt::emphasis::bold | fg(fmt::color::cornsilk), "ItemManager"), m_descriptions.size());
}
void ItemManager::Encode() {
    std::size_t alloc = 6;
    for (ItemInfo* item : m_items)
        alloc += item->GetMemoryUsage() + 20;

    BinaryWriter buffer(alloc);
    buffer.Write<uint16_t>(this->m_version);
    buffer.Write<uint32_t>(this->m_itemCount);
    for (ItemInfo* item : m_items)
        item->Pack(buffer);

    FileManager::WriteAsBytes("cache/items.dat", reinterpret_cast<char*>(buffer.Get()), buffer.GetPosition());
}
void ItemManager::Kill() {
    for (auto& item : m_items) {
        if (!item)
            continue;
        delete item;
        item = nullptr;
    }
    m_items.clear();
}

ItemInfo* ItemManager::GetItem(uint32_t itemId) {
    if (itemId < 0 || itemId > m_items.size())
        return nullptr;
    return m_items[itemId];
}

ItemInfo* ItemManager::GetItemByName(const std::string& name) {
    for (ItemInfo* item : m_items) {
        if (item && item->m_name == name)
            return item;
    }
    return nullptr;
}

std::vector<ItemInfo*> ItemManager::GetSeedsInRarityRange(uint16_t minRarity, uint16_t maxRarity) {
    std::vector<ItemInfo*> result;
    for (ItemInfo* item : m_items) {
        if (item && item->IsSeed() && item->m_rarity >= minRarity && item->m_rarity <= maxRarity)
            result.push_back(item);
    }
    return result;
}
std::vector<ItemInfo*> ItemManager::GetClothingInRarityRange(uint16_t minRarity, uint16_t maxRarity) {
    std::vector<ItemInfo*> result;
    for (ItemInfo* item : m_items) {
        if (item && item->IsClothing() && item->m_rarity >= minRarity && item->m_rarity <= maxRarity)
            result.push_back(item);
    }
    return result;
}

std::vector<ItemInfo*> ItemManager::GetItems() {
    return m_items;
}
size_t ItemManager::GetItemsLoaded() {
    return m_items.size();
}
