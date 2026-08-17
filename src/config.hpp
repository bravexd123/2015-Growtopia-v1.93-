#pragma once
#include <string>
#include <filesystem>

class Configuration {
public:
    static std::string GetName() { return "Growtopia Server"; }
    static std::string GetVersion() { return "0.0.1"; }

    static std::string GetBaseHost() { return "127.0.0.1"; }
    static uint16_t GetBasePort() { return 17091; }

public:
    static std::filesystem::path GetPlayerDBDirectory() { return "db/players"; }
    static std::filesystem::path GetWorldDBDirectory() { return "db/worlds"; }
};
