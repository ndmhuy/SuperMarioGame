#include "Utils/Serializer.hpp"
#include "Entities/Player.hpp"
#include "Entities/Mario.hpp"
#include "Entities/Luigi.hpp"
#include "Entities/Toad.hpp"
#include "Entities/Peach.hpp"
#include "Entities/IPlayerState.hpp"
#include "Core/StatisticsTracker.hpp"
#include "Core/AchievementManager.hpp"
#include <fstream>
#include <filesystem>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <ctime>
#include <algorithm>
#include <vector>

// Helper to get active character name
static std::string getCharacterName(const Player& player) {
    if (dynamic_cast<const Mario*>(&player)) return "mario";
    if (dynamic_cast<const Luigi*>(&player)) return "luigi";
    if (dynamic_cast<const Toad*>(&player)) return "toad";
    if (dynamic_cast<const Peach*>(&player)) return "peach";
    return "mario";
}

// Helper to get active state name
static std::string getPlayerStateName(const Player& player) {
    IPlayerState* state = player.getCurrentState();
    // Unwrap decorators if present
    while (auto* decorator = dynamic_cast<PlayerStateDecorator*>(state)) {
        state = decorator->getWrappedState();
    }
    if (dynamic_cast<SmallState*>(state)) return "small";
    if (dynamic_cast<SuperState*>(state)) return "super";
    if (dynamic_cast<FireState*>(state)) return "fire";
    if (dynamic_cast<CapeState*>(state)) return "cape";
    if (dynamic_cast<MiniState*>(state)) return "mini";
    return "small";
}

// Helper to restore base player state
static void restorePlayerState(Player& player, const std::string& stateName) {
    std::unique_ptr<IPlayerState> state;
    if (stateName == "small") state = std::make_unique<SmallState>();
    else if (stateName == "super") state = std::make_unique<SuperState>();
    else if (stateName == "fire") state = std::make_unique<FireState>();
    else if (stateName == "cape") state = std::make_unique<CapeState>();
    else if (stateName == "mini") state = std::make_unique<MiniState>();
    else state = std::make_unique<SmallState>();

    player.changeState(std::move(state));
}

static std::string getCurrentISOTimestamp() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::gmtime(&now_c), "%Y-%m-%dT%H:%M:%SZ");
    return ss.str();
}

// Everything under saves/ resolves through here.
//
// These used to be bare relative paths, so the file you got depended on where
// the binary was launched from: running from build/ read build/saves/config.json
// while running from the source root read saves/config.json. Two different
// configs, and a key rebind saved in one was invisible to the other — which is
// exactly how a player ends up unable to move with a config file that looks
// fine.
//
// Resolution mirrors ResourceManager::resolvePath: prefer a saves/ directory
// that already exists at one of the usual roots, and otherwise fall back to the
// working directory so a first run still has somewhere to write.
std::string Serializer::saveDirectory() {
    static const std::string resolved = [] {
        const std::vector<std::string> candidates = {
            "saves", "../saves", "../../saves",
            "SuperMarioGame/saves", "../SuperMarioGame/saves"
        };
        // Never settle on a saves/ that sits inside a CMake build tree. That is
        // how the split happened: launching from build/ found build/saves and
        // wrote settings there, so the same game had two configs and a key
        // rebind made in one was invisible from the other.
        auto insideBuildTree = [](const std::string& dir) {
            std::error_code ec;
            const std::filesystem::path parent = std::filesystem::path(dir).parent_path();
            return std::filesystem::exists(parent / "CMakeCache.txt", ec);
        };

        for (const auto& candidate : candidates) {
            std::error_code ec;
            if (std::filesystem::is_directory(candidate, ec) && !insideBuildTree(candidate)) {
                return candidate;
            }
        }
        return std::string("saves");
    }();
    return resolved;
}

std::string Serializer::getSaveFilePath(int slot) {
    return saveDirectory() + "/slot_" + std::to_string(slot) + ".json";
}

std::string Serializer::getSettingsFilePath() {
    return saveDirectory() + "/config.json";
}

std::string Serializer::getHighScoresFilePath() {
    return saveDirectory() + "/highscores.json";
}

