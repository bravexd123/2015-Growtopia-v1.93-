#pragma once
#include <cstdint>
#include <cstdlib>
#include <string>
#include <ctime>
#include <Manager/Item/ItemComponent.hpp>

struct TileExtraDoor {
    std::string m_label;

    std::string m_target;
    std::string m_targetId;
};

struct TileExtraSign {
    std::string m_text;
};

enum eTileFlags : uint16_t {
    TILEFLAG_EXTRA    = 0x0001,
    TILEFLAG_LOCKED   = 0x0010,
    TILEFLAG_FLIPPED  = 0x0020,
    TILEFLAG_OPEN     = 0x0040,
    TILEFLAG_PUBLIC   = 0x0080,
    TILEFLAG_WATER    = 0x0400,
    TILEFLAG_GLUE     = 0x0800,
    TILEFLAG_FIRE     = 0x1000,
};

inline constexpr uint16_t kTilePersistedFlags =
    TILEFLAG_FLIPPED | TILEFLAG_OPEN | TILEFLAG_PUBLIC |
    TILEFLAG_WATER | TILEFLAG_GLUE | TILEFLAG_FIRE;

struct Tile {
    uint16_t m_foregroundId = ITEM_BLANK;
    uint16_t m_backgroundId = ITEM_BLANK;
    uint16_t m_flags = 0;
    uint8_t  m_currentBreakHits = 0;

    int64_t  m_lastHitUnix = 0;

    bool m_hasDoorExtra = false;
    TileExtraDoor m_doorExtra;

    bool m_hasSignExtra = false;
    TileExtraSign m_signExtra;

    int64_t m_plantedAtUnix = 0;

    int64_t m_lastHarvestUnix = 0;

    uint8_t m_randomValue = 0;

    uint8_t m_fruitCount = 1;

    bool m_bulletinPublicCanAdd = false;
    bool m_bulletinHideNames = false;

    std::string m_spotlightName;

    bool IsForegroundEmpty() const { return m_foregroundId == ITEM_BLANK; }
    bool IsBackgroundEmpty() const { return m_backgroundId == ITEM_BLANK; }
    void ResetBreakProgress() { m_currentBreakHits = 0; m_lastHitUnix = 0; }

    bool HasFlag(uint16_t flag) const { return (m_flags & flag) != 0; }
    void SetFlag(uint16_t flag) { m_flags |= flag; }
    void ClearFlag(uint16_t flag) { m_flags = static_cast<uint16_t>(m_flags & ~flag); }
    void ToggleFlag(uint16_t flag) { m_flags = static_cast<uint16_t>(m_flags ^ flag); }

    static constexpr int64_t kBreakProgressResetSeconds = 5;

    void ExpireBreakProgressIfStale() {
        if (m_currentBreakHits == 0)
            return;
        if (static_cast<int64_t>(std::time(nullptr)) - m_lastHitUnix >= kBreakProgressResetSeconds)
            m_currentBreakHits = 0;
    }

    void SetPlanted() {
        m_plantedAtUnix = static_cast<int64_t>(std::time(nullptr));

        m_fruitCount = static_cast<uint8_t>(1 + (std::rand() % 4));
    }

    void ClearForeground() {
        m_foregroundId = ITEM_BLANK;
        m_plantedAtUnix = 0;
        m_fruitCount = 1;
        m_currentBreakHits = 0;
        m_lastHitUnix = 0;

        m_flags = 0;

        m_hasDoorExtra = false;
        m_doorExtra = {};
        m_hasSignExtra = false;
        m_signExtra = {};
        m_lastHarvestUnix = 0;
        m_randomValue = 0;
    }

    bool IsProviderReady(uint32_t cooldownSeconds) const {
        return (static_cast<int64_t>(std::time(nullptr)) - m_lastHarvestUnix) >= static_cast<int64_t>(cooldownSeconds);
    }

    bool IsFullyGrown(uint32_t growTimeSeconds) const {
        return (static_cast<int64_t>(std::time(nullptr)) - m_plantedAtUnix) >= static_cast<int64_t>(growTimeSeconds);
    }
};
