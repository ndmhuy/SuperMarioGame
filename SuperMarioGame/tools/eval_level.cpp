// Headless level evaluation: play a level with an AI, report how it went.
//
// Phase 0 deliverable 1 of the map-generation side project
// (docs/mapgen_gan_rl_plan.md §4.1). It is the bottleneck for both halves: the
// RL trainer needs fast rollouts, and the level generator needs a fitness score
// for candidate levels. The JSON report this writes is the only currency the
// two halves exchange, which is why it is versioned.
//
//   ./eval_level assets/levels/level_1.json
//   ./eval_level gen_0042.json --archetype speedrunner --max-seconds 120 \
//                              --report saves/eval/gen_0042.json
//
// Why this is a separate harness and not PlayingState
// ---------------------------------------------------
// PlayingState cannot be driven headlessly as it stands: it reads
// ImGui::GetIO() (a hard abort with no context), has no entry point for an
// arbitrary level path, can only attach an AIController to a CPU *Player 2*,
// and its update path writes save files, pushes states on death and victory,
// and snapshots the whole world every frame for the rewind buffer. None of that
// is wrong for the game; all of it is wrong for ten thousand rollouts.
//
// So this reuses the parts that carry the actual behaviour — LevelLoader,
// EntityFactory, PhysicsEngine, every Entity::update, AIController and its
// policies — and reimplements only the handful of rules PlayingState owns that
// decide when a run ends. Those are marked PORTED below, with the line they
// came from, because they are the one thing here that can silently drift out of
// agreement with the real game.
//
// No window, no GL context, no ImGui, no rendering.

#include "Core/EventBus.hpp"
#include "Core/Game.hpp"
#include "Core/GameMode.hpp"
#include "Core/SoundManager.hpp"
#include "Entities/AIController.hpp"
#include "Entities/Mario.hpp"
#include "Entities/NeuralPolicy.hpp"
#include "Entities/Player.hpp"
#include "Physics/PhysicsEngine.hpp"
#include "Utils/Constants.hpp"
#include "Utils/LevelLoader.hpp"
#include "Utils/TileMap.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace {

// Bump when a field is added, removed or redefined. A trainer or a generator
// pipeline reading an older report must be able to tell. Same contract as
// kAIObservationVersion in IAIPolicy.hpp.
constexpr int kEvalReportVersion = 1;

constexpr float kDt = Constants::FIXED_TIMESTEP;

// A run is "stuck" once the agent has gone this long without beating its own
// furthest point. Reported, never acted on — a generator wants to know that a
// level has a wall in it, and deciding what to do about that is the caller's.
constexpr float kStuckSeconds = 3.0f;

struct Options {
    std::string levelPath;
    std::string reportPath;
    std::string weightsPath;          // empty = heuristic policy
    AIDifficulty difficulty = AIDifficulty::Hard;
    AIArchetype archetype = AIArchetype::Speedrunner;
    float maxSeconds = 300.0f;        // in-game seconds, not wall clock
    bool quiet = false;
    // Per-frame state dump over [traceFrom, traceTo). Off unless asked for.
    // A stalled agent is invisible in a summary — the report says "stuck" and
    // nothing about why — so the runner has to be able to show its working.
    int traceFrom = -1;
    int traceTo = -1;
};

struct Report {
    bool completed = false;
    std::string outcome = "timeout";  // completed | died | timeout | stuck-out-of-time
    float timeToFlag = 0.0f;
    float simSeconds = 0.0f;
    float maxProgressX = 0.0f;        // pixels
    float progressFraction = 0.0f;    // 0..1 of level width — the fitness scalar
    int   deaths = 0;
    int   damageTaken = 0;
    int   coins = 0;
    int   enemiesDefeated = 0;
    int   starCoins = 0;
    float longestStallSeconds = 0.0f;
    int   frames = 0;
    float episodeReward = 0.0f;
};

