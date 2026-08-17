#pragma once
#include <algorithm>
#include <memory>
#include <chrono>
#include <ctime>
#include <deque>
#include <string>
#include <unordered_map>
#include <vector>
#include <enet/enet.h>
#include <ENetWrapper/Peer.hpp>
#include <Protocol/TankInfo.hpp>
#include <Player/PlayerDialog/PlayerDialog.hpp>
#include <Player/PlayerItems.hpp>
#include <Utils/TextParse.hpp>

class World;

enum class AccountTier : uint8_t {
    Default = 0,
    Supporter = 1,
    SuperSupporter = 2,
};

enum class PlayerRole : uint8_t {
    Default = 0,
    Moderator = 1,
    Developer = 2,
};

class Player : public Peer,
    public PlayerDialog {
public:
    explicit Player(ENetPeer* pPeer);
    ~Player();

    operator ENetPeer*() {
        return this->Get();
    }
public:
    bool IsFlagOn(ePlayerFlags flag) const;
    void SetFlag(ePlayerFlags flag);
    void RemoveFlag(ePlayerFlags flag);

    uint32_t GetUserId() const;
    void SetUserId(const uint32_t& userId);

    std::string GetRawName() const;
    void SetRawName(const std::string& name);

    std::string GetDisplayName() const;
    void SetDisplayName(const std::string& name);

    std::string GetEmailAddress() const;
    void SetEmailAddress(const std::string& email);

    uint64_t GetDiscordClientId() const;
    void SetDiscordClientId(uint64_t discordClientId);

    uint32_t GetFlags() const;
    void SetFlags(uint32_t flags);

    AccountTier GetAccountTier() const;
    void SetAccountTier(AccountTier tier);

    PlayerRole GetRole() const;
    void SetRole(PlayerRole role);

    std::string GetFormattedName() const;

    std::string GetSpawnData(bool local) const;

    const std::string& GetNickname() const { return m_nickname; }
    void SetNickname(const std::string& nickname) { m_nickname = nickname; }
    void ClearNickname() { m_nickname.clear(); }

    bool IsOneHitEnabled() const { return m_oneHitMode; }
    void SetOneHitEnabled(bool enabled) { m_oneHitMode = enabled; }

    bool IsGhostEnabled() const { return m_ghostMode; }
    void SetGhostEnabled(bool enabled) { m_ghostMode = enabled; }

    int64_t GetBannedUntil() const { return m_bannedUntil; }
    const std::string& GetBanReason() const { return m_banReason; }
    bool IsBanned() const { return m_bannedUntil > std::time(nullptr); }
    void SetBan(int64_t untilUnixTime, const std::string& reason) {
        m_bannedUntil = untilUnixTime;
        m_banReason = reason;
    }
    void ClearBan() { m_bannedUntil = 0; m_banReason.clear(); }

    bool IsSilenced() const { return m_silencedUntil > std::time(nullptr); }
    int64_t GetSilencedUntil() const { return m_silencedUntil; }
    void SetSilencedUntil(int64_t untilUnixTime) { m_silencedUntil = untilUnixTime; }

    struct PlayerNote { std::string author; int64_t time; std::string text; };
    const std::vector<PlayerNote>& GetNotes() const { return m_notes; }
    void AddNote(const std::string& author, const std::string& text) {
        m_notes.push_back({ author, std::time(nullptr), text });
    }
    void SetNotes(std::vector<PlayerNote> notes) { m_notes = std::move(notes); }

    const std::string& GetLastIp() const { return m_lastIp; }
    void SetLastIp(const std::string& ip) { m_lastIp = ip; }

    bool IsNameLocked() const { return m_nameLocked; }
    void SetNameLocked(bool locked) { m_nameLocked = locked; }

    struct ActivePlaymod {
        uint16_t m_id;
        int64_t m_expiry;
        std::string m_issuer;
    };

    const std::vector<ActivePlaymod>& GetPlaymods() const { return m_playmods; }

    uint16_t GetPendingBlastItem() const { return m_pendingBlastItem; }
    void SetPendingBlastItem(uint16_t itemId) { m_pendingBlastItem = itemId; }

    bool HasPlaymod(uint16_t playmodId) const {
        int64_t now = std::time(nullptr);
        for (const auto& mod : m_playmods)
            if (mod.m_id == playmodId && mod.m_expiry > now)
                return true;
        return false;
    }

    int64_t GetPlaymodRemaining(uint16_t playmodId) const {
        int64_t now = std::time(nullptr);
        for (const auto& mod : m_playmods)
            if (mod.m_id == playmodId && mod.m_expiry > now)
                return mod.m_expiry - now;
        return 0;
    }

    bool AddPlaymod(uint16_t playmodId, int64_t durationSeconds, const std::string& issuer = std::string()) {
        if (HasPlaymod(playmodId))
            return false;

        RemovePlaymod(playmodId);
        m_playmods.push_back({ playmodId, std::time(nullptr) + durationSeconds, issuer });
        return true;
    }
    void RemovePlaymod(uint16_t playmodId) {
        m_playmods.erase(std::remove_if(m_playmods.begin(), m_playmods.end(),
            [playmodId](const ActivePlaymod& mod) { return mod.m_id == playmodId; }), m_playmods.end());
    }

    void PrunePlaymods() {
        int64_t now = std::time(nullptr);
        m_playmods.erase(std::remove_if(m_playmods.begin(), m_playmods.end(),
            [now](const ActivePlaymod& mod) { return mod.m_expiry <= now; }), m_playmods.end());
    }

    void AgePlaymods(int64_t seconds) {
        int64_t now = std::time(nullptr);
        for (auto& mod : m_playmods) {
            if (mod.m_expiry > now)
                mod.m_expiry -= seconds;
        }
    }

    bool HasCheckpoint() const { return m_hasCheckpoint; }
    uint32_t GetCheckpointX() const { return m_checkpointX; }
    uint32_t GetCheckpointY() const { return m_checkpointY; }
    void SetCheckpoint(uint32_t tileX, uint32_t tileY) {
        m_hasCheckpoint = true;
        m_checkpointX = tileX;
        m_checkpointY = tileY;
    }
    void ClearCheckpoint() { m_hasCheckpoint = false; m_checkpointX = 0; m_checkpointY = 0; }

    static constexpr std::size_t kMaxFriends = 25;
    const std::vector<uint32_t>& GetFriends() const { return m_friends; }
    void SetFriends(std::vector<uint32_t> friends) { m_friends = std::move(friends); }
    bool IsFriend(uint32_t userId) const {
        return std::find(m_friends.begin(), m_friends.end(), userId) != m_friends.end();
    }
    bool AddFriend(uint32_t userId) {
        if (IsFriend(userId))
            return true;
        if (m_friends.size() >= kMaxFriends)
            return false;
        m_friends.push_back(userId);
        return true;
    }
    void RemoveFriend(uint32_t userId) {
        m_friends.erase(std::remove(m_friends.begin(), m_friends.end(), userId), m_friends.end());
    }

    const std::vector<uint32_t>& GetSentFriendRequests() const { return m_sentFriendRequests; }
    void SetSentFriendRequests(std::vector<uint32_t> requests) { m_sentFriendRequests = std::move(requests); }
    bool HasSentRequestTo(uint32_t userId) const {
        return std::find(m_sentFriendRequests.begin(), m_sentFriendRequests.end(), userId) != m_sentFriendRequests.end();
    }
    void AddSentRequest(uint32_t userId) {
        if (!HasSentRequestTo(userId))
            m_sentFriendRequests.push_back(userId);
    }
    void ClearSentRequest(uint32_t userId) {
        m_sentFriendRequests.erase(std::remove(m_sentFriendRequests.begin(), m_sentFriendRequests.end(), userId), m_sentFriendRequests.end());
    }

    bool GetShowLocationToFriends() const { return m_showLocationToFriends; }
    void SetShowLocationToFriends(bool value) { m_showLocationToFriends = value; }
    bool GetShowFriendNotifications() const { return m_showFriendNotifications; }
    void SetShowFriendNotifications(bool value) { m_showFriendNotifications = value; }

    uint32_t GetFriendDialogConfirmingRemoval() const { return m_friendDialogConfirmingRemoval; }
    void SetFriendDialogConfirmingRemoval(uint32_t userId) { m_friendDialogConfirmingRemoval = userId; }

    static constexpr int16_t kMaxHealth = 100;
    static constexpr int16_t kLavaDamage = 2;
    int16_t GetHealth() const { return m_health; }
    void SetHealth(int16_t hp) { m_health = hp; }
    void ResetHealth() { m_health = kMaxHealth; }

    bool ApplyDamage(int16_t amount) {
        m_health = static_cast<int16_t>(m_health - amount);
        if (m_health > 0)
            return false;
        m_health = kMaxHealth;
        return true;
    }

    static constexpr std::size_t kMaxRecentWorlds = 6;
    const std::vector<std::string>& GetRecentWorlds() const { return m_recentWorlds; }
    void SetRecentWorlds(std::vector<std::string> worlds) {
        m_recentWorlds = std::move(worlds);
        if (m_recentWorlds.size() > kMaxRecentWorlds)
            m_recentWorlds.resize(kMaxRecentWorlds);
    }
    void PushRecentWorld(const std::string& name) {
        auto it = std::find(m_recentWorlds.begin(), m_recentWorlds.end(), name);
        if (it != m_recentWorlds.end())
            m_recentWorlds.erase(it);
        m_recentWorlds.insert(m_recentWorlds.begin(), name);
        if (m_recentWorlds.size() > kMaxRecentWorlds)
            m_recentWorlds.resize(kMaxRecentWorlds);
    }

    std::vector<uint16_t>& GetFavouriteItems() { return m_favouriteItems; }

    bool RegisterChatMessageAndCheckSpam() {
        int64_t now = std::time(nullptr);
        m_recentMessages.push_back(now);
        if (m_recentMessages.size() > 5)
            m_recentMessages.pop_front();
        return m_recentMessages.size() == 5 && (now - m_recentMessages.front()) < 6;
    }

    bool IsDuctTaped() const { return m_ductTapedUntil > std::time(nullptr); }
    void SetDuctTapedFor(int64_t seconds) { m_ductTapedUntil = std::time(nullptr) + seconds; }
    void ClearDuctTape() { m_ductTapedUntil = 0; }

    static constexpr uint16_t kMaxLevel = 125;
    static constexpr uint32_t XpForNextLevel(uint16_t level) {
        return 50u * (static_cast<uint32_t>(level) * level + 2u);
    }

    uint16_t GetLevel() const { return m_level; }
    uint32_t GetXp() const { return m_xp; }
    void SetLevel(uint16_t level) { m_level = level; }
    void SetXp(uint32_t xp) { m_xp = xp; }

    void AddXp(uint32_t amount);

    int64_t GetCreatedAt() const { return m_createdAt; }
    void SetCreatedAt(int64_t unixTime) { m_createdAt = unixTime; }

    int32_t GetTradingWithNetId() const { return m_tradingWithNetId; }
    void SetTradingWithNetId(int32_t netId) { m_tradingWithNetId = netId; }

    std::vector<std::pair<uint16_t, uint8_t>>& GetTradeOffer() { return m_tradeOffer; }
    void ClearTradeOffer() { m_tradeOffer.clear(); }

    bool HasAcceptedTrade() const { return m_tradeAccepted; }
    void SetTradeAccepted(bool accepted) { m_tradeAccepted = accepted; }

    bool HasConfirmedTrade() const { return m_tradeConfirmed; }
    void SetTradeConfirmed(bool confirmed) { m_tradeConfirmed = confirmed; }

public:
    TankInfo& GetDetail();
    PlayerItems* GetItems();

public:
    std::shared_ptr<World> GetWorld() const;
    void SetWorld(std::shared_ptr<World> pWorld);

    int32_t GetNetId() const;
    void SetNetId(int32_t netId);

    float GetX() const;
    float GetY() const;
    void SetPosition(float x, float y);

    bool IsFacingLeft() const;
    void SetFacingLeft(bool facingLeft);

public:
    void OnConnect();
    void OnDisconnect();

private:
    TankInfo m_detail;
    PlayerItems m_items;

private:
    uint32_t m_userId;
    uint32_t m_flags;
    AccountTier m_accountTier = AccountTier::Default;
    PlayerRole m_role = PlayerRole::Default;
    bool m_oneHitMode = false;
    bool m_ghostMode = false;
    int64_t m_bannedUntil = 0;
    std::string m_banReason;
    int64_t m_silencedUntil = 0;
    std::vector<PlayerNote> m_notes;
    std::string m_lastIp;
    bool m_nameLocked = false;
    std::vector<ActivePlaymod> m_playmods;
    uint16_t m_pendingBlastItem = 0;
    bool m_hasCheckpoint = false;
    uint32_t m_checkpointX = 0, m_checkpointY = 0;
    int64_t m_createdAt = 0;
    std::string m_nickname;
    uint16_t m_level = 1;
    uint32_t m_xp = 0;
    std::deque<int64_t> m_recentMessages;
    int64_t m_ductTapedUntil = 0;
    std::vector<uint16_t> m_favouriteItems;
    std::vector<std::string> m_recentWorlds;
    int16_t m_health = kMaxHealth;
    std::vector<uint32_t> m_friends;
    std::vector<uint32_t> m_sentFriendRequests;
    bool m_showLocationToFriends = true;
    bool m_showFriendNotifications = true;
    uint32_t m_friendDialogConfirmingRemoval = 0;

    int32_t m_tradingWithNetId = -1;
    std::vector<std::pair<uint16_t, uint8_t>> m_tradeOffer;
    bool m_tradeAccepted = false;
    bool m_tradeConfirmed = false;

    std::string m_rawName;
    std::string m_displayName;

    std::string m_emailAddress;
    uint64_t m_discordClientId;

private:
    std::shared_ptr<World> m_world = nullptr;
    int32_t m_netId = -1;
    float m_x = 0.0f, m_y = 0.0f;
    bool m_facingLeft = false;
};
