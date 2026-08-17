#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <array>

namespace Playmods {

    enum class Action {

        Apply,

        Drop,

        Throw,

        Consume,

        CuteWords,

        Pet
    };

    struct Info {
        uint16_t m_playmodId;
        uint16_t m_itemId;
        int32_t m_durationSeconds;
        const char* m_name;
        const char* m_onUsed;
        const char* m_onRemoved;
        uint16_t m_displayItemId;
        int8_t m_stateBit;
        const char* m_sfx;
        uint32_t m_skinColour;
        Action m_action;
        int32_t m_effect;
        int32_t m_gravity;
    };

    inline const std::vector<Info>& GetTable() {
        static const std::vector<Info> kTable = {
            {  1,  388,   300, "Stinky",          "You really really smell.",                                    "The air clears.",                                    372, 14, "spray.wav",        0,          Action::Apply,     0,   0 },
            {  2, 1368,     2, "Frozen",          "Your body has turned to ice.You can't move!",                 "You've thawed out.",                                1368, 11, "freeze.wav",       4284769380, Action::Apply,     0,   0 },
            {  3,  274,    10, "Frozen",          "Freeze!",                                                     "You've thawed out.",                                 274, 11, "freeze.wav",       4284769380, Action::Apply,     0,   0 },
            {  4,  874,   180, "Egged!",          "You have egg on your face.",                                  "You washed your face!",                              874, -1, "",                 16777215,   Action::Throw,    42,   0 },
            {  8,  386,     0, "",                "",                                                            "",                                                     0, -1, "",                 0,          Action::CuteWords, 0,   0 },
            { 10, 2206,  1800, "Irradiated",      "You are aglow with radiation!",                               "You have recovered.",                               2206, 19, "",                 0,          Action::Apply,     0,   0 },
            { 11,  408,   300, "Duct Tape",       "Duct tape has covered your mouth!",                           "Duct tape removed. OUCH!",                           408, 13, "already_used.wav", 0,          Action::Apply,     0,   0 },
            { 12,  384,  3600, "Valentine",       "You are somebody's valentine!",                               "Yuck!",                                              384, -1, "choir.wav",        2526478335, Action::Apply,     0,   0 },
            { 13, 2480,   300, "Megaphone!",      "Broadcasting to ALL!",                                        "You can broadcast once again.",                     2480, -1, "",                 0,          Action::Apply,     0,   0 },
            { 14,  528,  1800, "Lucky",           "You're luckier than before!",                                 "Your luck has worn off.",                            528, 15, "",                 0,          Action::Apply,     0,   0 },
            { 15, 1510,    10, "1512",            "",                                                            "",                                                     0, -1, "",                 0,          Action::Pet,       0,   0 },
            { 17,  196,  3600, "Feelin' Blue",    "A `!blueberry`` slides down your throat!",                    "The effects of the `!blueberry`` have worn off.",     196, -1, "spray.wav",        4278190335, Action::Drop,      0,   0 },
            { 18,  338,     2, "Floating!",       "Whoooooooaaaaaaaa...",                                        "Gravity - it's the law.",                            338, -1, "balloon.wav",      0,          Action::Drop,      0, -30 },
            { 19,  962,   180, "Saucy!",          "You are a saucy person.",                                     "You got cleaned up.",                                962, -1, "",                 65535,      Action::Throw,    45,   0 },
            { 20,  950,     0, ":D`#YUM!``",      "",                                                            "",                                                     0, -1, "",                 0,          Action::Consume,   0,   0 },
            { 21,  968,     0, ":D`#YUM!``",      "",                                                            "",                                                     0, -1, "",                 0,          Action::Consume,   0,   0 },
            { 24,  868,     0, ":D`#YUM!``",      "",                                                            "",                                                     0, -1, "",                 0,          Action::Consume,   0,   0 },
            { 25,  782,  3600, "Antidote!",       "You are now immune to zombie bites! Temporarily...",          "Your immunity is gone.",                             782, -1, "",                 0,          Action::Drop,      0,   0 },
            { 27,  128,  1800, "Golden Halo!",    "You have been good.",                                         "You're falling out of favor.",                       128,  7, "",                 2190853119, Action::Drop,      0,   0 },
            { 28,  764,    60, "Infected!",       "You've been infected by the g-Virus. Punch others to infect them, too! Braiiiins...", "You've been cured.", 764, 16, "",                 0,          Action::Drop,      0,   0 },
            { 30, 1058,     0, ":D`#YUM!``",      "",                                                            "",                                                     0, -1, "",                 0,          Action::Consume,   0,   0 },
            { 31, 1094,     0, ":D`#YUM!``",      "",                                                            "",                                                     0, -1, "",                 0,          Action::Consume,   0,   0 },
            { 32, 1096,     0, ":D`#YUM!``",      "",                                                            "",                                                     0, -1, "",                 0,          Action::Consume,   0,   0 },
            { 33, 1098,     0, ":D`#YUM!``",      "",                                                            "",                                                     0, -1, "",                 0,          Action::Consume,   0,   0 },
            { 34, 2002, 86400, "Doctor Replusion","",                                                            "",                                                  2002, -1, "spray.wav",        0,          Action::Apply,     0,   0 },
            { 36, 1056,  1800, "Lucky",           "You're luckier than before!",                                 "Your luck has worn off.",                           1056, 15, "",                 0,          Action::Apply,     0,   0 },
            { 38,  614,   300, "Rotten Egg",      "You really really smell.",                                    "The air clears.",                                    614, 14, "spray.wav",        0,          Action::Apply,     0,   0 },
            { 39, 1374,     0, ":D`#YUM!``",      "",                                                            "",                                                     0, -1, "",                 0,          Action::Consume,   0,   0 },
            { 40,  406,     0, ":D`#YUM!``",      "",                                                            "",                                                     0, -1, "",                 0,          Action::Consume,   0,   0 },
            { 41,  966,     0, ":D`#YUM!``",      "",                                                            "",                                                     0, -1, "",                 0,          Action::Consume,   0,   0 },
            { 43,  958,     0, ":D`#YUM!``",      "",                                                            "",                                                     0, -1, "",                 0,          Action::Consume,   0,   0 },
            { 45, 1580,     0, ":D`#YUM!``",      "",                                                            "",                                                     0, -1, "",                 0,          Action::Consume,   0,   0 },
            { 47, 1634,     5, "Caffeinated",     "",                                                            "",                                                  1634, 14, "spray.wav",        0,          Action::Apply,     0,   0 },
            { 53, 1474,  1800, "Food: Extra XP",  "",                                                            "",                                                  1474, -1, "spray.wav",        0,          Action::Drop,      0,   0 },
            { 60, 2734,     0, ":D`#YUM!``",      "",                                                            "",                                                     0, -1, "",                 0,          Action::CuteWords, 0,   0 },
            { 64,  964,     0, ":D`#YUM!``",      "",                                                            "",                                                     0, -1, "",                 0,          Action::Consume,   0,   0 },
            { 67,  126,  1800, "Devil Horns",     "",                                                            "",                                                   126,  6, "spray.wav",        0,          Action::Drop,      0,   0 },
            { 68, 1964,  1800, "Devil Horns",     "",                                                            "",                                                  1964,  6, "spray.wav",        0,          Action::Drop,      0,   0 },
            { 69,  960,     5, "ON FIRE!!!",      "",                                                            "",                                                   960, 17, "spray.wav",        842203135,  Action::Apply,     0,   0 },
            { 70,  712,     5, "ON FIRE!!!",      "",                                                            "",                                                   712, 17, "spray.wav",        842203135,  Action::Apply,     0,   0 },
            { 71, 1988,  1800, "Haunted!",        "",                                                            "",                                                   372, 18, "spray.wav",        0,          Action::Apply,     0,   0 },
            { 72, 1772,     2, "Floating!",       "Whoooooooaaaaaaaa...",                                        "Gravity - it's the law.",                           1772, -1, "balloon.wav",      0,          Action::Drop,      0, -30 },
            { 74,  676, 86400, "Doctor Replusion","",                                                            "",                                                   676, -1, "spray.wav",        0,          Action::Apply,     0,   0 },
            { 75,  276,     0, "",                "",                                                            "",                                                     0, -1, "",                 0,          Action::CuteWords, 0,   0 },

            { 76,  732,   600, "Banned",          "Reality flickers as you begin to wake up.",                   "You are no longer banned. Now be good!",             732, 12, "",                 0,          Action::Apply,     0,   0 },
            { 77,  618,     0, "",                "",                                                            "",                                                     0, -1, "",                 0,          Action::CuteWords, 0,   0 },
            { 78,  616,     0, "",                "",                                                            "",                                                     0, -1, "",                 0,          Action::CuteWords, 0,   0 },
            { 80,  750,  1800, "Lucky",           "You're luckier than before!",                                 "Your luck has worn off.",                            750, 15, "",                 0,          Action::Apply,     0,   0 },
            { 81,  752,     0, "",                "",                                                            "",                                                     0, -1, "",                 0,          Action::CuteWords, 0,   0 },
            { 82, 1208,     0, ":D`#YUM!``",      "",                                                            "",                                                     0, -1, "",                 0,          Action::Consume,   0,   0 },
            { 98, 2734,  1200, "Ultimate Super Pineapple", "",                                                   "",                                                  2734, 25, "",                 0,          Action::Apply,     0,   0 },
            {115, 1602,   900, "Minty",           "Ya'll are feelin' minty, sugah.",                             "Healthy color restored.",                           1602, -1, "eat.wav",          1627349247, Action::Apply,     0,   0 },
            {119, 1662,     5, "Spikeproof",      "You are briefly immune to spikes and lava",                   "You feel vulnerable again.",                        1662, 10, "",                 0,          Action::Apply,     0,   0 },
            {123,  368,     6, "Muddy",           "You've been splattered with mud!",                            "Clean again!",                                       368, -1, "spray.wav",        1348237567, Action::Apply,     0,   0 },

            {139,  278,   600, "Curse",           "You've been cursed to (the world) `4hell``!",                 "You are no longer bound to the netherworld, you're free to go.", 278, 12, "", 0, Action::Apply, 0, 0 },

            {900,  540,   900, "Envious",         "It ain't easy being you.",                                    "Healthy color restored.",                            540, -1, "eat.wav",          1627349247, Action::Consume,   0,   0 },

            {901,    0,     0, "In the Spotlight","",                                                            "Back to anonymity.",                                2646, 20, "",                 0,          Action::Apply,     0,   0 },
        };
        return kTable;
    }

    inline const Info* GetForItem(uint16_t itemId) {
        for (const auto& row : GetTable())
            if (row.m_itemId == itemId)
                return &row;
        return nullptr;
    }

    inline const Info* GetById(uint16_t playmodId) {
        for (const auto& row : GetTable())
            if (row.m_playmodId == playmodId)
                return &row;
        return nullptr;
    }

    inline const std::array<uint16_t, 7>& GetAfflictionIds() {
        static const std::array<uint16_t, 7> kIds = { 4, 17, 19, 27, 28, 71, 72 };
        return kIds;
    }

    inline std::string FormatDuration(int64_t seconds) {
        if (seconds >= 86400)
            return std::to_string(seconds / 86400) + " days";
        if (seconds >= 3600)
            return std::to_string(seconds / 3600) + " hours";
        if (seconds >= 60)
            return std::to_string(seconds / 60) + " minutes";
        return std::to_string(seconds) + " seconds";
    }

}
