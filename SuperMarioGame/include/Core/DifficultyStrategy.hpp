#pragma once

#include <memory>
#include <string>

// Task 9.4 — difficulty as a Strategy.
//
// Game has stored a difficulty string since save/load was written, Serializer
// has persisted it, and the options screen edits it. Nothing has ever read it:
// before this, picking Hard changed a word in config.json and nothing else.
//
// Each concrete strategy answers the same questions with different numbers, so
// adding a fourth difficulty means adding a class, not editing switches spread
// across the level loader, the player and the bosses (OCP).
class IDifficultyStrategy {
public:
    virtual ~IDifficultyStrategy() = default;

    // Persisted id ("easy" / "normal" / "hard") and the label a menu shows.
    virtual std::string getId() const = 0;
    virtual std::string getDisplayName() const = 0;

    // Multiplies every enemy's speed as it enters the world.
    virtual float enemySpeedScale() const = 0;
    // Lives a fresh run starts with.
    virtual int startingLives() const = 0;
    // Multiplies the per-level clock.
    virtual float levelTimeScale() const = 0;
    // Multiplies a boss's health bar, rounded to at least one hit.
    virtual float bossHealthScale() const = 0;

    // Builds the strategy for a persisted id. Unknown ids fall back to Normal
    // rather than throwing: config.json is user-editable.
    static std::unique_ptr<IDifficultyStrategy> fromId(const std::string& id);
};

class EasyDifficulty : public IDifficultyStrategy {
public:
    std::string getId() const override { return "easy"; }
    std::string getDisplayName() const override { return "EASY"; }
    float enemySpeedScale() const override { return 0.80f; }
    int   startingLives() const override { return 5; }
    float levelTimeScale() const override { return 1.30f; }
    float bossHealthScale() const override { return 0.70f; }
};

class NormalDifficulty : public IDifficultyStrategy {
public:
    std::string getId() const override { return "normal"; }
    std::string getDisplayName() const override { return "NORMAL"; }
    float enemySpeedScale() const override { return 1.00f; }
    int   startingLives() const override { return 3; }
    float levelTimeScale() const override { return 1.00f; }
    float bossHealthScale() const override { return 1.00f; }
};

class HardDifficulty : public IDifficultyStrategy {
public:
    std::string getId() const override { return "hard"; }
    std::string getDisplayName() const override { return "HARD"; }
    float enemySpeedScale() const override { return 1.30f; }
    int   startingLives() const override { return 2; }
    float levelTimeScale() const override { return 0.75f; }
    float bossHealthScale() const override { return 1.40f; }
};
