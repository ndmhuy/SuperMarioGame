#pragma once

#include "Core/GameMode.hpp"
#include "Entities/ExperienceLog.hpp"
#include "Entities/IAIPolicy.hpp"
#include "Entities/RewardTracker.hpp"

#include <SFML/System/Vector2.hpp>
#include <memory>
#include <random>
#include <string>
#include <vector>

class Entity;
class Player;
class TileMap;

// Drives a Player from code instead of from a keyboard.
//
// The controller owns the two halves the game loop cares about — sensing the
// world into an AIObservation, and turning the chosen AIAction back into calls
// on the Player — and delegates the decision itself to an IAIPolicy. That split
// is what lets the reinforcement-learning side project drop a trained network in
// without touching anything here (see IAIPolicy.hpp).
//
// It deliberately does NOT go through InputManager. A bot has no keys; routing
// it through the binding tables would mean synthesising key events for a player
// index that has no keyboard, and Player already exposes the same verbs the
// commands call.
//
// Not an IMovementStrategy: that interface takes an `Enemy&`, is assigned in
// enemy constructors, and would have to be widened across all eight concrete
// strategies to drive a Player. A separate controller is the cheaper seam.
class AIController {
public:
    AIController(Player& controlled, AIDifficulty difficulty, AIArchetype archetype);
    ~AIController();

    // One frame. `opponent` may be null — a bot playing a level alone still has
    // a goal to run towards. Nothing is mutated except the controlled player.
    void update(float dt, const Player* opponent, const TileMap& tileMap,
                const std::vector<std::unique_ptr<Entity>>& entities);

    void setDifficulty(AIDifficulty difficulty);
    void setArchetype(AIArchetype archetype);
    AIDifficulty getDifficulty() const { return m_difficulty; }
    AIArchetype getArchetype() const { return m_archetype; }

    // Swap the brain, keeping the senses. This is the entire integration surface
    // the neural policy needs.
    void setPolicy(std::unique_ptr<IAIPolicy> policy);

    // --- Dev panel / debug overlay ------------------------------------------
    // What the bot is doing and why, as a short line for the HUD and overlay.
    const char* policyName() const;
    const char* reason() const { return m_reason; }
    // The last observation, for the overlay that draws the vision grid.
    const AIObservation& lastObservation() const { return m_observation; }
    // Live tunables, so the numbers in the difficulty table can be felt rather
    // than argued about. Latency is seconds between decisions; noise is the
    // epsilon that randomises a button.
    float getReactionLatency() const { return m_reactionLatency; }
    void setReactionLatency(float seconds);
    float getActionNoise() const { return m_actionNoise; }
    void setActionNoise(float epsilon);
    int getVisionRadius() const { return m_visionRadiusX; }

    // Freeze/thaw. The pause overlay stops the simulation but not this object's
    // decision clock; without this the bot banks however long the game sat
    // paused and fires a burst of decisions on resume.
    void setPaused(bool paused) { m_paused = paused; }

    // Forget per-episode state — called on respawn and on level load.
    void reset();

    // --- Reinforcement learning (A/rl-neural-policy) ------------------------
    //
    // Off by default and inert until switched on, so an ordinary match pays
    // nothing for it. Turning it on starts crediting reward from the EventBus
    // and writing one transition per decision to `logPath` — which is the
    // dataset a trainer consumes. Pass an empty path to score without logging.
    void enableLearning(const std::string& logPath = {});
    bool isLearning() const { return m_learning; }

    // Reward earned this episode, for the dev overlay. Watching this number
    // while the bot plays is the fastest way to tell a badly shaped reward from
    // a badly trained policy.
    float episodeReward() const { return m_reward.episodeTotal(); }
    // Tunable weights — the main lever for anyone shaping the reward.
    RewardTracker::Weights& rewardWeights() { return m_reward.weights(); }
    std::size_t transitionsLogged() const { return m_experience.rowsWritten(); }

private:
    // Read the world into m_observation.
    void scanEnvironment(const Player* opponent, const TileMap& tileMap,
                         const std::vector<std::unique_ptr<Entity>>& entities);
    // Apply m_action to the controlled player.
    void actuate();
    // Epsilon-greedy exploration, applied outside the policy so it behaves
    // identically for a heuristic and for a network (and so it maps onto the
    // training-time exploration schedule unchanged).
    void applyNoise(AIAction& action);
    // Difficulty table from docs/two_player_ai_plan.md §2A.
    void applyDifficultyProfile();

    Player& m_player;
    std::unique_ptr<IAIPolicy> m_policy;

    AIDifficulty m_difficulty;
    AIArchetype m_archetype;

    // Derived from the difficulty by applyDifficultyProfile(), then overridable
    // from the dev panel.
    int m_visionRadiusX = 5;
    int m_visionRadiusY = 5;
    float m_reactionLatency = 0.12f;
    float m_actionNoise = 0.05f;
    bool m_allowRun = true;
    bool m_allowShoot = true;
    bool m_allowGroundPound = false;

    float m_decisionTimer = 0.0f;
    bool m_paused = false;

    AIObservation m_observation;
    // The decision is held between decisions: at Easy's 400ms latency the bot
    // keeps holding whatever it last chose, which is what makes it feel slow to
    // react rather than paralysed.
    AIAction m_action;
    const char* m_reason = "idle";

    // Deterministic per-controller, so a run is reproducible and a bug in bot
    // behaviour can be watched twice. Seeded from the archetype and difficulty
    // rather than from a clock.
    std::mt19937 m_rng;
    std::uniform_real_distribution<float> m_unit{0.0f, 1.0f};

    // --- Reinforcement learning --------------------------------------------
    bool m_learning = false;
    RewardTracker m_reward;
    ExperienceLog m_experience;
};
