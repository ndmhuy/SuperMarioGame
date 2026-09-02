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

    // The one-line "what is in this slot" string: character, level, score, star
    // coins and play time (SPEC 12.3) — or "EMPTY".
    //
    // It lives on the struct rather than in MenuState.cpp, where it started,
    // because two screens now have to answer the same question: LOAD GAME says
    // what you would resume, and the pause menu's slot picker says what you
    // would OVERWRITE. Two formatters would let the picker describe a slot
    // differently from the page the player just read it on, which is exactly
    // the kind of disagreement that makes a destructive choice untrustworthy.
    std::string summary() const;
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

    // Point every save file at `directory` instead of the auto-resolved one.
    //
    // This exists for the tests, and it is not a convenience — it is the fix for
    // a suite that wrote to, and deleted from, the developer's real saves/.
    // CampaignProgress::reset() calls std::filesystem::remove() on a path that
    // resolves through saveDirectory(), so a regression case testing "progress
    // can be reset" was erasing actual campaign progress; that was observed
    // live, with a seeded progress.json vanishing between two game launches.
    // Meanwhile the high-score case asserted against whatever table happened to
    // be on disk, so it passed under ctest and failed when the same binary was
    // run from build/ — the two have different working directories.
    //
    // g-rule-13: a test must never assert against machine state. Every verify_*
    // harness now points this at a fresh temporary directory before it runs, so
    // no test can reach real save data and every one starts from empty.
    //
    // Call it before anything reads a save path. Passing an empty string
    // restores the automatic resolution.
    static void setSaveDirectory(const std::string& directory);

    // --- Player profile: achievements and lifetime statistics ---------------
    //
    // These were serialized ONLY inside a level save slot, and the only code path
    // that read a slot back is PlayingState::loadFromSlot — whose only caller is
    // the ImGui dev panel. So achievements and statistics were written to disk on
    // every autosave and never loaded by the running game: every launch started
    // with an empty achievement set and zeroed counters, and because Toad and
    // Peach are gated on the "toad"/"peach" achievements, both characters were
    // permanently locked no matter what the player had done.
    //
    // A profile is separate from a save slot on purpose. Achievements and
    // lifetime totals belong to the player, not to one run, and must survive
    // starting a new game. Same directory and same shape as progress.json, which
    // is the one piece of persistence that already worked.
    static bool saveProfile();
    static bool loadProfile();

    // High-score table (saves/highscores.json). recordHighScore() merges the
    // entry in, keeps the list sorted descending and truncates to MAX_HIGH_SCORES,
    // so callers never have to read-modify-write it themselves.
    static constexpr int MAX_HIGH_SCORES = 10;
    static bool recordHighScore(const HighScoreEntry& entry);
    static std::vector<HighScoreEntry> loadHighScores();
    // Empty the table. Action-oriented on purpose: the callers that need this —
    // tests wanting a known starting point, and any future "erase records"
    // option — want the table cleared, not the path it lives at. Handing out
    // getHighScoresFilePath() would let any caller write the file itself.
    static bool clearHighScores();
    static bool deleteSlot(int slot);

    // Settings save/load.
    //
    // `keyBindings2` is Player 2's pad. Written under its own "keyBindings2" key
    // and treated as optional on load, so a config.json written before two
    // players were configurable still parses and simply keeps the built-in
    // arrow-key layout.
    //
    // `debugMode` is optional on load in exactly the same way and defaults to
    // FALSE when absent — the developer ImGui surfaces must stay off for anyone
    // whose config.json predates the setting.
    static bool saveSettings(float sfxVolume, float musicVolume, const std::string& difficulty,
                             const std::unordered_map<std::string, std::string>& keyBindings,
                             const std::unordered_map<std::string, std::string>& keyBindings2,
                             bool colorblindMode, bool debugMode);

    static bool loadSettings(float& sfxVolume, float& musicVolume, std::string& difficulty,
                             std::unordered_map<std::string, std::string>& keyBindings,
                             std::unordered_map<std::string, std::string>& keyBindings2,
                             bool& colorblindMode, bool& debugMode);

private:
    static std::string getSaveFilePath(int slot);
    static std::string getHighScoresFilePath();
    static std::string getSettingsFilePath();
};