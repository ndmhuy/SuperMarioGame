#pragma once

// What kind of match PlayingState is running.
//
// Two-player mode used to be a single `bool twoPlayer` threaded through the
// PlayingState constructor. That was enough while "two players" meant exactly
// one thing, but it cannot distinguish a human opponent from a CPU one, versus
// from co-op, or either from Shadow Mario — which is a second body on screen
// that is not a participant at all. Naming the mode once, here, keeps those
// distinctions out of the boolean-argument list and lets the front-end states
// pass a value the HUD and the AI can both read.
enum class GameMode {
    SinglePlayer,   // one human, the original campaign
    VersusHuman,    // two humans, shared screen, combat on
    VersusCPU,      // human vs AIController-driven opponent
    CoopHuman,      // two humans, friendly fire off, shared lives and score
    ShadowChase     // one human, chased by a 3-second-delayed replay of itself
};

// How sharp the AI opponent is. Drives the observation grid, the reaction
// latency and the action noise — see docs/two_player_ai_plan.md §2A.
enum class AIDifficulty { Easy, Normal, Hard };

// What the AI opponent wants. Drives the utility weights the policy scores
// candidate actions with — see docs/two_player_ai_plan.md §2B.
enum class AIArchetype { Speedrunner, Hunter, Collector };

// Everything the front-end has to decide before a level starts.
//
// Passed by value: it is four ints wide, it is read on level setup rather than
// per frame, and a copy per state transition is cheaper than the indirection.
struct MatchConfig {
    GameMode mode = GameMode::SinglePlayer;
    AIDifficulty aiDifficulty = AIDifficulty::Normal;
    AIArchetype aiArchetype = AIArchetype::Speedrunner;

    // Is there a second scoring participant? Shadow Mario is deliberately
    // excluded: it has no lives, no score and cannot win.
    bool hasSecondPlayer() const {
        return mode == GameMode::VersusHuman || mode == GameMode::VersusCPU ||
               mode == GameMode::CoopHuman;
    }
    // Do the two participants fight each other?
    bool isVersus() const {
        return mode == GameMode::VersusHuman || mode == GameMode::VersusCPU;
    }
    bool isCoop() const { return mode == GameMode::CoopHuman; }
    // Is the second participant driven by an AIController rather than a keyboard?
    bool isCpuOpponent() const { return mode == GameMode::VersusCPU; }
    bool isShadowChase() const { return mode == GameMode::ShadowChase; }
};

inline const char* toString(AIDifficulty difficulty) {
    switch (difficulty) {
        case AIDifficulty::Easy:   return "EASY";
        case AIDifficulty::Normal: return "NORMAL";
        case AIDifficulty::Hard:   return "HARD";
    }
    return "NORMAL";
}

inline const char* toString(AIArchetype archetype) {
    switch (archetype) {
        case AIArchetype::Speedrunner: return "SPEEDRUNNER";
        case AIArchetype::Hunter:      return "HUNTER";
        case AIArchetype::Collector:   return "COLLECTOR";
    }
    return "SPEEDRUNNER";
}

inline const char* toString(GameMode mode) {
    switch (mode) {
        case GameMode::SinglePlayer: return "1 PLAYER";
        case GameMode::VersusHuman:  return "VERSUS";
        case GameMode::VersusCPU:    return "VERSUS CPU";
        case GameMode::CoopHuman:    return "CO-OP";
        case GameMode::ShadowChase:  return "SHADOW CHASE";
    }
    return "1 PLAYER";
}
