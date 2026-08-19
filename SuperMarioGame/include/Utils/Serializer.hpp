#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <nlohmann/json.hpp>

class Player;

// One row of the high-score table shown by the Options & High Scores screen.
struct HighScoreEntry {
    int score = 0;
    int coins = 0;
    int starCoins = 0;
    std::string character = "mario";
    std::string levelName = "1-1";
    std::string timestamp = "";
};

struct SaveSlotPreview {
    bool exists = false;
    std::string character = "mario";
    int levelId = 1;
    std::string levelName = "";
    int score = 0;
    int starCoinsCount = 0;
    float playTime = 0.0f;
    std::string timestamp = "";
};

class Serializer {
public:
    // Game progress save/load
    static bool saveGame(int slot, const Player& player, int levelId, const std::string& levelName, 
                         float timeRemaining, float checkpointX, float checkpointY, 
                         const std::vector<bool>& starCoinsCollected);

    static bool loadGame(int slot, std::unique_ptr<Player>& player, int& levelId, std::string& levelName, 
                         float& timeRemaining, float& checkpointX, float& checkpointY, 
                         std::vector<bool>& starCoinsCollected);

    static SaveSlotPreview getSlotPreview(int slot);

    // The one directory every save file lives in, resolved once. Public because
    // CampaignProgress writes alongside these files and must not resolve its own
    // — see the comment on the definition: bare relative paths gave you a
    // different config depending on the working directory.
    static std::string saveDirectory();

    // High-score table (saves/highscores.json). recordHighScore() merges the
    // entry in, keeps the list sorted descending and truncates to MAX_HIGH_SCORES,
    // so callers never have to read-modify-write it themselves.
    static constexpr int MAX_HIGH_SCORES = 10;
    static bool recordHighScore(const HighScoreEntry& entry);
    static std::vector<HighScoreEntry> loadHighScores();
    static bool deleteSlot(int slot);

    // Settings save/load
    static bool saveSettings(float sfxVolume, float musicVolume, const std::string& difficulty, 
                             const std::unordered_map<std::string, std::string>& keyBindings,
                             bool colorblindMode);

    static bool loadSettings(float& sfxVolume, float& musicVolume, std::string& difficulty, 
                             std::unordered_map<std::string, std::string>& keyBindings,
                             bool& colorblindMode);

private:
    static std::string getSaveFilePath(int slot);
    static std::string getHighScoresFilePath();
    static std::string getSettingsFilePath();
};