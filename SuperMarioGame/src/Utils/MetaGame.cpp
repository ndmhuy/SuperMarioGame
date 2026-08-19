#include "Utils/MetaGame.hpp"
#include "Core/AchievementManager.hpp"
#include "Utils/CampaignProgress.hpp"
#include "Utils/LevelCatalog.hpp"

#include <algorithm>
#include <ctime>
#include <string>

namespace {
// Each cycle makes enemies 15% faster, and it stops at four. Past roughly 1.6x
// an enemy can cross more than its own width in a frame, which lets it tunnel
// through the collision grid — the difficulty would stop being difficulty and
// start being a bug.
constexpr float NG_PLUS_SPEED_STEP = 0.15f;
constexpr int NG_PLUS_MAX_CYCLES = 4;
}

int MetaGame::newGamePlusLevel() {
    return CampaignProgress::newGamePlusLevel();
}

bool MetaGame::advanceNewGamePlus() {
    return CampaignProgress::advanceNewGamePlus();
}

float MetaGame::enemySpeedMultiplier() {
    const int cycles = std::clamp(newGamePlusLevel(), 0, NG_PLUS_MAX_CYCLES);
    return 1.0f + static_cast<float>(cycles) * NG_PLUS_SPEED_STEP;
}

std::string MetaGame::newGamePlusLabel() {
    const int level = newGamePlusLevel();
    if (level <= 0) return "";
    if (level == 1) return "NEW GAME+";
    return "NEW GAME+" + std::to_string(level);
}

unsigned int MetaGame::dailySeed(int year, int month, int day) {
    // Any stable mixing of the date will do; what matters is that it depends on
    // nothing else, so two players on the same day get the same level.
    unsigned int seed = static_cast<unsigned int>(year) * 10000u
                      + static_cast<unsigned int>(month) * 100u
                      + static_cast<unsigned int>(day);
    seed ^= seed << 13;
    seed ^= seed >> 17;
    seed ^= seed << 5;
    // 0 means "pick a random seed" to MapGenerator, so it can never be the
    // answer here — that would make the daily challenge different every time.
    return seed == 0u ? 1u : seed;
}

unsigned int MetaGame::todaysSeed() {
    const std::time_t now = std::time(nullptr);
    const std::tm* utc = std::gmtime(&now);
    if (!utc) return 1u;
    return dailySeed(utc->tm_year + 1900, utc->tm_mon + 1, utc->tm_mday);
}

MapGeneratorConfig MetaGame::dailyChallengeConfig(unsigned int seed) {
    MapGeneratorConfig config;
    config.seed = seed;

    // The theme and shape are derived from the seed too, so the challenge varies
    // day to day rather than being the same level with different scenery.
    config.theme = static_cast<MapTheme>(seed % 4u);
    config.difficulty = MapDifficulty::Medium;
    config.pitProbability  = 0.10f + static_cast<float>(seed % 11u) * 0.01f;
    config.enemySpawnRate  = 0.18f + static_cast<float>((seed / 7u) % 13u) * 0.01f;
    config.coinClusterRate = 0.15f + static_cast<float>((seed / 13u) % 11u) * 0.01f;
    return config;
}

std::string MetaGame::todaysChallengeName() {
    const std::time_t now = std::time(nullptr);
    const std::tm* utc = std::gmtime(&now);
    if (!utc) return "DAILY CHALLENGE";

    char buffer[32] = {0};
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d", utc);
    return std::string("DAILY ") + buffer;
}

std::vector<Unlockable> MetaGame::unlockables() {
    AchievementManager& achievements = AchievementManager::getInstance();
    const std::vector<LevelProgress> progress = CampaignProgress::load();

    const int cleared = static_cast<int>(
        std::count_if(progress.begin(), progress.end(),
                      [](const LevelProgress& level) { return level.completed; }));

    std::vector<Unlockable> list;
    list.push_back({"toad", "TOAD", "CLEAR ALL 3 LEVELS", achievements.isUnlocked("toad")});
    list.push_back({"peach", "PEACH", "CLEAR ALL 3 NO DEATHS", achievements.isUnlocked("peach")});
    list.push_back({"star_hoarder", "STAR HOARDER", "COLLECT 9 STAR COINS",
                    CampaignProgress::totalStarCoins() >= 9});
    list.push_back({"campaign", "NEW GAME+", "FINISH THE CAMPAIGN",
                    newGamePlusLevel() > 0});
    list.push_back({"completionist", "ALL LEVELS", "CLEAR EVERY LEVEL",
                    cleared >= LevelCatalog::count()});
    return list;
}
