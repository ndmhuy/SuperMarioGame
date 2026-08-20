// Headless policy training: no window, no GL context, no ImGui.
//
// TrainingState renders while it trains, which is the point of it — learning
// should be watchable. But a window is exactly wrong for a long run: it costs
// frame budget on a machine that may be weak, it pins the run to a visible
// desktop session, and it caps the simulation at whatever the renderer can
// keep up with. This is the same training loop with the screen taken away.
//
//   ./train_policy --episodes 400
//   ./train_policy --episodes 800 --level assets/levels/level_2.json
//   ./train_policy --episodes 200 --imitation-only
//
// Shares PolicyTrainer, NeuralPolicy, HeuristicPolicy and AIController with the
// in-game path, so what is trained here is what plays there. The episode loop
// mirrors tools/eval_level.cpp, for the reasons documented at the top of that
// file: PlayingState cannot be driven headlessly.

#include "Core/EventBus.hpp"
#include "Core/Game.hpp"
#include "Core/GameMode.hpp"
#include "Core/SoundManager.hpp"
#include "Entities/AIController.hpp"
#include "Entities/BorrowedPolicy.hpp"
#include "Entities/HeuristicPolicy.hpp"
#include "Entities/Mario.hpp"
#include "Entities/NeuralPolicy.hpp"
#include "Entities/Player.hpp"
#include "Entities/PolicyTrainer.hpp"
#include "Entities/RewardTracker.hpp"
#include "Physics/PhysicsEngine.hpp"
#include "Utils/Constants.hpp"
#include "Utils/LevelLoader.hpp"
#include "Utils/TileMap.hpp"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace {

constexpr float kDt = Constants::FIXED_TIMESTEP;
const char* kImitationCheckpoint = "saves/ai/policy_imitation.ckpt";
const char* kReinforceCheckpoint = "saves/ai/policy_reinforce.ckpt";

struct Options {
    std::string levelPath = "assets/levels/level_1.json";
    int episodes = 300;
    float maxEpisodeSeconds = 60.0f;
    bool imitationOnly = false;
    bool resume = false;
};

void usage() {
    std::cerr <<
        "usage: train_policy [options]\n\n"
        "  --level <path>        level to train on (default: level_1)\n"
        "  --episodes <n>        episodes to run (default: 300)\n"
        "  --seconds <n>         max seconds per episode (default: 60)\n"
        "  --imitation-only      stop before the reinforcement phase\n"
        "  --resume              start from the existing imitation checkpoint\n";
}

bool parseArgs(int argc, char** argv, Options& opt) {
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        auto next = [&]() -> const char* { return (i + 1 < argc) ? argv[++i] : nullptr; };
        if (arg == "--level")            { const char* v = next(); if (!v) return false; opt.levelPath = v; }
        else if (arg == "--episodes")    { const char* v = next(); if (!v) return false; opt.episodes = std::atoi(v); }
        else if (arg == "--seconds")     { const char* v = next(); if (!v) return false; opt.maxEpisodeSeconds = std::strtof(v, nullptr); }
        else if (arg == "--imitation-only") opt.imitationOnly = true;
        else if (arg == "--resume")         opt.resume = true;
        else { usage(); return false; }
    }
    return true;
}

} // namespace

