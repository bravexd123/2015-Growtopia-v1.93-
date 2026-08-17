#include <Player/Player.hpp>
#include <algorithm>
#include <fmt/core.h>
#include <ENetWrapper/ENetWrapper.hpp>
#include <Packet/PacketFactory.hpp>
#include <Packet/VariantFunction.hpp>
#include <Logger/Logger.hpp>
#include <World/World.hpp>
#include <Manager/Database/Database.hpp>
#include <Packet/TextFunction.hpp>
#include <Commands/CommandUtils.hpp>

Player::Player(ENetPeer* peer) : Peer(peer),
    PlayerDialog(peer),
    m_detail(), m_items(peer),
    m_userId(0), m_flags(0),
    m_discordClientId(0) {

}
Player::~Player() {

}

void Player::AddXp(uint32_t amount) {
    if (amount == 0 || m_level >= kMaxLevel)
        return;

    m_xp += amount;
    bool levelled = false;
    while (m_level < kMaxLevel && m_xp >= XpForNextLevel(m_level)) {
        m_xp -= XpForNextLevel(m_level);
        m_level++;
        levelled = true;

        VarList::OnConsoleMessage(this->Get(), fmt::format("`5You are now level `w{}``!", m_level));
        VarList::OnTalkBubble(this->Get(), m_netId, fmt::format("`5Level `w{}``!", m_level));
    }
    if (m_level >= kMaxLevel)
        m_xp = 0;

    if (levelled) {

        VarList::OnCountryState(this->Get(), m_netId, m_level >= kMaxLevel ? "|maxLevel" : "");
        GetDatabase()->GetPlayerTable()->Save(this);
    }
}

bool Player::IsFlagOn(ePlayerFlags flag) const {
    if (m_flags & flag)
        return true;
    return false;
}
void Player::SetFlag(ePlayerFlags flag) {
    m_flags |= flag;
}
void Player::RemoveFlag(ePlayerFlags flag) {
    m_flags &= ~flag;
}

uint32_t Player::GetUserId() const {
    return m_userId;
}
void Player::SetUserId(const uint32_t& userId) {
    m_userId = userId;
}

std::string Player::GetRawName() const {
    return m_rawName;
}
void Player::SetRawName(const std::string& name) {
    m_rawName = name;
}

std::string Player::GetDisplayName() const {
    return m_displayName;
}
void Player::SetDisplayName(const std::string& name) {
    m_displayName = name;
}

std::string Player::GetEmailAddress() const {
    return m_emailAddress;
}
void Player::SetEmailAddress(const std::string& email) {
    m_emailAddress = email;
}

uint64_t Player::GetDiscordClientId() const {
    return m_discordClientId;
}
void Player::SetDiscordClientId(uint64_t discordClientId) {
    m_discordClientId = discordClientId;
}

uint32_t Player::GetFlags() const {
    return m_flags;
}
void Player::SetFlags(uint32_t flags) {
    m_flags = flags;
}

std::string Player::GetSpawnData(bool local) const {
    return fmt::format(
        "spawn|avatar\n"
        "netID|{}\n"
        "userID|{}\n"
        "colrect|0|0|20|30\n"
        "posXY|{}|{}\n"
        "name|{}\n"
        "country|{}\n"
        "invis|{}\n"
        "mstate|{}\n"
        "smstate|{}\n"
        "onlineID|\n"
        "{}",
        m_netId, m_userId,
        static_cast<int>(m_x), static_cast<int>(m_y),
        this->GetFormattedName(), m_detail.GetCountryCode(),
        this->IsFlagOn(PLAYERFLAG_IS_INVISIBLE) ? 1 : 0,
        (m_role != PlayerRole::Default || this->IsFlagOn(PLAYERFLAG_IS_MOD)) ? 1 : 0,
        (m_role == PlayerRole::Developer || this->IsFlagOn(PLAYERFLAG_IS_SUPER_MOD)) ? 1 : 0,
        local ? "type|local\n" : "");
}

