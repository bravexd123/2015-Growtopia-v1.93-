#pragma once
#include <cstdint>

inline uint8_t GetPunchEffectId(uint16_t itemId) {
    switch (itemId) {
    case 138:  return 1;
    case 366:  return 2;
    case 472:  return 3;
    case 594:  return 4;
    case 768:  return 5;
    case 900:  return 6;
    case 930:  return 8;
    case 1204: return 10;
    case 1738: return 11;
    case 1484: return 12;
    case 2636: return 29;
    default:   return 0;
    }
}
