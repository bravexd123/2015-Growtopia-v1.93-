#pragma once
#include <cstdint>
#include <Manager/Item/ItemComponent.hpp>

inline bool IsUndroppableItem(uint16_t itemId) {
    return itemId == ITEM_FIST || itemId == ITEM_WRENCH;
}
#include <string>
#include <Manager/Item/ItemType.hpp>
#include <Manager/Item/ItemFlags.hpp>
#include <Utils/BinaryReader.hpp>
#include <Utils/BinaryWriter.hpp>
#include <Utils/Math.hpp>

struct ItemInfo {
    uint32_t m_Id;
    uint16_t m_properties = 0;
    uint8_t m_itemType = 0;
    uint8_t m_material = 0;

    std::string m_name = "";
    std::string m_textureFile = "";

    uint32_t m_textureFileHash = 0;
    uint8_t m_visualEffect = 0;
    uint32_t m_cookTime = 0;

    CL_Vec2<uint8_t> m_texture = CL_Vec2<uint8_t>{ 0, 0 };
    uint8_t m_spreadType = 0;
    uint8_t m_layer = 0;
    uint8_t m_collisionType = 0;
    uint8_t m_breakHits = 0;

    uint32_t m_growTime = 0;
    uint8_t m_clothingType = 0;
    uint16_t m_rarity = 0;
    uint8_t m_maxAmount = 0;

    std::string m_extraFile = "";
    uint32_t m_extraFileHash = 0;
    uint32_t m_animMs = 0;

    struct {
        uint8_t m_base = 0;
        uint8_t m_overlay = 0;
        uint32_t m_color = 0;
        uint32_t m_overlayColor = 0;
    } m_seed;

    struct {
        uint8_t m_base = 0;
        uint8_t m_leaves = 0;
    } m_tree;

    uint16_t m_seed1 = 0;
    uint16_t m_seed2 = 0;
    uint32_t m_bloomTime = 0;

    bool m_hasExtra = false;
    CL_Vec2<uint8_t> m_defaultTexture = CL_Vec2<uint8_t>{ 0, 0 };
    std::string m_description = "This is a seed.";

    std::string Cypher(const std::string& input) {
        constexpr std::string_view key{ "PBG892FXX982ABC*" };
        std::string ret(input.size(), 0);

        for (uint32_t index = 0; index < input.size(); index++)
            ret[index] = static_cast<char>(input[index] ^ key[(index + this->m_Id) % key.size()]);
        return ret;
    }
    bool IsBackground() const {
        return m_itemType == ITEMTYPE_BACKGROUND ||
            m_itemType == ITEMTYPE_BACK_BOOMBOX ||
            m_itemType == ITEMTYPE_MUSIC_NOTE;
    }
    bool IsLock() const {
        return m_itemType == ITEMTYPE_LOCK;
    }

    bool IsFarmableBlock() const {
        return m_itemType == ITEMTYPE_FOREGROUND || m_itemType == ITEMTYPE_BACKGROUND
            || m_itemType == ITEMTYPE_FOREGROUND_WITH_EXTRA_FRAME || m_itemType == ITEMTYPE_BACKGD_SFX_EXTRA_FRAME;
    }
    bool IsSeed() const {
        return m_itemType == ITEMTYPE_SEED;
    }

    bool IsDoor() const {
        return m_itemType == ITEMTYPE_DOOR || m_itemType == ITEMTYPE_MAIN_DOOR ||
               m_itemType == ITEMTYPE_PORTAL;
    }
    bool IsClothing() const {
        return m_itemType == ITEMTYPE_CLOTHES;
    }

    bool HasFlag1(uint8_t flag) const { return (m_properties & flag) != 0; }
    bool HasFlag2(uint8_t flag) const { return ((m_properties >> 8) & flag) != 0; }

    bool IsUntradable() const { return HasFlag2(ITEMFLAG2_UNTRADABLE); }

    bool IsModOnly() const { return HasFlag2(ITEMFLAG2_MOD); }

    bool IsPermanent() const { return HasFlag1(ITEMFLAG1_PERMANENT); }

    bool IsPublic() const { return HasFlag2(ITEMFLAG2_PUBLIC); }

    bool RequiresWorldLock() const { return HasFlag1(ITEMFLAG1_WORLD_LOCK); }

    bool NeverDropsSeed() const { return HasFlag1(ITEMFLAG1_SEEDLESS); }
    bool NeverDropsSelf() const { return HasFlag1(ITEMFLAG1_DROPLESS); }

    bool HasSurprisingFruit() const { return HasFlag2(ITEMFLAG2_RANDGROW); }

    static constexpr uint32_t ExpectedBloomTime(uint16_t rarity) {
        return static_cast<uint32_t>(rarity) * rarity * rarity + 30u * rarity;
    }

