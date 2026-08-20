#include "Utils/LevelCatalog.hpp"

const std::vector<LevelEntry>& LevelCatalog::levels() {
    // Main path only. The *_sub levels are side rooms reached by a warp pipe and
    // left by the pipe at their far end — they carry no flagpole, so nothing in
    // them can ever publish LevelComplete. Listing them here put them in the
    // linear campaign anyway: finishing 1-1 advanced the player into "1-1 Sub",
    // which had no exit and no way to progress. They are still fully playable;
    // they are reached the way the level design intends, through the pipe.
    static const std::vector<LevelEntry> kLevels = {
        {"assets/levels/level_1.json", "1-1",     "World 1-1: Grassland Overworld"},
        {"assets/levels/level_2.json", "1-2",     "World 1-2: Ice Cavern Path"},
        {"assets/levels/level_3.json", "1-3",     "World 1-3: Bowser's Castle Fortress"},
        {"assets/levels/bonus_1.json", "Bonus 1", "Bonus Stage 1: Coin Paradise"}
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

const std::string& LevelCatalog::longNameFor(int index) {
    static const std::string kUnknown = "?";
    return isValidIndex(index) ? levels()[static_cast<std::size_t>(index)].longName
                               : kUnknown;
}

const std::vector<const char*>& LevelCatalog::longNameItems() {
    // Backed by levels(), which is a function-local static built once, so these
    // pointers stay valid for the life of the process.
    static const std::vector<const char*> kItems = [] {
        std::vector<const char*> items;
        items.reserve(levels().size());
        for (const LevelEntry& entry : levels()) items.push_back(entry.longName.c_str());
        return items;
    }();
    return kItems;
}
