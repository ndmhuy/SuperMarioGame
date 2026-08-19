#pragma once

#include <string>
#include <vector>

// One entry of the campaign, in play order.
struct LevelEntry {
    std::string path;         // asset-relative JSON path
    std::string displayName;  // "1-1", "1-1 Sub", ... as shown on menus and summaries
};

// The campaign order in one place.
//
// It used to be an if-chain in PlayingState::setupTestScene() with a matching
// hardcoded `levelCount = 7` in advanceToNextLevel() and a third copy in the dev
// panel's dropdown. The victory and game-over screens need the display name too,
// which would have made five copies.
class LevelCatalog {
public:
    static const std::vector<LevelEntry>& levels();

    static int count();

    // Path for `index`, or the first level's path when the index is out of range.
    static const std::string& pathFor(int index);

    // Display name for `index`, or "?" when out of range.
    static const std::string& nameFor(int index);

    static bool isValidIndex(int index);
};
