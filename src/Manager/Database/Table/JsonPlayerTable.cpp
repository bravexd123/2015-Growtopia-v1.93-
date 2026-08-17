#include <Manager/Database/Table/JsonPlayerTable.hpp>
#include <fstream>
#include <algorithm>
#include <cctype>
#include <ctime>
#include <string_view>
#include <fmt/format.h>
#include <nlohmann/json.hpp>
#include <Manager/Item/ItemManager.hpp>
#include <Manager/Item/ItemInfo.hpp>
#include <Utils/MiscUtils.hpp>
#include <Logger/Logger.hpp>

static const char* const kClothSlotNames[NUM_BODY_PARTS] = {
    "hair", "shirt", "pants", "feet", "face", "hand", "back", "mask", "necklace", "ances"
};

JsonPlayerTable::JsonPlayerTable(std::filesystem::path directory) : m_directory(std::move(directory)) {}

std::string JsonPlayerTable::ToLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) { return std::tolower(c); });
    return value;
}

std::filesystem::path JsonPlayerTable::PathFor(const SaveKey& key) const {
    if (key.isGuest)
        return m_directory / "guests" / (key.value + ".json");
    return m_directory / (key.value + ".json");
}

JsonPlayerTable::SaveKey JsonPlayerTable::KeyFor(Player* pAvatar) {
    auto& det = pAvatar->GetDetail();
    if (!det.GetTankIDName().empty())
        return SaveKey{ ToLower(det.GetTankIDName()), false };
    if (!det.GetRelativeId().empty())
        return SaveKey{ det.GetRelativeId(), true };
    return SaveKey{};
}

bool JsonPlayerTable::IsAccountExist(const std::string& name) const {
    return std::filesystem::exists(this->PathFor(SaveKey{ ToLower(name), false }));
}

bool JsonPlayerTable::IsGuestExist(const std::string& rid) const {
    return std::filesystem::exists(this->PathFor(SaveKey{ rid, true }));
}

void JsonPlayerTable::DeleteGuest(const std::string& rid) const {
    std::error_code ec;
    std::filesystem::remove(this->PathFor(SaveKey{ rid, true }), ec);
}

uint32_t JsonPlayerTable::AllocateNextId() const {
    uint32_t maxId = 0;
    std::error_code ec;

    for (const auto& dir : { m_directory, m_directory / "guests" }) {
        for (const auto& entry : std::filesystem::directory_iterator(dir, ec)) {
            if (!entry.is_regular_file() || entry.path().extension() != ".json")
                continue;
            std::ifstream file(entry.path());
            if (!file.is_open())
                continue;
            try {
                nlohmann::json j;
                file >> j;
                if (j.contains("id") && j["id"].is_number_integer())
                    maxId = std::max(maxId, j["id"].get<uint32_t>());
            }
            catch (const nlohmann::json::exception&) {
                continue;
            }
        }
    }
    return maxId + 1;
}

