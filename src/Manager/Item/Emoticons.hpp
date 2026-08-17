#pragma once
#include <array>
#include <string>
#include <string_view>
#include <utility>

inline constexpr std::array<std::pair<std::string_view, std::string_view>, 59> kEmoticons{ {
    {"wl", "ā"}, {"yes", "Ă"}, {"no", "ă"}, {"love", "Ą"},
    {"oops", "ą"}, {"shy", "Ć"}, {"wink", "ć"}, {"tongue", "Ĉ"},
    {"agree", "ĉ"}, {"sleep", "Ċ"}, {"punch", "ċ"}, {"music", "Č"},
    {"build", "č"}, {"megaphone", "Ď"}, {"sigh", "ď"}, {"mad", "Đ"},
    {"wow", "đ"}, {"dance", "Ē"}, {"see-no-evil", "ē"}, {"bheart", "Ĕ"},
    {"heart", "ĕ"}, {"grow", "Ė"}, {"gems", "ė"}, {"kiss", "Ę"},
    {"gtoken", "ę"}, {"lol", "Ě"}, {"smile", "ě"}, {"cool", "Ĝ"},
    {"cry", "ĝ"}, {"vend", "Ğ"}, {"bunny", "ğ"}, {"cactus", "Ġ"},
    {"pine", "ġ"}, {"peace", "Ģ"}, {"terror", "ģ"}, {"troll", "Ĥ"},
    {"evil", "ĥ"}, {"fireworks", "Ħ"}, {"football", "ħ"}, {"alien", "Ĩ"},
    {"party", "ĩ"}, {"pizza", "Ī"}, {"clap", "ī"}, {"song", "Ĭ"},
    {"ghost", "ĭ"}, {"nuke", "Į"}, {"halo", "į"}, {"turkey", "İ"},
    {"gift", "ı"}, {"cake", "Ĳ"}, {"heartarrow", "ĳ"}, {"lucky", "Ĵ"},
    {"shamrock", "ĵ"}, {"grin", "Ķ"}, {"ill", "ķ"}, {"eyes", "ĸ"},
    {"weary", "Ĺ"}, {"moyai", "ĺ"}, {"plead", "Ļ"},
} };

inline constexpr int32_t kEmoticonVersion = 201560520;

inline std::string BuildEmoticonData() {
    std::string data;
    data.reserve(kEmoticons.size() * 24);
    for (const auto& [name, glyph] : kEmoticons) {
        data += '(';
        data += name;
        data += ")|";
        data += glyph;
        data += "|1&";
    }
    return data;
}
