#pragma once
#include <array>
#include <vector>
#include <cstdio>
#include <string>
#include <string_view>
#ifdef __linux__

#elif _WIN32
#include <winsock.h>

#define HOST_NAME_MAX       128
#endif
#include <enet/enet.h>
#include <Packet/GameUpdatePacket.hpp>
#include <Packet/VariantList.hpp>
#include <Packet/PacketFactory.hpp>

namespace Utils {
    inline std::vector<std::string> Split(std::string data, std::string delimiter) {
        std::size_t startPos = 0, endPos, delimiterLength = delimiter.length();
        std::string token{};
        std::vector<std::string> ret{};

        while ((endPos = data.find(delimiter, startPos)) != std::string::npos) {
            token = data.substr(startPos, endPos - startPos);
            startPos = endPos + delimiterLength;
            ret.push_back(token);
        }

        ret.push_back (data.substr(startPos));
        return ret;
    }
    inline bool IsValidUsername(std::string username) {
        if (username.empty())
            return false;

        for (auto ch : username) {
            if (!(ch >= '0' && ch <= '9') && !((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z')))
                return false;
        }
        return true;
    }
    inline bool ToLowerCase(std::string& data, const bool& underscore = false) {
        if (data.empty())
            return false;

        for (auto& ch : data) {
            if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z'))
                ch = std::tolower(ch);
            else if (!(ch >= '0' && ch <= '9') && !(underscore && ch == '_'))
                return false;
        }
        return true;
    }

    inline void ToUpperCase(std::string& data) {
        for (auto& ch : data) {
            if (ch >= 'a' && ch <= 'z')
                ch = static_cast<char>(std::toupper(ch));
        }
    }

    inline std::string FormatWithCommas(int64_t value) {
        bool negative = value < 0;
        std::string digits = std::to_string(negative ? -value : value);
        std::string out;
        out.reserve(digits.size() + digits.size() / 3 + 1);
        for (std::size_t i = 0; i < digits.size(); i++) {
            if (i > 0 && (digits.size() - i) % 3 == 0)
                out.push_back(',');
            out.push_back(digits[i]);
        }
        return negative ? "-" + out : out;
    }
}

namespace Console {
    inline std::string Execute(std::string command) {
    #ifdef __linux__
        std::array<char, 128> buffer;
        std::string result{};

        auto pipe = popen(command.c_str(), "r");
        if (!pipe)
            return std::string{ "N/A" };

        while (!feof(pipe)) {
            if (fgets(buffer.data(), 128, pipe) != nullptr)
                result += buffer.data();
        }

        pclose(pipe);
        return result;
    #else
        return std::string{};
    #endif
    }
    inline std::string GetHostname() {
        std::array<char, HOST_NAME_MAX> buffer{};
        gethostname(buffer.data(), HOST_NAME_MAX);
        return std::string{ buffer.data() };
    }
    inline std::string GetLoadAverage() {
    #ifdef __linux__
        std::array<double, 3> buffer{};
        if(getloadavg(buffer.data(), 3) < 3)
            return std::string{ "00.00 00.00 00.00" };
        return fmt::format("{} {} {}", fmt::format("{:.{}f}", buffer[0], 2), fmt::format("{:.{}f}", buffer[1], 2), fmt::format("{:.{}f}", buffer[2], 2));
    #else
        return std::string{ "00.00 00.00 00.00" };
    #endif
    }
}

namespace Hash {
    constexpr std::size_t Fnv1A(const std::string_view& data) {
        std::size_t prime = 1099511628211ULL;
        std::size_t hash{ 14695981039346656037ULL };
        for (auto& c : data)
            hash ^= c, hash *= prime;
        return hash;
    }
}

constexpr std::size_t operator ""_fnv(const char* str, std::size_t len) {
    return Hash::Fnv1A(std::string_view{ str, len });
}