void JsonPlayerTable::WriteToDisk(Player* pAvatar, uint32_t id, const SaveKey& key) const {
    auto& det = pAvatar->GetDetail();
    auto* pItems = pAvatar->GetItems();

    nlohmann::json j;
    j["id"] = id;
    j["requestedName"] = det.GetRequestedName();
    j["tankIdName"] = det.GetTankIDName();
    j["tankIdPass"] = det.GetTankIDPass();
    j["rawName"] = pAvatar->GetRawName();
    j["displayName"] = pAvatar->GetDisplayName();
    j["emailAddress"] = pAvatar->GetEmailAddress();
    j["discordClientId"] = pAvatar->GetDiscordClientId();
    j["gems"] = pItems->GetGems();
    j["playerFlags"] = pAvatar->GetFlags();
    j["accountTier"] = static_cast<uint8_t>(pAvatar->GetAccountTier());
    j["role"] = static_cast<uint8_t>(pAvatar->GetRole());
    j["createdAt"] = pAvatar->GetCreatedAt();
    j["level"] = pAvatar->GetLevel();
    j["xp"] = pAvatar->GetXp();
    j["recentWorlds"] = pAvatar->GetRecentWorlds();
    j["friends"] = pAvatar->GetFriends();
    j["sentFriendRequests"] = pAvatar->GetSentFriendRequests();
    j["showLocationToFriends"] = pAvatar->GetShowLocationToFriends();
    j["showFriendNotifications"] = pAvatar->GetShowFriendNotifications();
    j["bannedUntil"] = pAvatar->GetBannedUntil();
    j["banReason"] = pAvatar->GetBanReason();
    j["silencedUntil"] = pAvatar->GetSilencedUntil();
    j["lastIp"] = pAvatar->GetLastIp();
    j["nameLocked"] = pAvatar->IsNameLocked();
    nlohmann::json notesJson = nlohmann::json::array();
    for (const auto& note : pAvatar->GetNotes()) {
        nlohmann::json n;
        n["author"] = note.author;
        n["time"] = note.time;
        n["text"] = note.text;
        notesJson.push_back(std::move(n));
    }
    j["notes"] = notesJson;

    nlohmann::json itemsJson;
    itemsJson["backpackSpace"] = pItems->m_backpackSpace;
    itemsJson["skinColor"] = fmt::format("{:08X}", pItems->GetSkinColor().GetInt());

    nlohmann::json clothesJson;
    auto& clothes = pItems->GetClothes();
    for (size_t i = 0; i < NUM_BODY_PARTS; i++)
        clothesJson[kClothSlotNames[i]] = clothes[i];
    itemsJson["clothes"] = clothesJson;

    nlohmann::json inventoryJson = nlohmann::json::object();
    for (const auto& [itemId, count] : pItems->m_bpItems)
        inventoryJson[std::to_string(itemId)] = count;
    itemsJson["inventory"] = inventoryJson;

    j["items"] = itemsJson;
    j["playMods"] = nlohmann::json::array();
    j["characterState"] = nlohmann::json::object();

    std::ofstream file(this->PathFor(key));
    file << j.dump(4);
}

uint32_t JsonPlayerTable::Insert(Player* pAvatar, uint32_t preferredId) {
    auto& det = pAvatar->GetDetail();
    if (this->IsAccountExist(det.GetTankIDName()))
        return 0;

    uint32_t newId = preferredId != 0 ? preferredId : this->AllocateNextId();
    pAvatar->SetUserId(newId);
    this->WriteToDisk(pAvatar, newId, SaveKey{ ToLower(det.GetTankIDName()), false });
    return newId;
}

bool JsonPlayerTable::Save(Player* pAvatar) {
    SaveKey key = KeyFor(pAvatar);
    if (!key.IsValid())
        return false;

    uint32_t id = pAvatar->GetUserId();
    if (id == 0)
        id = this->AllocateNextId();
    pAvatar->SetUserId(id);

    this->WriteToDisk(pAvatar, id, key);
    return true;
}

bool JsonPlayerTable::Load(const std::string& name, Player* pAvatar) {
    return this->LoadByKey(SaveKey{ ToLower(name), false }, pAvatar);
}

bool JsonPlayerTable::LoadGuest(const std::string& rid, Player* pAvatar) {
    return this->LoadByKey(SaveKey{ rid, true }, pAvatar);
}

