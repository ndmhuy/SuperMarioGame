#pragma once

#include <array>
#include <string>
#include <vector>

// Per-level campaign progress, persisted across runs.
//
// Serializer already writes a "progress" block into each save slot, but its
// levelsCompleted list is *fabricated* — it is generated as [1 .. levelId-1] on
// the assumption that reaching a level means having cleared everything before
// it, and its star-coin map only ever holds the level being saved. That is
// enough to write a save file and not enough to draw a world map, which needs to
// know which levels are actually finished and how many star coins each one gave
// up.
//
// Kept as static functions over a small value type rather than another
// singleton: this is a file with a load, a merge and a save, and nothing needs
// to hold it open.
struct LevelProgress {
    bool completed = false;
    std::array<bool, 3> starCoins{{false, false, false}};

    int starCoinCount() const;
};

class CampaignProgress {
public:
    // Always sized to LevelCatalog::count(), so callers can index by level
    // without bounds checks. A missing or corrupt file reads as "nothing done".
    static std::vector<LevelProgress> load();

    // Marks a level cleared and merges its star coins in — collected coins are
    // never un-collected by a later, worse run.
    static bool recordLevelCleared(int levelIndex, const std::array<bool, 3>& starCoins);

    // Sequential unlock: the first level is always open, and every other level
    // needs the one before it finished.
    static bool isUnlocked(int levelIndex);

    // Highest level the player may enter — the furthest unlocked index.
    static int highestUnlockedIndex();

    // Total star coins collected across the campaign, for the map's header.
    static int totalStarCoins();

    // --- New Game+ (task 11.3) ---
    // How many times the campaign has been finished. Kept here rather than in a
    // third file because this one already outlives a run.
    static int newGamePlusLevel();
    // Records another completed campaign and clears the level flags, so the
    // next cycle starts from 1-1 with the unlocks and the counter intact.
    static bool advanceNewGamePlus();

    // Wipes the file, counter included.
    static bool reset();

private:
    static std::string filePath();
    static bool save(const std::vector<LevelProgress>& progress);
};
