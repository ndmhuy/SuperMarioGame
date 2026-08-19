#include "Utils/CampaignProgress.hpp"
#include "Utils/LevelCatalog.hpp"
#include "Utils/Serializer.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

int LevelProgress::starCoinCount() const {
    return static_cast<int>(std::count(starCoins.begin(), starCoins.end(), true));
}

std::string CampaignProgress::filePath() {
    // Shares Serializer's resolved directory, so progress cannot end up in a
    // different saves/ than the settings and the save slots.
    return Serializer::saveDirectory() + "/progress.json";
}

std::vector<LevelProgress> CampaignProgress::load() {
    std::vector<LevelProgress> progress(static_cast<std::size_t>(LevelCatalog::count()));

    try {
        const std::string path = filePath();
        if (!std::filesystem::exists(path)) return progress;

        std::ifstream file(path);
        if (!file.is_open()) return progress;

        nlohmann::json j;
        file >> j;
        if (!j.contains("levels") || !j["levels"].is_array()) return progress;

        // Read by index, and ignore anything past the current campaign length:
        // a progress file written when the campaign was longer must not resize
        // the vector callers index into.
        const auto& levels = j["levels"];
        for (std::size_t i = 0; i < progress.size() && i < levels.size(); ++i) {
            progress[i].completed = levels[i].value("completed", false);
            if (levels[i].contains("starCoins") && levels[i]["starCoins"].is_array()) {
                const auto& coins = levels[i]["starCoins"];
                for (std::size_t c = 0; c < 3 && c < coins.size(); ++c) {
                    progress[i].starCoins[c] = coins[c].get<bool>();
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "[CampaignProgress] Could not read progress: " << e.what()
                  << " — starting from nothing." << std::endl;
        progress.assign(static_cast<std::size_t>(LevelCatalog::count()), LevelProgress{});
    }
    return progress;
}

bool CampaignProgress::save(const std::vector<LevelProgress>& progress) {
    try {
        const std::string path = filePath();
        std::filesystem::path fsPath(path);
        if (fsPath.has_parent_path()) {
            std::filesystem::create_directories(fsPath.parent_path());
        }

        nlohmann::json j;
        j["version"] = "1.0";
        j["levels"] = nlohmann::json::array();
        for (const auto& level : progress) {
            nlohmann::json entry;
            entry["completed"] = level.completed;
            entry["starCoins"] = {level.starCoins[0], level.starCoins[1], level.starCoins[2]};
            j["levels"].push_back(std::move(entry));
        }

        std::ofstream file(path);
        if (!file.is_open()) return false;
        file << j.dump(4);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[CampaignProgress] Could not write progress: " << e.what() << std::endl;
        return false;
    }
}

bool CampaignProgress::recordLevelCleared(int levelIndex, const std::array<bool, 3>& starCoins) {
    if (!LevelCatalog::isValidIndex(levelIndex)) return false;

    std::vector<LevelProgress> progress = load();
    LevelProgress& level = progress[static_cast<std::size_t>(levelIndex)];
    level.completed = true;
    // Merge, never overwrite: replaying a level and missing a coin must not take
    // away one that was already found.
    for (std::size_t i = 0; i < 3; ++i) {
        level.starCoins[i] = level.starCoins[i] || starCoins[i];
    }
    return save(progress);
}

bool CampaignProgress::isUnlocked(int levelIndex) {
    if (!LevelCatalog::isValidIndex(levelIndex)) return false;
    if (levelIndex == 0) return true;   // the first level is always open

    const std::vector<LevelProgress> progress = load();
    return progress[static_cast<std::size_t>(levelIndex - 1)].completed;
}

int CampaignProgress::highestUnlockedIndex() {
    const std::vector<LevelProgress> progress = load();
    int highest = 0;
    for (std::size_t i = 0; i < progress.size(); ++i) {
        if (progress[i].completed && static_cast<int>(i) + 1 < LevelCatalog::count()) {
            highest = static_cast<int>(i) + 1;
        }
    }
    return highest;
}

int CampaignProgress::totalStarCoins() {
    int total = 0;
    for (const LevelProgress& level : load()) {
        total += level.starCoinCount();
    }
    return total;
}

bool CampaignProgress::reset() {
    std::error_code ec;
    std::filesystem::remove(filePath(), ec);
    return !ec;
}