// --- argument parsing -------------------------------------------------------

bool parseDifficulty(const std::string& s, AIDifficulty& out) {
    if (s == "easy")   { out = AIDifficulty::Easy;   return true; }
    if (s == "normal") { out = AIDifficulty::Normal; return true; }
    if (s == "hard")   { out = AIDifficulty::Hard;   return true; }
    return false;
}

bool parseArchetype(const std::string& s, AIArchetype& out) {
    if (s == "speedrunner") { out = AIArchetype::Speedrunner; return true; }
    if (s == "hunter")      { out = AIArchetype::Hunter;      return true; }
    if (s == "collector")   { out = AIArchetype::Collector;   return true; }
    return false;
}

void usage() {
    std::cerr <<
        "usage: eval_level <level.json> [options]\n"
        "\n"
        "  --report <path>       write the JSON report here (default: stdout only)\n"
        "  --policy <weights>    a NeuralPolicy weight file; omit for the heuristic\n"
        "  --difficulty <d>      easy | normal | hard          (default: hard)\n"
        "  --archetype <a>       speedrunner | hunter | collector (default: speedrunner)\n"
        "  --max-seconds <n>     in-game seconds before timeout (default: 300)\n"
        "  --quiet               suppress the human-readable summary\n"
        "  --trace <a>:<b>       dump per-frame agent state for frames [a, b)\n";
}

bool parseArgs(int argc, char** argv, Options& opt) {
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        auto next = [&](const char* what) -> const char* {
            if (i + 1 >= argc) {
                std::cerr << "[eval] " << arg << " needs " << what << ".\n";
                return nullptr;
            }
            return argv[++i];
        };

        if (arg == "--report") {
            const char* v = next("a path");            if (!v) return false;
            opt.reportPath = v;
        } else if (arg == "--policy") {
            const char* v = next("a weight file");     if (!v) return false;
            opt.weightsPath = v;
        } else if (arg == "--difficulty") {
            const char* v = next("a difficulty");      if (!v) return false;
            if (!parseDifficulty(v, opt.difficulty)) {
                std::cerr << "[eval] unknown difficulty '" << v << "'.\n";
                return false;
            }
        } else if (arg == "--archetype") {
            const char* v = next("an archetype");      if (!v) return false;
            if (!parseArchetype(v, opt.archetype)) {
                std::cerr << "[eval] unknown archetype '" << v << "'.\n";
                return false;
            }
        } else if (arg == "--max-seconds") {
            const char* v = next("a number");          if (!v) return false;
            opt.maxSeconds = std::strtof(v, nullptr);
            if (opt.maxSeconds <= 0.0f) {
                std::cerr << "[eval] --max-seconds must be positive.\n";
                return false;
            }
        } else if (arg == "--trace") {
            const char* v = next("a frame range like 600:660");  if (!v) return false;
            const std::string range = v;
            const std::size_t colon = range.find(':');
            if (colon == std::string::npos) {
                std::cerr << "[eval] --trace wants <from>:<to>, e.g. 600:660.\n";
                return false;
            }
            opt.traceFrom = std::atoi(range.substr(0, colon).c_str());
            opt.traceTo = std::atoi(range.substr(colon + 1).c_str());
            if (opt.traceTo <= opt.traceFrom) {
                std::cerr << "[eval] --trace range must be increasing.\n";
                return false;
            }
        } else if (arg == "--quiet") {
            opt.quiet = true;
        } else if (arg == "--help" || arg == "-h") {
            usage();
            return false;
        } else if (!arg.empty() && arg[0] == '-') {
            std::cerr << "[eval] unknown option '" << arg << "'.\n";
            return false;
        } else if (opt.levelPath.empty()) {
            opt.levelPath = arg;
        } else {
            std::cerr << "[eval] more than one level given.\n";
            return false;
        }
    }

    if (opt.levelPath.empty()) {
        usage();
        return false;
    }
    return true;
}

