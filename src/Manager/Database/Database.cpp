#include <Manager/Database/Database.hpp>
#include <filesystem>
#include <config.hpp>
#include <fmt/color.h>
#include <Logger/Logger.hpp>

Database g_database;
Database* GetDatabase() {
    return &g_database;
}

bool Database::Connect() {
    try {
        std::filesystem::create_directories(Configuration::GetPlayerDBDirectory());
        std::filesystem::create_directories(Configuration::GetPlayerDBDirectory() / "guests");
        std::filesystem::create_directories(Configuration::GetWorldDBDirectory());

        m_pPlayerTable = new JsonPlayerTable(Configuration::GetPlayerDBDirectory());
        m_pWorldTable = new JsonWorldTable(Configuration::GetWorldDBDirectory());

        Logger::Print(INFO, "Initializing {}, using flat-file JSON storage at '{}' and '{}'.",
            fmt::format(fmt::emphasis::bold | fg(fmt::color::cornsilk), "Database"),
            Configuration::GetPlayerDBDirectory().string(),
            Configuration::GetWorldDBDirectory().string());
        return true;
    }
    catch (const std::filesystem::filesystem_error& e) {
        Logger::Print(EXCEPTION, "Database error (filesystem): {}", e.what());
        return false;
    }
    catch (const std::exception& e) {
        Logger::Print(EXCEPTION, "Database error: {}", e.what());
        return false;
    }
}

JsonPlayerTable* Database::GetPlayerTable() {
    return m_pPlayerTable;
}
JsonWorldTable* Database::GetWorldTable() {
    return m_pWorldTable;
}