bool JsonPlayerTable::LoadByKey(const SaveKey& key, Player* pAvatar) {
    auto path = this->PathFor(key);
    if (!std::filesystem::exists(path))
        return false;

    std::ifstream file(path);
    if (!file.is_open())
        return false;

    try {
        nlohmann::json j;
        file >> j;

        auto& det = pAvatar->GetDetail();
        det.SetTankIDName(j.value("tankIdName", std::string{}));
        det.SetTankIDPass(j.value("tankIdPass", std::string{}));

        pAvatar->SetUserId(j.value("id", 0u));
        pAvatar->SetRawName(j.value("rawName", std::string{}));
        pAvatar->SetDisplayName(j.value("displayName", std::string{}));
        pAvatar->SetEmailAddress(j.value("emailAddress", std::string{}));
        pAvatar->SetDiscordClientId(j.value("discordClientId", uint64_t{ 0 }));
        pAvatar->SetFlags(j.value("playerFlags", 0u));

        pAvatar->SetAccountTier(static_cast<AccountTier>(j.value("accountTier", static_cast<uint8_t>(AccountTier::Default))));

        pAvatar->SetRole(static_cast<PlayerRole>(j.value("role", static_cast<uint8_t>(PlayerRole::Default))));

        pAvatar->SetCreatedAt(j.value("createdAt", static_cast<int64_t>(std::time(nullptr))));

        pAvatar->SetLevel(j.value("level", static_cast<uint16_t>(1)));
        pAvatar->SetXp(j.value("xp", static_cast<uint32_t>(0)));
        if (j.contains("recentWorlds")) {
            std::vector<std::string> recent;
            for (const auto& name : j["recentWorlds"])
                recent.push_back(name.get<std::string>());
            pAvatar->SetRecentWorlds(std::move(recent));
        }
        if (j.contains("friends")) {
            std::vector<uint32_t> friends;
            for (const auto& id : j["friends"])
                friends.push_back(id.get<uint32_t>());
            pAvatar->SetFriends(std::move(friends));
        }
        if (j.contains("sentFriendRequests")) {
            std::vector<uint32_t> requests;
            for (const auto& id : j["sentFriendRequests"])
                requests.push_back(id.get<uint32_t>());
            pAvatar->SetSentFriendRequests(std::move(requests));
        }
        pAvatar->SetShowLocationToFriends(j.value("showLocationToFriends", true));
        pAvatar->SetShowFriendNotifications(j.value("showFriendNotifications", true));
        pAvatar->SetBan(j.value("bannedUntil", static_cast<int64_t>(0)), j.value("banReason", std::string{}));
        pAvatar->SetSilencedUntil(j.value("silencedUntil", static_cast<int64_t>(0)));
        pAvatar->SetLastIp(j.value("lastIp", std::string{}));
        pAvatar->SetNameLocked(j.value("nameLocked", false));
        if (j.contains("notes")) {
            std::vector<Player::PlayerNote> notes;
            for (const auto& n : j["notes"]) {
                notes.push_back({ n.value("author", std::string{}), n.value("time", static_cast<int64_t>(0)),
                    n.value("text", std::string{}) });
            }
            pAvatar->SetNotes(std::move(notes));
        }

        auto* pItems = pAvatar->GetItems();
        if (j.contains("items")) {
            const auto& itemsJson = j["items"];
            pItems->SetGems(j.value("gems", 0));
            pItems->m_backpackSpace = itemsJson.value("backpackSpace", 20u);

            if (itemsJson.contains("skinColor")) {
                uint32_t colorInt = std::stoul(itemsJson["skinColor"].get<std::string>(), nullptr, 16);
                pItems->m_skinColor = Color(colorInt);
            }
            if (itemsJson.contains("clothes")) {
                const auto& clothesJson = itemsJson["clothes"];
                for (size_t i = 0; i < NUM_BODY_PARTS; i++)
                    pItems->GetCloth(static_cast<eClothTypes>(i)) = clothesJson.value(kClothSlotNames[i], 0);
            }
            if (itemsJson.contains("inventory")) {
                pItems->m_bpItems.clear();
                for (const auto& [key, value] : itemsJson["inventory"].items())
                    pItems->m_bpItems[static_cast<uint16_t>(std::stoi(key))] = value.get<uint8_t>();
            }
        }
        return true;
    }
    catch (const nlohmann::json::exception& e) {
        Logger::Print(WARNING, "JsonPlayerTable::Load: failed to parse '{}': {}", path.string(), e.what());
        return false;
    }
}

bool JsonPlayerTable::SetRoleByName(const std::string& name, PlayerRole role) const {
    auto path = this->PathFor(SaveKey{ ToLower(name), false });
    if (!std::filesystem::exists(path))
        return false;

    try {
        nlohmann::json j;
        {
            std::ifstream file(path);
            if (!file.is_open())
                return false;
            file >> j;
        }
        j["role"] = static_cast<uint8_t>(role);
        std::ofstream outFile(path);
        if (!outFile.is_open())
            return false;
        outFile << j.dump(4);
        return true;
    }
    catch (const nlohmann::json::exception& e) {
        Logger::Print(WARNING, "JsonPlayerTable::SetRoleByName: failed to parse '{}': {}", path.string(), e.what());
        return false;
    }
}

namespace {
    bool IsAllDigitsLocal(const std::string& s) {
        return !s.empty() && std::all_of(s.begin(), s.end(), [](unsigned char c) { return std::isdigit(c); });
    }
}

