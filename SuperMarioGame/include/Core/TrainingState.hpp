#pragma once

#include "Core/IGameState.hpp"
#include "Entities/AIController.hpp"
#include "Entities/PolicyTrainer.hpp"
#include "Entities/RewardTracker.hpp"
#include "Graphics/Camera.hpp"
#include "Physics/PhysicsEngine.hpp"
#include "Utils/LevelLoader.hpp"
#include "Utils/TileMap.hpp"

#include <memory>
#include <string>
#include <vector>

class NeuralPolicy;
class Player;

// Watches a neural policy learn to play, live.
//
// Why a state of its own rather than a mode inside PlayingState — the same
// reason tools/eval_level.cpp is a separate harness: training runs episodes
// back to back with resets, bookkeeping and an optimiser step between them,
// none of which PlayingState was built for, and its update path also writes
// save files, pushes states on death and victory, and snapshots the whole world
// every frame for the rewind buffer. All correct for playing; all wrong for
// running thousands of episodes.
//
// What it shows, and why each panel earns its space:
//   - the episode itself, so the behaviour being learned is visible rather than
//     inferred from a number;
//   - agreement with the teacher, which is the metric that actually says
//     whether it is learning;
//   - the loss curve, which says whether the optimiser is healthy;
//   - the seven button outputs, which is where a policy that refuses to jump
//     shows a 0.49 that should be a 1.0;
//   - beta, so it is clear who is driving at any moment.
class TrainingState : public IGameState {
public:
    explicit TrainingState(int levelIndex = 0);
    ~TrainingState() override;

    void enter() override;
    void exit() override;
    void handleInput(const sf::Event& event) override;
    void update(float dt) override;
    void render(sf::RenderTarget& target) override;

private:
    void startEpisode();
    void finishEpisode(const char* reason);
    void renderOverlay(sf::RenderTarget& target);
    void renderCurve(sf::RenderTarget& target, const std::vector<float>& series,
                     sf::Vector2f topLeft, sf::Vector2f size, sf::Color color,
                     float fixedMax) const;

    // World
    TileMap m_tileMap;
    std::vector<std::unique_ptr<Entity>> m_entities;
    PhysicsEngine m_physics;
    Camera m_camera;
    LevelLoader m_loader;
    LevelData m_levelData;
    Player* m_player = nullptr;
    std::string m_levelPath;

    // Learner
    std::unique_ptr<NeuralPolicy> m_policy;
    std::unique_ptr<PolicyTrainer> m_trainer;
    // The teacher. Kept as a full AIController rather than a bare policy so the
    // observation the learner trains on is byte-identical to the one the game
    // feeds a policy during play — sensing is the controller's job, and
    // duplicating it here is how the two would silently drift apart.
    std::unique_ptr<AIController> m_agent;
    // The teacher, as a policy rather than a controller: sensing is the
    // controller's job and duplicating it would let the observation the learner
    // trains on drift from the one the game feeds a policy during play.
    std::unique_ptr<class HeuristicPolicy> m_teacherPolicy;
    bool m_teacherDriving = true;

    // The reinforcement signal, assembled from events the game already
    // publishes. Not consulted at all during the imitation phase.
    RewardTracker m_reward;

    // Episode bookkeeping
    float m_episodeTime = 0.0f;
    float m_maxEpisodeSeconds = 60.0f;
    float m_furthestX = 0.0f;
    float m_stallTime = 0.0f;
    const char* m_lastOutcome = "—";
    int m_completions = 0;

    // Presentation
    bool m_paused = false;
    // Episodes per rendered frame. 1 shows every frame of play; higher values
    // trade watchability for training throughput, which is the whole tuning
    // knob when the point is both to learn and to be seen learning.
    int m_stepsPerFrame = 1;
    float m_blinkPhase = 0.0f;

    // Whether to draw the world this frame.
    //
    // Rendering a level nobody is watching closely is wasted work, and at high
    // speed it is also misleading: 64 simulated steps between frames means the
    // picture skips most of what happened. So above 1x the world is drawn only
    // when something substantial has changed — a new furthest point, or the
    // switch from imitation to reinforcement — and otherwise the panel alone is
    // updated. At 1x it always draws, because then the point IS to watch.
    bool m_renderWorld = true;
    float m_bestProgressX = 0.0f;
    float m_showWorldUntil = 0.0f;   // seconds of wall time to keep showing
    float m_wallClock = 0.0f;
    bool m_sawReinforcePhase = false;
};