// --- report -----------------------------------------------------------------

const char* difficultyName(AIDifficulty d) {
    switch (d) {
        case AIDifficulty::Easy:   return "easy";
        case AIDifficulty::Normal: return "normal";
        case AIDifficulty::Hard:   return "hard";
    }
    return "hard";
}

const char* archetypeName(AIArchetype a) {
    switch (a) {
        case AIArchetype::Speedrunner: return "speedrunner";
        case AIArchetype::Hunter:      return "hunter";
        case AIArchetype::Collector:   return "collector";
    }
    return "speedrunner";
}

std::string toJson(const Report& r, const Options& opt, const std::string& policyName) {
    std::ostringstream o;
    o << std::fixed << std::setprecision(3);
    o << "{\n"
      << "  \"reportVersion\": " << kEvalReportVersion << ",\n"
      << "  \"level\": \"" << opt.levelPath << "\",\n"
      << "  \"policy\": \"" << policyName << "\",\n"
      << "  \"difficulty\": \"" << difficultyName(opt.difficulty) << "\",\n"
      << "  \"archetype\": \"" << archetypeName(opt.archetype) << "\",\n"
      << "  \"completed\": " << (r.completed ? "true" : "false") << ",\n"
      << "  \"outcome\": \"" << r.outcome << "\",\n"
      << "  \"timeToFlag\": " << r.timeToFlag << ",\n"
      << "  \"simSeconds\": " << r.simSeconds << ",\n"
      << "  \"maxProgressX\": " << r.maxProgressX << ",\n"
      << "  \"progressFraction\": " << r.progressFraction << ",\n"
      << "  \"deaths\": " << r.deaths << ",\n"
      << "  \"damageTaken\": " << r.damageTaken << ",\n"
      << "  \"coins\": " << r.coins << ",\n"
      << "  \"enemiesDefeated\": " << r.enemiesDefeated << ",\n"
      << "  \"starCoins\": " << r.starCoins << ",\n"
      << "  \"longestStallSeconds\": " << r.longestStallSeconds << ",\n"
      << "  \"frames\": " << r.frames << ",\n"
      << "  \"episodeReward\": " << r.episodeReward << "\n"
      << "}\n";
    return o.str();
}

} // namespace