std::vector<HighScoreEntry> Serializer::loadHighScores() {
    std::vector<HighScoreEntry> scores;
    try {
        const std::string path = getHighScoresFilePath();
        if (!std::filesystem::exists(path)) return scores;

        std::ifstream file(path);
        if (!file.is_open()) return scores;

        nlohmann::json j;
        file >> j;
        if (!j.contains("scores") || !j["scores"].is_array()) return scores;

        for (const auto& item : j["scores"]) {
            HighScoreEntry entry;
            entry.score      = item.value("score", 0);
            entry.coins      = item.value("coins", 0);
            entry.starCoins  = item.value("starCoins", 0);
            entry.character  = item.value("character", std::string("mario"));
            entry.levelName  = item.value("levelName", std::string("1-1"));
            entry.timestamp  = item.value("timestamp", std::string(""));
            scores.push_back(std::move(entry));
        }
    } catch (const std::exception& e) {
        std::cerr << "[Serializer] Failed to read high scores: " << e.what() << std::endl;
        scores.clear();
    }
    return scores;
}

bool Serializer::recordHighScore(const HighScoreEntry& entry) {
    // A run worth nothing is not a high score; writing it would just push a real
    // entry off the bottom of the table.
    if (entry.score <= 0) return false;

    try {
        std::vector<HighScoreEntry> scores = loadHighScores();

        HighScoreEntry stamped = entry;
        if (stamped.timestamp.empty()) {
            stamped.timestamp = getCurrentISOTimestamp();
        }
        scores.push_back(std::move(stamped));

        std::stable_sort(scores.begin(), scores.end(),
                         [](const HighScoreEntry& a, const HighScoreEntry& b) {
                             return a.score > b.score;
                         });
        if (static_cast<int>(scores.size()) > MAX_HIGH_SCORES) {
            scores.resize(static_cast<std::size_t>(MAX_HIGH_SCORES));
        }

        const std::string path = getHighScoresFilePath();
        std::filesystem::path fsPath(path);
        if (fsPath.has_parent_path()) {
            std::filesystem::create_directories(fsPath.parent_path());
        }

        nlohmann::json j;
        j["version"] = "1.0";
        j["scores"] = nlohmann::json::array();
        for (const auto& s : scores) {
            nlohmann::json row;
            row["score"]     = s.score;
            row["coins"]     = s.coins;
            row["starCoins"] = s.starCoins;
            row["character"] = s.character;
            row["levelName"] = s.levelName;
            row["timestamp"] = s.timestamp;
            j["scores"].push_back(std::move(row));
        }

        std::ofstream file(path);
        if (!file.is_open()) return false;
        file << j.dump(4);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[Serializer] Failed to write high score: " << e.what() << std::endl;
        return false;
    }
}

bool Serializer::saveGame(int slot, const Player& player, int levelId, const std::string& levelName, 
                         float timeRemaining, float checkpointX, float checkpointY, 
                         const std::vector<bool>& starCoinsCollected) {
    try {
        std::string path = getSaveFilePath(slot);
        std::filesystem::path fsPath(path);
        if (fsPath.has_parent_path()) {
            std::filesystem::create_directories(fsPath.parent_path());
        }

        nlohmann::json j;
        j["version"] = "2.0";
        j["timestamp"] = getCurrentISOTimestamp();

        // 1. Player Data
        j["player"]["character"] = getCharacterName(player);
        j["player"]["state"] = getPlayerStateName(player);
        j["player"]["position"]["x"] = player.getPosition().x;
        j["player"]["position"]["y"] = player.getPosition().y;
        j["player"]["lives"] = player.getLives();
        j["player"]["score"] = player.getScore();
        j["player"]["coins"] = player.getCoins();

        // 2. Level Data
        j["level"]["id"] = levelId;
        j["level"]["name"] = levelName;
        j["level"]["timeRemaining"] = timeRemaining;
        j["level"]["checkpoint"]["x"] = checkpointX;
        j["level"]["checkpoint"]["y"] = checkpointY;
        j["level"]["starCoinsCollected"] = starCoinsCollected;

        // 3. Progress Data (derived dynamically from level progress and achievements)
        std::vector<int> completed;
        for (int i = 1; i < levelId; ++i) {
            completed.push_back(i);
        }
        j["progress"]["levelsCompleted"] = completed;
        
        std::vector<std::string> unlockedChars = { "mario", "luigi" };
        if (AchievementManager::getInstance().isUnlocked("toad")) unlockedChars.push_back("toad");
        if (AchievementManager::getInstance().isUnlocked("peach")) unlockedChars.push_back("peach");
        j["progress"]["unlockedCharacters"] = unlockedChars;
        j["progress"]["starCoins"][std::to_string(levelId)] = starCoinsCollected;

        // 4. Statistics Data
        const auto& stats = StatisticsTracker::getInstance().getStats();
        j["statistics"]["totalEnemiesDefeated"] = stats.totalEnemiesDefeated;
        j["statistics"]["totalCoinsCollected"] = stats.totalCoinsCollected;
        j["statistics"]["totalDeaths"] = stats.totalDeaths;
        j["statistics"]["totalTimePlayed"] = stats.totalTimePlayed;
        j["statistics"]["highestCombo"] = stats.highestCombo;

        // 5. Achievements
        j["achievements"] = AchievementManager::getInstance().getUnlockedIds();

        // 6. Settings (Volume and defaults)
        float sfx = 80.0f, music = 60.0f;
        std::string diff = "normal";
        std::unordered_map<std::string, std::string> bindings;
        std::unordered_map<std::string, std::string> bindings2;
        bool colorblind = false;
        loadSettings(sfx, music, diff, bindings, bindings2, colorblind);

        j["settings"]["sfxVolume"] = sfx;
        j["settings"]["musicVolume"] = music;
        j["settings"]["difficulty"] = diff;
        j["settings"]["keyBindings"] = bindings;
        j["settings"]["keyBindings2"] = bindings2;
        j["settings"]["colorblindMode"] = colorblind;

        std::ofstream file(path);
        if (!file.is_open()) return false;
        file << j.dump(4);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Serializer::saveGame failed: " << e.what() << std::endl;
        return false;
    }
}