int main(int argc, char** argv) {
    Options opt;
    if (!parseArgs(argc, argv, opt)) return 2;

    SoundManager::getInstance().setSFXVolume(0.0f);
    SoundManager::getInstance().setMusicVolume(0.0f);

    NeuralPolicy policy;
    if (opt.resume && policy.load(kImitationCheckpoint)) {
        std::cout << "[train] resumed from " << kImitationCheckpoint << std::endl;
    }

    PolicyTrainer::Config config;
    if (opt.imitationOnly) config.imitationEpisodes = opt.episodes + 1;
    PolicyTrainer trainer(policy, config);
    trainer.openLog("saves/ai/training_log.csv");

    HeuristicPolicy teacherPolicy(AIArchetype::Speedrunner);

    // Same rebalanced weights the in-game trainer uses; see TrainingState.
    RewardTracker reward;
    RewardTracker::Weights& weights = reward.weights();
    weights.progressPerPixel = 0.1f;
    weights.died = -10.0f;
    weights.timeStep = -0.02f;

    LevelLoader loader;
    PhysicsEngine physics;

    const auto wallStart = std::chrono::steady_clock::now();
    int completions = 0;
    double progressSum = 0.0;

    for (int episode = 0; episode < opt.episodes; ++episode) {
        TileMap tileMap;
        LevelData levelData;
        if (!loader.loadLevel(opt.levelPath, tileMap, levelData)) {
            std::cerr << "[train] cannot load " << opt.levelPath << std::endl;
            return 1;
        }

        std::vector<std::unique_ptr<Entity>> entities = std::move(levelData.entities);
        auto owned = std::make_unique<Mario>(levelData.spawnPoint);
        Player* player = owned.get();
        entities.insert(entities.begin(), std::move(owned));

        // Non-optional: seventeen entity and strategy files reach for
        // Game::getInstance().getNearestPlayer() or getTileMap() from inside
        // their update. Without these every enemy stands still and the agent
        // trains against a world containing no threats.
        Game::getInstance().setPlayer(player);
        Game::getInstance().setTileMap(&tileMap);

        AIController agent(*player, AIDifficulty::Hard, AIArchetype::Speedrunner);
        const bool teacherDrives = trainer.teacherDrives();
        IAIPolicy& driver = teacherDrives ? static_cast<IAIPolicy&>(teacherPolicy)
                                          : static_cast<IAIPolicy&>(policy);
        agent.setPolicy(std::make_unique<BorrowedPolicy>(driver));
        teacherPolicy.reset();
        reward.reset(levelData.spawnPoint);

        const float levelWidth = tileMap.getWidth() * Constants::TILE_SIZE;
        const float bottomVoid = tileMap.getHeight() * Constants::TILE_SIZE + 32.0f;
        const float startX = levelData.spawnPoint.x;
        float simTime = 0.0f, furthest = startX, stall = 0.0f;
        const char* outcome = "timeout";

        while (simTime < opt.maxEpisodeSeconds) {
            agent.update(kDt, nullptr, tileMap, entities);
            const AIObservation& observation = agent.lastObservation();

            if (trainer.mode() == PolicyTrainer::Mode::Imitation) {
                const AIAction teacherAction = teacherDrives ? agent.lastAction()
                                                             : teacherPolicy.decide(observation);
                trainer.learn(observation, teacherAction);
            } else {
                agent.overrideAction(trainer.sampleAction(observation));
                reward.observe(player->getPosition());
                trainer.recordReward(reward.consume());
            }

            for (auto& e : entities) if (e && e->isActive()) e->update(kDt);
            physics.update(entities, tileMap, kDt);
            entities.erase(std::remove_if(entities.begin(), entities.end(),
                               [player](const std::unique_ptr<Entity>& e) {
                                   return !e || (e.get() != player && !e->isActive());
                               }), entities.end());

            simTime += kDt;
            const sf::Vector2f position = player->getPosition();
            if (position.x > furthest) { furthest = position.x; stall = 0.0f; }
            else                       { stall += kDt; }

            if (position.y > bottomVoid || player->isDying() || player->getLives() <= 0) {
                outcome = "died"; break;
            }
            // The stall cut-off is an imitation-phase device. Under REINFORCE it
            // is harmful: exploration looks like stalling for a second or two
            // before it pays off, and cutting at 4 s made every episode end with
            // near-identical returns, which then carried no learning signal.
            const float stallLimit = (trainer.mode() == PolicyTrainer::Mode::Reinforce)
                                         ? opt.maxEpisodeSeconds : 4.0f;
            if (stall >= stallLimit) { outcome = "stuck"; break; }
            if (position.x >= levelWidth - 4.0f * Constants::TILE_SIZE) {
                outcome = "reached the end"; ++completions; break;
            }
        }

        const float progress = std::clamp((furthest - startX) / (levelWidth - startX), 0.0f, 1.0f);
        progressSum += progress;
        trainer.endEpisode(outcome);

        if ((episode + 1) % 25 == 0) {
            std::error_code ignored;
            std::filesystem::create_directories("saves/ai", ignored);
            policy.saveCheckpoint(trainer.mode() == PolicyTrainer::Mode::Reinforce
                                      ? kReinforceCheckpoint : kImitationCheckpoint);
            const auto now = std::chrono::steady_clock::now();
            const double elapsed = std::chrono::duration<double>(now - wallStart).count();
            std::cout << std::fixed << std::setprecision(1)
                      << "[train] ep " << std::setw(4) << (episode + 1) << "/" << opt.episodes
                      << "  " << (trainer.mode() == PolicyTrainer::Mode::Reinforce ? "RL " : "IM ")
                      << " mean-progress " << std::setw(5) << (progressSum / 25.0 * 100.0) << "%"
                      << "  jump-agr " << std::setw(5)
                      << (trainer.buttonAgreement().size() > 2 ? trainer.buttonAgreement()[2] * 100.0f : 0.0f) << "%"
                      << "  return " << std::setw(7) << trainer.lastEpisodeReturn()
                      << "  " << std::setprecision(0) << elapsed << "s" << std::setprecision(1)
                      << std::endl;
            progressSum = 0.0;
        }

        Game::getInstance().setPlayer(nullptr);
        Game::getInstance().setTileMap(nullptr);
        entities.clear();
    }

    std::error_code ignored;
    std::filesystem::create_directories("saves/ai", ignored);
    policy.saveCheckpoint(trainer.mode() == PolicyTrainer::Mode::Reinforce
                              ? kReinforceCheckpoint : kImitationCheckpoint);
    std::cout << "[train] done: " << trainer.episodes() << " episodes, "
              << trainer.samples() << " samples, " << completions << " completions."
              << std::endl;

    SoundManager::getInstance().shutdown();
    return 0;
}