int main(int argc, char** argv) {
    Options opt;
    if (!parseArgs(argc, argv, opt)) return 2;

    // Rollouts have no speakers. There is no mute switch on SoundManager, and
    // the first entity to stomp something constructs it either way, so this
    // only silences it — see the plan's note about giving it a real no-op path.
    SoundManager::getInstance().setSFXVolume(0.0f);
    SoundManager::getInstance().setMusicVolume(0.0f);

    // --- load ---------------------------------------------------------------
    TileMap tileMap;
    LevelData levelData;
    LevelLoader loader;
    if (!loader.loadLevel(opt.levelPath, tileMap, levelData)) {
        std::cerr << "[eval] could not load '" << opt.levelPath << "'.\n";
        return 1;
    }

    std::vector<std::unique_ptr<Entity>> entities = std::move(levelData.entities);

    // The player goes in at index 0, mirroring PlayingState::adoptPlayer.
    auto owned = std::make_unique<Mario>(levelData.spawnPoint);
    Player* player = owned.get();
    entities.insert(entities.begin(), std::move(owned));

    // NOT optional. Seventeen entity and strategy files reach for
    // Game::getInstance().getNearestPlayer() or getTileMap() from inside their
    // update: ChaseStrategy, Thwomp, MovingPlatform, FallingPlatform,
    // ConveyorBelt, every boss. Without these two calls they all find null and
    // go inert — every enemy in the level stands still, and the level scores as
    // trivially easy while looking like it was played properly.
    Game::getInstance().setPlayer(player);
    Game::getInstance().setTileMap(&tileMap);

    // --- the agent ----------------------------------------------------------
    AIController controller(*player, opt.difficulty, opt.archetype);
    if (!opt.weightsPath.empty()) {
        auto net = std::make_unique<NeuralPolicy>();
        if (!net->load(opt.weightsPath)) {
            std::cerr << "[eval] could not load weights '" << opt.weightsPath
                      << "' — refusing to fall back to the heuristic silently, "
                         "because a report that says 'neural' about a heuristic run "
                         "is worse than no report.\n";
            return 1;
        }
        controller.setPolicy(std::move(net));
    }
    // Scores the run without writing a dataset. The reward is a useful summary
    // even when nothing is training: it is the one number that folds progress,
    // damage and dawdling together.
    controller.enableLearning();

    const std::string policyName = controller.policyName();

    // --- outcome signals ----------------------------------------------------
    Report report;
    bool levelComplete = false;

    EventBus::ScopedSubscription onComplete(
        EventType::LevelComplete, [&](const GameEvent&) { levelComplete = true; });
    EventBus::ScopedSubscription onCoin(
        EventType::CoinCollected, [&](const GameEvent&) { ++report.coins; });
    EventBus::ScopedSubscription onKill(
        EventType::EnemyDefeated, [&](const GameEvent&) { ++report.enemiesDefeated; });
    EventBus::ScopedSubscription onStar(
        EventType::StarCoinCollected, [&](const GameEvent&) { ++report.starCoins; });
    EventBus::ScopedSubscription onHurt(
        EventType::PlayerDamaged, [&](const GameEvent&) { ++report.damageTaken; });

    // --- the loop -----------------------------------------------------------
    PhysicsEngine physics;

    const float levelWidthPx  = tileMap.getWidth()  * Constants::TILE_SIZE;
    const float bottomVoidY   = tileMap.getHeight() * Constants::TILE_SIZE + 32.0f;   // PORTED PlayingState.cpp:635
    const float rightVoidX    = levelWidthPx + 64.0f;                                  // PORTED PlayingState.cpp:636
    const float startX        = levelData.spawnPoint.x;

    float simTime = 0.0f;
    float stallTime = 0.0f;
    float furthestX = startX;
    report.maxProgressX = startX;

    if (opt.traceFrom >= 0) {
        std::cout << "# frame      x       y      vx      vy  ground  canJump  "
                     "tile(x,y)  reason\n";
    }

    while (simTime < opt.maxSeconds) {
        controller.update(kDt, nullptr, tileMap, entities);

        if (report.frames >= opt.traceFrom && report.frames < opt.traceTo) {
            const sf::Vector2f p = player->getPosition();
            const sf::Vector2f v = player->getVelocity();
            const AIObservation& obs = controller.lastObservation();
            const AABB b = player->getBoundingBox();
            const int tx = int(b.x / Constants::TILE_SIZE);
            const int ty = int(b.y / Constants::TILE_SIZE);
            auto tile = [&](int dx, int dy) {
                return int(tileMap.getTileType(tx + dx, ty + dy));
            };
            std::cout << std::fixed << std::setprecision(1)
                      << "  " << std::setw(5) << report.frames
                      << std::setw(8) << p.x << std::setw(8) << p.y
                      << std::setw(8) << v.x << std::setw(8) << v.y
                      << std::setw(7) << (player->isOnGround() ? "yes" : "no")
                      << std::setw(8) << (obs.canJump ? "yes" : "no")
                      << "  box=" << b.x << "," << b.y
                      << " " << b.width << "x" << b.height
                      << "  up=" << tile(0, -1) << " right=" << tile(1, 0)
                      << " upright=" << tile(1, -1) << " down=" << tile(0, 1)
                      << "  " << controller.reason() << "\n";
        }

        for (auto& e : entities) {
            if (e && e->isActive()) e->update(kDt);
        }
        physics.update(entities, tileMap, kDt);

        entities.erase(std::remove_if(entities.begin(), entities.end(),
                                      [player](const std::unique_ptr<Entity>& e) {
                                          return !e || (e.get() != player && !e->isActive());
                                      }),
                       entities.end());

        simTime += kDt;
        ++report.frames;

        // Progress, measured against the furthest point reached rather than the
        // previous frame — the same rule RewardTracker uses, so the report and
        // the reward never disagree about what progress means.
        const sf::Vector2f pos = player->getPosition();
        if (pos.x > furthestX) {
            furthestX = pos.x;
            report.maxProgressX = furthestX;
            stallTime = 0.0f;
        } else {
            stallTime += kDt;
            report.longestStallSeconds = std::max(report.longestStallSeconds, stallTime);
        }

        if (levelComplete) {
            report.completed = true;
            report.outcome = "completed";
            report.timeToFlag = simTime;
            break;
        }

        // PORTED PlayingState.cpp:615 — lava is not solid, so nothing in the
        // physics engine notices it. Checked at the feet, which touch first.
        const AABB box = player->getBoundingBox();
        if (tileMap.getTileAt(box.x + box.width * 0.5f,
                              box.y + box.height - 2.0f) == TileType::Lava) {
            player->takeDamage(1);
        }

        // PORTED PlayingState.cpp:635 — the void. Sideways counts: a bad warp
        // exit can park the player past the edge, where they are outside every
        // tile and would otherwise never fall and never die.
        const bool inVoid = pos.y > bottomVoidY || pos.x < -64.0f || pos.x > rightVoidX;

        if (inVoid || player->isDying() || player->getLives() <= 0) {
            ++report.deaths;
            if (player->getLives() <= 0) {
                report.outcome = "died";
                break;
            }
            // Respawn at the start rather than a checkpoint: a generated level
            // has no checkpoints, and a fitness score for "can this be finished"
            // should not be helped by one.
            player->setPosition(levelData.spawnPoint);
            player->setVelocity({0.0f, 0.0f});
            controller.reset();
            furthestX = startX;
            stallTime = 0.0f;
        }
    }

    if (!report.completed && report.outcome == "timeout" &&
        report.longestStallSeconds >= kStuckSeconds) {
        report.outcome = "stuck-out-of-time";
    }

    report.simSeconds = simTime;
    report.episodeReward = controller.episodeReward();
    report.progressFraction =
        levelWidthPx > 0.0f
            ? std::clamp((report.maxProgressX - startX) / (levelWidthPx - startX), 0.0f, 1.0f)
            : 0.0f;

    // --- out ----------------------------------------------------------------
    const std::string json = toJson(report, opt, policyName);

    if (!opt.reportPath.empty()) {
        std::error_code ignored;
        const std::filesystem::path out(opt.reportPath);
        if (out.has_parent_path()) std::filesystem::create_directories(out.parent_path(), ignored);
        std::ofstream file(opt.reportPath);
        if (!file) {
            std::cerr << "[eval] could not write '" << opt.reportPath << "'.\n";
            return 1;
        }
        file << json;
    }

    if (!opt.quiet) {
        std::cout << json;
        std::cout << "[eval] " << opt.levelPath << ": " << report.outcome
                  << ", " << std::fixed << std::setprecision(1)
                  << (report.progressFraction * 100.0f) << "% of the way, "
                  << report.deaths << " death(s), "
                  << std::setprecision(1) << report.simSeconds << "s simulated in "
                  << report.frames << " frames.\n";
    }

    // Entities hold raw Player* observers registered above; drop the
    // registrations before the vector unwinds.
    Game::getInstance().setPlayer(nullptr);
    Game::getInstance().setTileMap(nullptr);

    // verify_frontend_states.cpp learned this the hard way: SFML's audio and
    // texture statics must be torn down before main returns, or destruction
    // order at exit throws and aborts the process.
    entities.clear();
    SoundManager::getInstance().shutdown();

    return report.completed ? 0 : 3;
}
