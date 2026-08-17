#pragma once
#include <cstdint>
#include <vector>
#include <array>

namespace CombineRecipes {

    struct Ingredient {
        uint16_t m_itemId;
        uint8_t m_count;
    };

    struct Recipe {
        std::array<Ingredient, 3> m_inputs;

        std::vector<uint16_t> m_results;
        uint8_t m_resultCount;
    };

    enum : uint16_t {
        kSongpyeon = 1056,
        kHarmony   = 1058,
        kPeace     = 1094,
        kLongevity = 1096,
        kProsperity= 1098,
        kBalance   = 1828,
    };

    inline const std::vector<Recipe>& GetTable() {
        static const std::vector<Recipe> kTable = {

            { { { { kBalance, 1 }, { kPeace, 1 }, { kProsperity, 1 } } },     { 1054 }, 3 },
            { { { { kBalance, 6 }, { kLongevity, 4 }, { kProsperity, 4 } } }, { 1056 }, 1 },
            { { { { kSongpyeon, 10 }, { kHarmony, 10 }, { kPeace, 10 } } },   { 1060 }, 1 },
            { { { { kSongpyeon, 5 }, { kProsperity, 20 }, { kLongevity, 20 } } }, { 1062 }, 1 },
            { { { { kHarmony, 5 }, { kLongevity, 5 }, { kPeace, 5 } } },      { 1064 }, 1 },
            { { { { kHarmony, 2 }, { kProsperity, 2 }, { kPeace, 2 } } },     { 1066 }, 3 },
            { { { { kSongpyeon, 2 }, { kLongevity, 4 }, { kHarmony, 4 } } },  { 1068 }, 1 },

            { { { { kPeace, 3 }, { kLongevity, 3 }, { kProsperity, 3 } } },
              { 1070, 1072, 1074, 1076, 1078, 1080, 1082, 1084, 1086, 1088, 1090, 1092 }, 1 },

            { { { { kSongpyeon, 20 }, { kBalance, 20 }, { kLongevity, 20 } } }, { 1804 }, 1 },
            { { { { kSongpyeon, 2 }, { kBalance, 2 }, { kHarmony, 3 } } },      { 1806 }, 1 },
            { { { { kBalance, 1 }, { kPeace, 2 }, { kLongevity, 2 } } },        { 1808 }, 1 },
            { { { { kBalance, 2 }, { kProsperity, 5 }, { kHarmony, 5 } } },     { 1810 }, 1 },

            { { { { kBalance, 1 }, { kPeace, 4 }, { kHarmony, 4 } } },
              { 1812, 1814, 1816, 1818, 1820 }, 1 },
            { { { { kSongpyeon, 40 }, { kBalance, 50 }, { kProsperity, 200 } } }, { 1824 }, 1 },
            { { { { kBalance, 2 }, { kLongevity, 7 }, { kHarmony, 7 } } },      { 1826 }, 1 },
            { { { { kSongpyeon, 4 }, { kBalance, 12 }, { kPeace, 8 } } },       { 1830 }, 1 },
            { { { { kSongpyeon, 1 }, { kHarmony, 3 }, { kProsperity, 3 } } },   { 1832 }, 1 },
            { { { { kHarmony, 2 }, { kLongevity, 2 }, { kProsperity, 2 } } },   { 1834 }, 3 },
            { { { { kSongpyeon, 20 }, { kProsperity, 20 }, { kPeace, 20 } } },  { 1836 }, 1 },

            { { { { 2206, 200 }, { 1962, 200 }, { 2038, 200 } } },              { 2072 }, 1 },
        };
        return kTable;
    }

}
