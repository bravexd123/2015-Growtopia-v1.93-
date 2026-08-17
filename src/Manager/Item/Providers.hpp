#pragma once
#include <cstdint>

enum class ProviderYield {
    None,
    Item,
    Gems,
    Chemical,
};

struct ProviderInfo {
    ProviderYield m_kind = ProviderYield::None;
    uint16_t m_itemId = 0;
};

inline ProviderInfo GetProviderInfo(uint16_t itemId) {
    switch (itemId) {
    case 866:  return { ProviderYield::Item, 868 };
    case 1044: return { ProviderYield::Item, 868 };
    case 872:  return { ProviderYield::Item, 874 };
    case 1632: return { ProviderYield::Item, 1634 };
    case 2890: return { ProviderYield::Item, 1634 };
    case 1636: return { ProviderYield::Item, 1638 };
    case 2798: return { ProviderYield::Item, 822 };
    case 1008: return { ProviderYield::Gems, 0 };
    case 928:  return { ProviderYield::Chemical, 0 };
    default:   return {};
    }
}

inline uint16_t RollScienceStationChemical(int roll16, int roll8, int roll6, int roll4) {
    if (roll16 == 0) return 918;
    if (roll8  == 0) return 920;
    if (roll6  == 0) return 924;
    if (roll4  == 0) return 916;
    return 914;
}