JsonPlayerTable::AccountSummary JsonPlayerTable::FindAccountByQuery(const std::string& query) const {
    AccountSummary result;
    if (query.empty() || !std::filesystem::exists(m_directory))
        return result;

    bool queryIsId = IsAllDigitsLocal(query);
    uint32_t queryId = queryIsId ? static_cast<uint32_t>(std::stoul(query)) : 0;
    std::string lowerQuery = ToLower(query);

    std::filesystem::path partialMatchPath;

    for (const auto& entry : std::filesystem::directory_iterator(m_directory)) {
        if (!entry.is_regular_file() || entry.path().extension() != ".json")
            continue;
        std::ifstream file(entry.path());
        if (!file.is_open())
            continue;
        try {
            nlohmann::json j;
            file >> j;
            std::string rawName = j.value("rawName", std::string{});
            uint32_t id = j.value("id", 0u);
            std::string lowerName = ToLower(rawName);

            bool exact = (queryIsId && id == queryId) || (!queryIsId && lowerName == lowerQuery);
            bool partial = !exact && !queryIsId && !lowerName.empty() && lowerName.find(lowerQuery) != std::string::npos;

            if (exact) {
                result.found = true;
                result.userId = id;
                result.rawName = rawName;
                result.emailAddress = j.value("emailAddress", std::string{});
                result.createdAt = j.value("createdAt", static_cast<int64_t>(0));
                result.lastIp = j.value("lastIp", std::string{});
                result.bannedUntil = j.value("bannedUntil", static_cast<int64_t>(0));
                result.banReason = j.value("banReason", std::string{});
                result.role = static_cast<PlayerRole>(j.value("role", static_cast<uint8_t>(PlayerRole::Default)));
                if (j.contains("notes")) {
                    for (const auto& n : j["notes"])
                        result.notes.push_back({ n.value("author", std::string{}), n.value("time", static_cast<int64_t>(0)), n.value("text", std::string{}) });
                }
                return result;
            }
            if (partial && partialMatchPath.empty())
                partialMatchPath = entry.path();
        }
        catch (const nlohmann::json::exception&) {
            continue;
        }
    }

    if (!partialMatchPath.empty()) {
        std::ifstream file(partialMatchPath);
        if (file.is_open()) {
            try {
                nlohmann::json j;
                file >> j;
                result.found = true;
                result.userId = j.value("id", 0u);
                result.rawName = j.value("rawName", std::string{});
                result.emailAddress = j.value("emailAddress", std::string{});
                result.createdAt = j.value("createdAt", static_cast<int64_t>(0));
                result.lastIp = j.value("lastIp", std::string{});
                result.bannedUntil = j.value("bannedUntil", static_cast<int64_t>(0));
                result.banReason = j.value("banReason", std::string{});
                result.role = static_cast<PlayerRole>(j.value("role", static_cast<uint8_t>(PlayerRole::Default)));
                if (j.contains("notes")) {
                    for (const auto& n : j["notes"])
                        result.notes.push_back({ n.value("author", std::string{}), n.value("time", static_cast<int64_t>(0)), n.value("text", std::string{}) });
                }
            }
            catch (const nlohmann::json::exception&) {}
        }
    }
    return result;
}

std::vector<std::string> JsonPlayerTable::FindAccountsByIp(const std::string& ip, std::size_t limit) const {
    std::vector<std::string> matches;
    if (ip.empty() || !std::filesystem::exists(m_directory))
        return matches;

    for (const auto& entry : std::filesystem::directory_iterator(m_directory)) {
        if (matches.size() >= limit)
            break;
        if (!entry.is_regular_file() || entry.path().extension() != ".json")
            continue;
        std::ifstream file(entry.path());
        if (!file.is_open())
            continue;
        try {
            nlohmann::json j;
            file >> j;
            if (j.value("lastIp", std::string{}) == ip)
                matches.push_back(j.value("rawName", std::string{}));
        }
        catch (const nlohmann::json::exception&) {
            continue;
        }
    }
    return matches;
}

std::string JsonPlayerTable::SetBanByQuery(const std::string& query, int64_t bannedUntil, const std::string& reason) const {
    AccountSummary account = this->FindAccountByQuery(query);
    if (!account.found)
        return {};

    auto path = this->PathFor(SaveKey{ ToLower(account.rawName), false });
    try {
        nlohmann::json j;
        {
            std::ifstream file(path);
            if (!file.is_open())
                return {};
            file >> j;
        }
        j["bannedUntil"] = bannedUntil;
        j["banReason"] = reason;
        std::ofstream outFile(path);
        if (!outFile.is_open())
            return {};
        outFile << j.dump(4);
        return account.rawName;
    }
    catch (const nlohmann::json::exception& e) {
        Logger::Print(WARNING, "JsonPlayerTable::SetBanByQuery: failed to parse '{}': {}", path.string(), e.what());
        return {};
    }
}

