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
        {"assets/levels/bonus_1.json", "Bonus 1", "Bonus Stage 1: Coin Paradise"},
        // The generated campaign (A/mapgen-gan-plan): evolved, oracle-certified,
        // gate-passing levels in a rising difficulty ladder — appended after
        // the hand campaign so existing save indices stay valid and the world
        // map unlocks them in order. Difficulty numbers are the oracle's
        // requiredDifficulty (0..1 of the physics envelope).
        {"assets/levels/campaign_gen/world_1_1.json", "G1-1",  "World 1-1 (Gen): First Steps"},
        {"assets/levels/campaign_gen/world_1_2.json", "G1-2",  "World 1-2 (Gen): Gentle Grounds"},
        {"assets/levels/campaign_gen/world_1_3.json", "G1-3",  "World 1-3 (Gen): Rising Road"},
        {"assets/levels/campaign_gen/world_2_1.json", "G2-1",  "World 2-1 (Gen): Broken Path"},
        {"assets/levels/campaign_gen/world_2_2.json", "G2-2",  "World 2-2 (Gen): Long Crossing"},
        {"assets/levels/campaign_gen/world_2_3.json", "G2-3",  "World 2-3 (Gen): The Climb"},
        {"assets/levels/campaign_gen/world_3_1.json", "G3-1",  "World 3-1 (Gen): Edge of Reach"},
        {"assets/levels/campaign_gen/world_3_2.json", "G3-2",  "World 3-2 (Gen): Narrow Margins"},
        {"assets/levels/campaign_gen/world_3_3.json", "G3-3",  "World 3-3 (Gen): Last Ascent"},
        {"assets/levels/campaign_gen/bonus_gen.json", "G-Max", "Bonus (Gen): The Limit"}
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