AccountTier Player::GetAccountTier() const {
    return m_accountTier;
}
void Player::SetAccountTier(AccountTier tier) {
    m_accountTier = tier;
}

PlayerRole Player::GetRole() const {
    return m_role;
}
void Player::SetRole(PlayerRole role) {
    m_role = role;
}

std::string Player::GetFormattedName() const {

    if (!m_nickname.empty())
        return m_nickname;

    if (m_role == PlayerRole::Developer)
        return fmt::format("`6@{}``", m_rawName);
    if (m_role == PlayerRole::Moderator)
        return fmt::format("`5@{}``", m_rawName);

    bool isWorldLockOwner = false;
    bool hasWorldLockAccess = false;
    if (m_world) {
        const LockInfo* pWorldLock = m_world->GetWorldLock();
        if (pWorldLock) {
            isWorldLockOwner = pWorldLock->m_ownerId == m_userId;
            hasWorldLockAccess = std::find(pWorldLock->m_accessUserIds.begin(), pWorldLock->m_accessUserIds.end(), m_userId) != pWorldLock->m_accessUserIds.end();
        }
    }
    bool isSupporterTier = m_accountTier != AccountTier::Default;
    std::string color = (isWorldLockOwner || isSupporterTier) ? "`2" : (hasWorldLockAccess ? "`^" : "`w");
    return fmt::format("{}{}``", color, m_rawName);
}

TankInfo& Player::GetDetail() {
    return m_detail;
}
PlayerItems* Player::GetItems() {
    return &m_items;
}

std::shared_ptr<World> Player::GetWorld() const {
    return m_world;
}
void Player::SetWorld(std::shared_ptr<World> pWorld) {
    m_world = pWorld;
}

int32_t Player::GetNetId() const {
    return m_netId;
}
void Player::SetNetId(int32_t netId) {
    m_netId = netId;
}

float Player::GetX() const {
    return m_x;
}
float Player::GetY() const {
    return m_y;
}
void Player::SetPosition(float x, float y) {
    m_x = x;
    m_y = y;
}

bool Player::IsFacingLeft() const {
    return m_facingLeft;
}
void Player::SetFacingLeft(bool facingLeft) {
    m_facingLeft = facingLeft;
}

void Player::OnConnect() {
    Logger::Print(INFO, "A player connected with IP {}, connectId {} and {} pings.", this->GetIp(), this->GetConnectId(), this->GetPing());

    auto packet = SLoginInformationRequestPacket();
    ENetWrapper::SendPacket(this->Get(), packet);
}
void Player::OnDisconnect() {
    Logger::Print(INFO, "A player disconnected with IP {}, connectId {} and {} pings.", this->GetIp(), this->GetConnectId(), this->GetPing());

    if (m_detail.IsFlagOn(CLIENTFLAG_LOGGED_ON) && !this->IsFlagOn(PLAYERFLAG_IS_INVISIBLE)) {
        for (uint32_t friendId : m_friends) {
            Player* pFriend = CommandUtils::FindOnlinePlayer(std::to_string(friendId));
            if (!pFriend || !pFriend->GetShowFriendNotifications())
                continue;
            VarList::OnConsoleMessage(pFriend->Get(), fmt::format("`3FRIEND ALERT:`` {} has `4logged off``.", this->GetRawName()));
            CAction::PlaySFX(pFriend->Get(), "friend_logoff", 0);
        }
    }

    if (m_detail.IsFlagOn(CLIENTFLAG_LOGGED_ON))
        GetDatabase()->GetPlayerTable()->Save(this);

    if (m_world) {

        m_world->BroadcastPlayerLeft(this);

        m_world->ReleaseNetId(m_netId);
        m_world->RemovePlayer(this);
        m_world = nullptr;
        m_netId = -1;
        this->GetDetail().RemoveFlag(CLIENTFLAG_IS_IN);
    }
}
