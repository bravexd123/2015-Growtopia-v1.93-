#pragma once
#include <string>
#include <filesystem>
#include <Player/Player.hpp>

struct PlayerRegistration {
public:
    enum class Result {
        SUCCESS,
        EXIST_GROWID,
        INVALID_GROWID,
        INVALID_PASSWORD,
        INVALID_EMAIL_OR_DISCORD,
        INVALID_GROWID_LENGTH,
        INVALID_PASSWORD_LENGTH,
        MISMATCH_VERIFY_PASSWORD,
        BAD_CONNECTION
    };

public:
    PlayerRegistration::Result  m_result;
    std::string                 m_data;
};

class JsonPlayerTable {
public:
    explicit JsonPlayerTable(std::filesystem::path directory);
    ~JsonPlayerTable() = default;

public:
    bool IsAccountExist(const std::string& name) const;

    uint32_t Insert(Player* pAvatar, uint32_t preferredId = 0);

    bool Save(Player* pAvatar);

    bool Load(const std::string& name, Player* pAvatar);

    bool IsGuestExist(const std::string& rid) const;
    bool LoadGuest(const std::string& rid, Player* pAvatar);

    void DeleteGuest(const std::string& rid) const;

    PlayerRegistration RegisterPlayer(const std::string& name, const std::string& password, const std::string& verifyPassword, const std::string& email);

    bool SetRoleByName(const std::string& name, PlayerRole role) const;

    struct AccountSummary {
        bool found = false;
        uint32_t userId = 0;
        std::string rawName;
        std::string emailAddress;
        int64_t createdAt = 0;
        std::string lastIp;
        int64_t bannedUntil = 0;
        std::string banReason;
        std::vector<Player::PlayerNote> notes;
        PlayerRole role = PlayerRole::Default;
    };

    AccountSummary FindAccountByQuery(const std::string& query) const;

    std::vector<std::string> FindAccountsByIp(const std::string& ip, std::size_t limit) const;

    std::string SetBanByQuery(const std::string& query, int64_t bannedUntil, const std::string& reason) const;

    std::string AddNoteByQuery(const std::string& query, const std::string& authorName, const std::string& text) const;

private:

    struct SaveKey {
        std::string value;
        bool isGuest = false;
        bool IsValid() const { return !value.empty(); }
    };

    std::filesystem::path PathFor(const SaveKey& key) const;
    uint32_t AllocateNextId() const;
    void WriteToDisk(Player* pAvatar, uint32_t id, const SaveKey& key) const;
    bool LoadByKey(const SaveKey& key, Player* pAvatar);
    static SaveKey KeyFor(Player* pAvatar);
    static std::string ToLower(std::string value);

private:
    std::filesystem::path m_directory;
};
