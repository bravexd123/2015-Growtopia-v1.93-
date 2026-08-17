#pragma once
#include <vector>
#include <filesystem>
#include <unordered_map>
#include <Manager/Item/ItemInfo.hpp>

class ItemManager {
public:
    bool Serialize();
    void Encode();
    void Kill();

    ItemInfo* GetItem(uint32_t itemId);
    ItemInfo* GetItemByName(const std::string& name);

    std::vector<ItemInfo*> GetSeedsInRarityRange(uint16_t minRarity, uint16_t maxRarity);
    std::vector<ItemInfo*> GetClothingInRarityRange(uint16_t minRarity, uint16_t maxRarity);

    std::vector<ItemInfo*> GetItems();
    size_t GetItemsLoaded();

    const std::vector<uint8_t>& GetClientItemsData() const { return m_clientItemsData; }

    uint32_t GetItemsDatHash() const { return m_itemsDatHash; }

    std::string GetDescription(uint32_t itemId) const {
        auto it = m_descriptions.find(itemId);
        return it != m_descriptions.end() ? it->second : std::string{};
    }

public:
    ItemManager() = default;
    ~ItemManager() = default;

private:
    void LoadDescriptions();

private:
    uint16_t m_version;
    uint32_t m_itemCount;

    std::vector<ItemInfo*> m_items;
    std::vector<uint8_t> m_clientItemsData;
    uint32_t m_itemsDatHash = 0;
    std::unordered_map<uint32_t, std::string> m_descriptions;
};

ItemManager* GetItemManager();