std::string JsonPlayerTable::AddNoteByQuery(const std::string& query, const std::string& authorName, const std::string& text) const {
    AccountSummary account = this->FindAccountByQuery(query);
    if (!account.found)
        return {};

    auto path = this->PathFor(SaveKey{ ToLower(account.rawName), false });
    try {
        nlohmann::json j;
        {
            std::ifstream file(path);
            if (!file.is_open())
                return {};
            file >> j;
        }
        nlohmann::json note;
        note["author"] = authorName;
        note["time"] = static_cast<int64_t>(std::time(nullptr));
        note["text"] = text;
        if (!j.contains("notes") || !j["notes"].is_array())
            j["notes"] = nlohmann::json::array();
        j["notes"].push_back(std::move(note));
        std::ofstream outFile(path);
        if (!outFile.is_open())
            return {};
        outFile << j.dump(4);
        return account.rawName;
    }
    catch (const nlohmann::json::exception& e) {
        Logger::Print(WARNING, "JsonPlayerTable::AddNoteByQuery: failed to parse '{}': {}", path.string(), e.what());
        return {};
    }
}

PlayerRegistration JsonPlayerTable::RegisterPlayer(const std::string& name, const std::string& password, const std::string& verifyPassword, const std::string& email) {
    std::string lowerCase = name;

    {
        auto at = email.find('@');
        auto lastDot = email.find_last_of('.');
        if (email.empty() || at == std::string::npos || lastDot == std::string::npos ||
            at == 0 || lastDot < at || lastDot + 1 >= email.size())
            return PlayerRegistration{
                .m_result = PlayerRegistration::Result::INVALID_EMAIL_OR_DISCORD,
                .m_data = "`4Oops!``  Please enter a valid `wemail`` address."
        };
    }
    if (password.length() < 8 || password.length() > 18)
        return PlayerRegistration{
            .m_result = PlayerRegistration::Result::INVALID_PASSWORD_LENGTH,
            .m_data = "`4Oops!``  Your password must be between `$8`` and `$18`` characters long."
    };

    {
        static constexpr std::string_view kSpecialChars = "@#!$^&*.,";
        bool hasLetter = false, hasDigit = false, hasSpecial = false;
        for (char c : password) {
            if (std::isalpha(static_cast<unsigned char>(c))) hasLetter = true;
            else if (std::isdigit(static_cast<unsigned char>(c))) hasDigit = true;
            else if (kSpecialChars.find(c) != std::string_view::npos) hasSpecial = true;
        }
        if (!hasLetter || !hasDigit || !hasSpecial)
            return PlayerRegistration{
                .m_result = PlayerRegistration::Result::INVALID_PASSWORD,
                .m_data = "`4Oops!``  Your password must contain `$1 letter``, `$1 number`` and `$1 special character: @#!$^&*.,``"
        };
    }
    if (verifyPassword != password)
        return PlayerRegistration{
            .m_result = PlayerRegistration::Result::MISMATCH_VERIFY_PASSWORD,
            .m_data = "`4Oops!``  Passwords don't match.  Try again."
    };
    if (!Utils::ToLowerCase(lowerCase))
        return PlayerRegistration{
            .m_result = PlayerRegistration::Result::INVALID_GROWID,
            .m_data = "`4Oops!``  the name is includes invalid characters."
    };
    if (lowerCase.length() < 3 || lowerCase.length() > 18)
        return PlayerRegistration{
            .m_result = PlayerRegistration::Result::INVALID_GROWID_LENGTH,
            .m_data = "`4Oops!``  Your `wGrowID`` must be between `$3`` and `$18`` characters long."
    };
    if (this->IsAccountExist(lowerCase))
        return PlayerRegistration{
            .m_result = PlayerRegistration::Result::EXIST_GROWID,
            .m_data = fmt::format("`4Oops!``  The name `w{}`` is so cool someone else has already taken it.  Please choose a different name.", name)
    };

    return PlayerRegistration{
        .m_result = PlayerRegistration::Result::SUCCESS,
        .m_data = ""
    };
}