bool Serializer::loadGame(int slot, std::unique_ptr<Player>& player, int& levelId, std::string& levelName, 
                         float& timeRemaining, float& checkpointX, float& checkpointY, 
                         std::vector<bool>& starCoinsCollected) {
    try {
        std::string path = getSaveFilePath(slot);
        if (!std::filesystem::exists(path)) return false;

        std::ifstream file(path);
        if (!file.is_open()) return false;

        nlohmann::json j;
        file >> j;

        // 1. Reconstruct Player character type
        std::string charName = j["player"]["character"];
        float px = j["player"]["position"]["x"];
        float py = j["player"]["position"]["y"];

        if (charName == "mario") player = std::make_unique<Mario>(sf::Vector2f(px, py));
        else if (charName == "luigi") player = std::make_unique<Luigi>(sf::Vector2f(px, py));
        else if (charName == "toad") player = std::make_unique<Toad>(sf::Vector2f(px, py));
        else if (charName == "peach") player = std::make_unique<Peach>(sf::Vector2f(px, py));
        else player = std::make_unique<Mario>(sf::Vector2f(px, py));

        // Restore attributes directly (without emitting gameplay events)
        player->coins = j["player"]["coins"];
        player->score = j["player"]["score"];
        player->lives = j["player"]["lives"];

        // Restore state, THEN the position.
        //
        // Changing the base form runs applyStateSize(), which shifts position.y
        // by the height difference so the feet stay planted — correct when a
        // mushroom makes you grow mid-level, wrong when restoring a snapshot
        // whose position already belongs to that form. Loading a saved Super
        // Mario therefore moved him by the Small/Super height difference, and
        // every save/load round trip drifted him further.
        std::string stateName = j["player"]["state"];
        restorePlayerState(*player, stateName);
        player->setPosition(sf::Vector2f(px, py));

        // 2. Restore level parameters
        levelId = j["level"]["id"];
        levelName = j["level"]["name"];
        timeRemaining = j["level"]["timeRemaining"];
        checkpointX = j["level"]["checkpoint"]["x"];
        checkpointY = j["level"]["checkpoint"]["y"];
        starCoinsCollected = j["level"]["starCoinsCollected"].get<std::vector<bool>>();

        // 3. Restore Statistics
        GameStatistics stats;
        stats.totalEnemiesDefeated = j["statistics"]["totalEnemiesDefeated"];
        stats.totalCoinsCollected = j["statistics"]["totalCoinsCollected"];
        stats.totalDeaths = j["statistics"]["totalDeaths"];
        stats.totalTimePlayed = j["statistics"]["totalTimePlayed"];
        stats.highestCombo = j["statistics"]["highestCombo"];
        StatisticsTracker::getInstance().setStats(stats);

        // 4. Restore Achievements
        std::vector<std::string> unlockedIds = j["achievements"].get<std::vector<std::string>>();
        AchievementManager::getInstance().setUnlockedAchievements(unlockedIds);

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Serializer::loadGame failed: " << e.what() << std::endl;
        return false;
    }
}

