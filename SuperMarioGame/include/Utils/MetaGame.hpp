#pragma once

#include "Utils/MapGenerator.hpp"
#include <string>
#include <vector>

// One thing the player can earn, and what earns it.
struct Unlockable {
    std::string id;
    std::string name;
    std::string requirement;
    bool unlocked = false;
};

// Task 11.3 — the meta-game layer that sits above a single run.
//
// New Game+ and the daily challenge both need somewhere to live that outlasts a
// level: CampaignProgress already persists across runs, so the NG+ counter goes
// there rather than in a third file.
class MetaGame {
public:
    // --- New Game+ ---
    // How many times the campaign has been finished. 0 is a first playthrough.
    static int newGamePlusLevel();
    // Called when the last level is cleared. Idempotent per completion — the
    // caller is PlayingState, which only reaches it once per campaign.
    static bool advanceNewGamePlus();

    // Extra enemy speed per NG+ cycle, multiplied on top of the difficulty
    // strategy. Capped, because an uncapped multiplier eventually makes enemies
    // teleport through the collision grid.
    static float enemySpeedMultiplier();

    // A label for the HUD and the menus ("", "NEW GAME+", "NEW GAME+2", ...).
    static std::string newGamePlusLabel();

    // --- Daily challenge ---
    // Same seed for everyone on the same date, which is the whole point: it is
    // only a challenge if two players get the same level.
    static unsigned int dailySeed(int year, int month, int day);
    // Today's seed, from the system clock.
    static unsigned int todaysSeed();
    // A generator config built from a date, so the level is reproducible.
    static MapGeneratorConfig dailyChallengeConfig(unsigned int seed);
    // "DAILY 2026-08-20", for the menu.
    static std::string todaysChallengeName();

    // --- Unlockables ---
    // Everything the player can earn and whether they have. Reads
    // AchievementManager and CampaignProgress; owns no state of its own.
    static std::vector<Unlockable> unlockables();
};