    std::size_t GetMemoryUsage() const {
        std::size_t ret{ 4 + 2 + 1 + 1 };
        ret += sizeof(uint16_t) + m_name.size();
        ret += sizeof(uint16_t) + m_textureFile.size();
        ret += 4 + 1 + 4;
        ret += 1 + 1 + 1 + 1 + 1 + 1;
        ret += 4 + 1 + 2 + 1;
        ret += sizeof(uint16_t) + m_extraFile.size();
        ret += 4 + 4;
        ret += 1 + 1 + 1 + 1;
        ret += 4 + 4;
        ret += 2 + 2 + 4;
        return ret;
    }
    void Pack(BinaryWriter& buffer) {
        buffer.Write<uint32_t>(m_Id);
        buffer.Write<uint16_t>(m_properties);
        buffer.Write<uint8_t>(m_itemType);
        buffer.Write<uint8_t>(m_material);

        buffer.Write(this->Cypher(m_name));
        buffer.Write(m_textureFile, sizeof(uint16_t));

        buffer.Write<uint32_t>(m_textureFileHash);
        buffer.Write<uint8_t>(m_visualEffect);
        buffer.Write<uint32_t>(m_cookTime);

        buffer.Write<uint8_t>(m_texture.X);
        buffer.Write<uint8_t>(m_texture.Y);
        buffer.Write<uint8_t>(m_spreadType);
        buffer.Write<uint8_t>(m_layer);
        buffer.Write<uint8_t>(m_collisionType);
        buffer.Write<uint8_t>(m_breakHits);

        buffer.Write<uint32_t>(m_growTime);
        buffer.Write<uint8_t>(m_clothingType);
        buffer.Write<uint16_t>(m_rarity);
        buffer.Write<uint8_t>(m_maxAmount);

        buffer.Write(m_extraFile, sizeof(uint16_t));
        buffer.Write<uint32_t>(m_extraFileHash);
        buffer.Write<uint32_t>(m_animMs);

        buffer.Write<uint8_t>(m_seed.m_base);
        buffer.Write<uint8_t>(m_seed.m_overlay);
        buffer.Write<uint8_t>(m_tree.m_base);
        buffer.Write<uint8_t>(m_tree.m_leaves);
        buffer.Write<uint32_t>(m_seed.m_color);
        buffer.Write<uint32_t>(m_seed.m_overlayColor);

        buffer.Write<uint16_t>(m_seed1);
        buffer.Write<uint16_t>(m_seed2);
        buffer.Write<uint32_t>(m_bloomTime);
    }
    void Serialize(BinaryReader& br) {
        m_Id = br.Read<uint32_t>();
        m_properties = br.Read<uint16_t>();
        m_itemType = br.Read<uint8_t>();
        m_material = br.Read<uint8_t>();

        m_name = this->Cypher(br.ReadStringU16());
        m_textureFile = br.ReadStringU16();

        m_textureFileHash = br.Read<uint32_t>();
        m_visualEffect = br.Read<uint8_t>();
        m_cookTime = br.Read<uint32_t>();

        m_texture = CL_Vec2<uint8_t>{ br.Read<uint8_t>(), br.Read<uint8_t>() };
        m_spreadType = br.Read<uint8_t>();
        m_layer = br.Read<uint8_t>();
        m_collisionType = br.Read<uint8_t>();
        m_breakHits = br.Read<uint8_t>();

        m_growTime = br.Read<uint32_t>();
        m_clothingType = br.Read<uint8_t>();
        m_rarity = br.Read<uint16_t>();
        m_maxAmount = br.Read<uint8_t>();

        m_extraFile = br.ReadStringU16();
        m_extraFileHash = br.Read<uint32_t>();
        m_animMs = br.Read<uint32_t>();

        m_seed.m_base = br.Read<uint8_t>();
        m_seed.m_overlay = br.Read<uint8_t>();
        m_tree.m_base = br.Read<uint8_t>();
        m_tree.m_leaves = br.Read<uint8_t>();
        m_seed.m_color = br.Read<uint32_t>();
        m_seed.m_overlayColor = br.Read<uint32_t>();

        m_seed1 = br.Read<uint16_t>();
        m_seed2 = br.Read<uint16_t>();
        m_bloomTime = br.Read<uint32_t>();

        switch (m_spreadType) {
        case TILESPREAD_DIRECT8:
        case TILESPREAD_DIRECT4: {
            m_defaultTexture = CL_Vec2<uint8_t>(m_texture.X + 4, m_texture.Y + 1);
        } break;
        case TILESPREAD_HORIZONTAL:
        case TILESPREAD_VERTICAL: {
            m_defaultTexture = CL_Vec2<uint8_t>(m_texture.X + 3, m_texture.Y);
        } break;
        default: {
            m_defaultTexture = CL_Vec2<uint8_t>(m_texture.X, m_texture.Y);
        } break;
        }
    }
};
