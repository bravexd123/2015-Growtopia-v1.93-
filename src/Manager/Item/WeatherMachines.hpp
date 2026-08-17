#pragma once
#include <cstdint>

inline int32_t GetWeatherIdForItem(uint16_t itemId) {
    switch (itemId) {
    case 932:  return 0;
    case 934:  return 2;
    case 946:  return 3;
    case 984:  return 5;
    case 1210: return 8;
    case 1364: return 11;
    case 1490: return 10;
    case 1750: return 15;
    case 2046: return 17;
    case 2284: return 18;
    case 2744: return 19;
    default:   return 0;
    }
}