SaveSlotPreview Serializer::getSlotPreview(int slot) {
    SaveSlotPreview preview;
    try {
        std::string path = getSaveFilePath(slot);
        if (!std::filesystem::exists(path)) return preview;

        std::ifstream file(path);
        if (!file.is_open()) return preview;

        nlohmann::json j;
        file >> j;

        preview.exists = true;
        preview.character = j["player"]["character"];
        preview.levelId = j["level"]["id"];
        preview.levelName = j["level"]["name"];
        preview.score = j["player"]["score"];
        
        std::vector<bool> coins = j["level"]["starCoinsCollected"].get<std::vector<bool>>();
        int starCoinsCount = 0;
        for (bool c : coins) {
            if (c) starCoinsCount++;
        }
        preview.starCoinsCount = starCoinsCount;
        preview.playTime = j["statistics"]["totalTimePlayed"];
        preview.timestamp = j["timestamp"];
    } catch (...) {
        preview.exists = false;
    }
    return preview;
}

bool Serializer::deleteSlot(int slot) {
    try {
        std::string path = getSaveFilePath(slot);
        if (std::filesystem::exists(path)) {
            return std::filesystem::remove(path);
        }
    } catch (...) {
        return false;
    }
    return false;
}

bool Serializer::saveSettings(float sfxVolume, float musicVolume, const std::string& difficulty,
                             const std::unordered_map<std::string, std::string>& keyBindings,
                             const std::unordered_map<std::string, std::string>& keyBindings2,
                             bool colorblindMode) {
    try {
        std::string path = getSettingsFilePath();
        std::filesystem::path fsPath(path);
        if (fsPath.has_parent_path()) {
            std::filesystem::create_directories(fsPath.parent_path());
        }

        nlohmann::json j;
        j["sfxVolume"] = sfxVolume;
        j["musicVolume"] = musicVolume;
        j["difficulty"] = difficulty;
        j["keyBindings"] = keyBindings;
        j["keyBindings2"] = keyBindings2;
        j["colorblindMode"] = colorblindMode;

        std::ofstream file(path);
        if (!file.is_open()) return false;
        file << j.dump(4);
        return true;
    } catch (...) {
        return false;
    }
}

bool Serializer::loadSettings(float& sfxVolume, float& musicVolume, std::string& difficulty,
                             std::unordered_map<std::string, std::string>& keyBindings,
                             std::unordered_map<std::string, std::string>& keyBindings2,
                             bool& colorblindMode) {
    try {
        std::string path = getSettingsFilePath();
        if (!std::filesystem::exists(path)) {
            // Default mappings
            sfxVolume = 80.0f;
            musicVolume = 60.0f;
            difficulty = "normal";
            keyBindings = { {"jump", "W"}, {"left", "A"}, {"right", "D"}, {"fire", "F"} };
            // Empty rather than a default layout: InputManager already ships the
            // arrow-key defaults, and applyBindings() only overrides what it is
            // given. Naming them here would be a second source of truth.
            keyBindings2.clear();
            colorblindMode = false;
            return true;
        }

        std::ifstream file(path);
        if (!file.is_open()) return false;

        nlohmann::json j;
        file >> j;

        sfxVolume = j["sfxVolume"];
        musicVolume = j["musicVolume"];
        difficulty = j["difficulty"];
        keyBindings = j["keyBindings"].get<std::unordered_map<std::string, std::string>>();
        // Optional: a config.json written before Player 2 was configurable has no
        // such key, and must not be an exception that discards every setting.
        keyBindings2.clear();
        if (j.contains("keyBindings2")) {
            keyBindings2 = j["keyBindings2"].get<std::unordered_map<std::string, std::string>>();
        }
        if (j.contains("colorblindMode")) {
            colorblindMode = j["colorblindMode"];
        } else {
            colorblindMode = false;
        }
        return true;
    } catch (...) {
        return false;
    }
}
