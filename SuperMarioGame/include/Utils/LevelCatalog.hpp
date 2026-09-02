#pragma once

#include <string>
#include <vector>

// One entry of the campaign, in play order.
struct LevelEntry {
    std::string path;         // asset-relative JSON path
    std::string displayName;  // "1-1", "1-2", ... as shown on menus and summaries
    std::string longName;     // "World 1-1: Grassland Overworld", for the dev dropdown
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

    // Long descriptive name for `index`, or "?" when out of range. The dev
    // panel's dropdown used to hold its own array of these, which is how it
    // ended up offering seven levels after the campaign dropped to four.
    static const std::string& longNameFor(int index);

    // Every longName in play order, as C strings, for ImGui::Combo.
    static const std::vector<const char*>& longNameItems();

    static bool isValidIndex(int index);

    // --- Author-created levels -------------------------------------------
    //
    // The level editor wrote its output to saves/<name>.json and nothing in the
    // game ever looked there: LevelCatalog listed only the four shipped files,
    // so an authored level could be saved and then never played. That is the
    // "where is it and how do I play it" dead end. Custom levels now live beside
    // the shipped ones in assets/levels/custom/ and this is what finds them.
    //
    // They are deliberately NOT part of levels()/count(): the campaign is a
    // fixed sequence that CampaignProgress sizes its arrays to and
    // advanceToNextLevel() walks. Dropping a file into a directory must not
    // renumber World 1-3.

    // Asset-relative directory custom levels live in ("assets/levels/custom").
    static const std::string& customDirectory();

    // The same directory as an existing filesystem path, created if absent.
    // Empty only if it could not be created.
    static std::string resolvedCustomDirectory();

    // Custom levels found on the last scan, sorted by file name. `displayName`
    // is the level's own "name" field; `longName` adds the file stem so two
    // levels that named themselves the same are still distinguishable.
    static const std::vector<LevelEntry>& customLevels();

    // Re-scan the custom directory. Cheap, and called whenever a screen that
    // shows the list opens — a level saved since the game started must appear
    // without a restart.
    static void refreshCustomLevels();

    // True when `path` names one of the files that ship with the game — the
    // campaign levels and their sub-levels. The editor refuses to overwrite one
    // without an explicit confirmation.
    static bool isBuiltIn(const std::string& path);

    // Where the editor should save a level called `levelName`: the custom
    // directory, the name reduced to a safe file stem, ".json". Never points
    // inside assets/levels itself, so Save As can never clobber a shipped file.
    static std::string customPathFor(const std::string& levelName);

    // `levelName` reduced to lowercase, digits, '-' and '_'. Exposed because the
    // editor shows the resulting file name before the save happens.
    static std::string toFileStem(const std::string& levelName);
};
