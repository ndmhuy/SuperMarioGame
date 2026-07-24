#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <nlohmann/json.hpp>

class Player;

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
    static std::string getSettingsFilePath();
};
