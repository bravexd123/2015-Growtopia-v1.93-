#include <Player/PlayerItems.hpp>
#include <algorithm>
#include <Player/Player.hpp>
#include <Manager/Item/ItemComponent.hpp>
#include <Manager/Item/ItemManager.hpp>
#include <Packet/PacketFactory.hpp>
#include <Utils/BinaryWriter.hpp>
#include <ENetWrapper/ENetWrapper.hpp>

PlayerItems::PlayerItems(ENetPeer* pPeer) : m_pPeer(pPeer) {
    for (auto index = 0; index < NUM_BODY_PARTS; index++)
        m_clothes[index] = ITEM_BLANK;

    m_skinColor = Color(0xC3, 0x95, 0x82, 0xFF);
    m_gems = 0;

    m_backpackSpace = 20;

    this->AddItem(ITEM_FIST, 1);
    this->AddItem(ITEM_WRENCH, 1);
}

int32_t PlayerItems::GetGems() const {
    return m_gems;
}
void PlayerItems::SetGems(int32_t gems) {
    m_gems = gems;
}

Color PlayerItems::GetSkinColor() const {
    return m_skinColor;
}

uint16_t& PlayerItems::GetCloth(eClothTypes bodyPart) {
    return m_clothes[bodyPart];
}
std::array<uint16_t, NUM_BODY_PARTS>& PlayerItems::GetClothes() {
    return m_clothes;
}
void PlayerItems::SetCloth(eClothTypes bodyPart, uint16_t itemId) {
    if (static_cast<int32_t>(bodyPart) < ITEM_BLANK || static_cast<int32_t>(bodyPart) > NUM_BODY_PARTS)
        return;
    m_clothes[bodyPart] = itemId;
}

static constexpr uint8_t kDefaultMaxStack = 200;

uint8_t PlayerItems::AddItemPartial(uint16_t itemId, uint8_t count) {
    if (count < 1)
        return 0;
    auto* pItem = GetItemManager()->GetItem(itemId);
    if (!pItem)
        return 0;

    uint8_t cap = pItem->m_maxAmount > 0 ? pItem->m_maxAmount : kDefaultMaxStack;

    auto it = m_bpItems.find(itemId);
    if (it == m_bpItems.end()) {
        if (m_bpItems.size() >= m_backpackSpace)
            return 0;
        uint8_t added = std::min<uint8_t>(count, cap);
        m_bpItems.insert_or_assign(itemId, added);
        return added;
    }

    if (it->second >= cap)
        return 0;

    uint8_t room = static_cast<uint8_t>(cap - it->second);
    uint8_t added = std::min<uint8_t>(count, room);
    it->second = static_cast<uint8_t>(it->second + added);
    return added;
}

bool PlayerItems::AddItem(uint16_t itemId, uint8_t count, bool sendPacket) {
    return this->AddItemPartial(itemId, count) > 0;
}

void PlayerItems::SendInventoryState(Player* pAvatar) {
    auto* pItems = pAvatar->GetItems();
    BinaryWriter iw(9 + pItems->m_bpItems.size() * 4);
    iw.Write<uint8_t>(0);
    iw.Write<uint32_t>(pItems->m_backpackSpace);
    iw.Write<uint8_t>(static_cast<uint8_t>(pItems->m_bpItems.size()));
    for (const auto& [itemId, count] : pItems->m_bpItems) {
        iw.Write<uint16_t>(itemId);
        iw.Write<uint8_t>(count);
        iw.Write<uint8_t>(0);
    }
    std::vector<uint8_t> invData(iw.Get(), iw.Get() + iw.GetPosition());

    TankPacketData t{};
    t.m_type = NET_GAME_PACKET_SEND_INVENTORY_STATE;
    t.m_netId = 0;

    SExtendedTankPacket packet(t, invData);
    ENetWrapper::SendPacket(pAvatar->Get(), packet);
}

bool PlayerItems::RemoveItem(uint16_t itemId, uint8_t count) {
    if (count < 1)
        return false;
    auto it = m_bpItems.find(itemId);
    if (it == m_bpItems.end() || it->second < count)
        return false;

    it->second -= count;
    if (it->second == 0)
        m_bpItems.erase(it);
    return true;
}
