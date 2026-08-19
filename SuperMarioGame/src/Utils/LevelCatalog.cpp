#include "Utils/LevelCatalog.hpp"

const std::vector<LevelEntry>& LevelCatalog::levels() {
    static const std::vector<LevelEntry> kLevels = {
        {"assets/levels/level_1.json",     "1-1"},
        {"assets/levels/level_1_sub.json", "1-1 Sub"},
        {"assets/levels/level_2.json",     "1-2"},
        {"assets/levels/level_2_sub.json", "1-2 Sub"},
        {"assets/levels/level_3.json",     "1-3"},
        {"assets/levels/level_3_sub.json", "1-3 Sub"},
        {"assets/levels/bonus_1.json",     "Bonus 1"}
    };
    return kLevels;
}

int LevelCatalog::count() {
    return static_cast<int>(levels().size());
}

bool LevelCatalog::isValidIndex(int index) {
    return index >= 0 && index < count();
}

const std::string& LevelCatalog::pathFor(int index) {
    return isValidIndex(index) ? levels()[static_cast<std::size_t>(index)].path
                               : levels().front().path;
}

const std::string& LevelCatalog::nameFor(int index) {
    static const std::string kUnknown = "?";
    return isValidIndex(index) ? levels()[static_cast<std::size_t>(index)].displayName
                               : kUnknown;
}
