#include "Entities/AIController.hpp"
#include "Entities/QuestionBlock.hpp"
#include "Entities/Coin.hpp"
#include "Entities/Enemy.hpp"
#include "Entities/Player.hpp"
#include "Entities/Projectile.hpp"

#include "Entities/Enemy.hpp"
#include "Entities/HeuristicPolicy.hpp"
#include "Entities/Item.hpp"
#include "Utils/Constants.hpp"
#include "Utils/TileMap.hpp"

#include <algorithm>
#include <cmath>

namespace {
// Vision half-extents per difficulty, in tiles. The grid buffer is always
// 21x15; these decide how much of it is filled in rather than how big it is.
// Easy's 5x5 and Normal's 11x11 are the plan's numbers, expressed as radii.
constexpr int kEasyRadiusX = 2,   kEasyRadiusY = 2;
constexpr int kNormalRadiusX = 5, kNormalRadiusY = 5;
constexpr int kHardRadiusX = kAIVisionWidth / 2, kHardRadiusY = kAIVisionHeight / 2;

// Which tiles read as which cell state.
AICellState classifyTile(TileType tile) {
    if (tile == TileType::Lava || tile == TileType::Water) return AICellState::Hazard;
    // A coin is worth a small detour; a question block might hold a power-up
    // and is also a platform, so it is worth more and is worth a jump.
    if (tile == TileType::Coin) return AICellState::Coin;
    if (tile == TileType::Question) return AICellState::PowerUp;
    return TileMap::getInfo(tile).isSolid ? AICellState::Solid : AICellState::Empty;
}
} // namespace

AIController::AIController(Player& controlled, AIDifficulty difficulty, AIArchetype archetype)
    : m_player(controlled),
      m_policy(std::make_unique<HeuristicPolicy>(archetype)),
      m_difficulty(difficulty),
      m_archetype(archetype),
      // Reproducible: same difficulty and archetype means the same noise
      // sequence, so a bot that walks into a pit does it again next run.
      m_rng(static_cast<std::mt19937::result_type>(
          0x5EEDu + static_cast<unsigned>(difficulty) * 31u +
          static_cast<unsigned>(archetype) * 7u)) {
    applyDifficultyProfile();
    m_observation.grid.fill(AICellState::Unknown);
}

AIController::~AIController() = default;

void AIController::applyDifficultyProfile() {
    switch (m_difficulty) {
        case AIDifficulty::Easy:
            m_visionRadiusX = kEasyRadiusX;
            m_visionRadiusY = kEasyRadiusY;
            m_reactionLatency = 0.40f;
            m_actionNoise = 0.35f;
            // Walk and a normal jump only, per the plan's allowed-controls row.
            m_allowRun = false;
            m_allowShoot = false;
            m_allowGroundPound = false;
            break;
        case AIDifficulty::Normal:
            m_visionRadiusX = kNormalRadiusX;
            m_visionRadiusY = kNormalRadiusY;
            m_reactionLatency = 0.12f;
            m_actionNoise = 0.05f;
            m_allowRun = true;
            m_allowShoot = true;
            m_allowGroundPound = false;
            break;
        case AIDifficulty::Hard:
            m_visionRadiusX = kHardRadiusX;
            m_visionRadiusY = kHardRadiusY;
            // Frame-perfect: a decision every frame, and no noise at all.
            m_reactionLatency = 0.0f;
            m_actionNoise = 0.0f;
            m_allowRun = true;
            m_allowShoot = true;
            m_allowGroundPound = true;
            break;
    }
}

void AIController::setDifficulty(AIDifficulty difficulty) {
    m_difficulty = difficulty;
    applyDifficultyProfile();
}

void AIController::setArchetype(AIArchetype archetype) {
    m_archetype = archetype;
    if (auto* heuristic = dynamic_cast<HeuristicPolicy*>(m_policy.get())) {
        heuristic->setArchetype(archetype);
    }
}

void AIController::setPolicy(std::unique_ptr<IAIPolicy> policy) {
    if (!policy) return;
    m_policy = std::move(policy);
    m_policy->reset();
}

void AIController::setReactionLatency(float seconds) {
    m_reactionLatency = std::clamp(seconds, 0.0f, 1.5f);
}

void AIController::setActionNoise(float epsilon) {
    m_actionNoise = std::clamp(epsilon, 0.0f, 1.0f);
}

const char* AIController::policyName() const {
    return m_policy ? m_policy->name() : "NONE";
}

void AIController::setWaypoints(std::vector<sf::Vector2f> waypoints) {
    m_waypoints = std::move(waypoints);
    m_waypointIndex = 0;
}

void AIController::reset() {
    m_decisionTimer = 0.0f;
    // A respawn puts the agent back at the start of the route, and re-arms
    // route guidance if the previous life lost it.
    m_waypointIndex = 0;
    m_waypointStallSeconds = 0.0f;
    m_routeLost = false;
    m_action = AIAction{};
    m_reason = "idle";
    m_observation.grid.fill(AICellState::Unknown);
    if (m_policy) m_policy->reset();
    // A respawn is a new episode: the progress mark has to move to where the
    // agent now is, or the first observe() after it credits the whole distance
    // back to the checkpoint as though the agent had just run it.
    if (m_learning) m_reward.reset(m_player.getPosition());
}

void AIController::enableLearning(const std::string& logPath) {
    m_learning = true;
    m_reward.reset(m_player.getPosition());
    if (!logPath.empty()) m_experience.open(logPath);
}

void AIController::scanEnvironment(const Player* opponent, const TileMap& tileMap,
                                  const std::vector<std::unique_ptr<Entity>>& entities) {
    m_observation.grid.fill(AICellState::Unknown);

    const AABB box = m_player.getBoundingBox();
    // Sense from the tile the agent's feet are in: that is the row it walks
    // along, and every probe in the policy is expressed relative to it.
    const sf::Vector2f feet{box.x + box.width * 0.5f, box.y + box.height - 2.0f};
    const sf::Vector2i origin = tileMap.worldToGrid(feet.x, feet.y);

    const int halfW = kAIVisionWidth / 2;
    const int halfH = kAIVisionHeight / 2;

    // Tiles first; entities are painted over the top, because an enemy standing
    // on a solid tile matters more to a decision than the tile does.
    for (int dy = -m_visionRadiusY; dy <= m_visionRadiusY; ++dy) {
        for (int dx = -m_visionRadiusX; dx <= m_visionRadiusX; ++dx) {
            const int gx = origin.x + dx;
            const int gy = origin.y + dy;
            const std::size_t index =
                static_cast<std::size_t>(dy + halfH) * kAIVisionWidth + (dx + halfW);
            // Off the map reads as solid: the agent should treat the level
            // boundary as a wall, not as unexplored space to walk into.
            const bool offMap = gx < 0 || gx >= tileMap.getWidth() ||
                                gy < 0 || gy >= tileMap.getHeight();
            m_observation.grid[index] =
                offMap ? AICellState::Solid
                       : classifyTile(tileMap.getTileType(gx, gy));
        }
    }

    for (const auto& entity : entities) {
        if (!entity || !entity->isActive()) continue;
        if (entity.get() == &m_player) continue;

        AICellState state;
        switch (entity->getCategory()) {
            case EntityCategory::Enemy: {
                // Stompable or not — see Enemy::isStompSafe(). This is the
                // distinction that decides whether jumping on the thing in
                // front of you is a kill or a hit.
                const Enemy* enemy = static_cast<const Enemy*>(entity.get());
                state = enemy->isStompSafe() ? AICellState::EnemyStompable
                                             : AICellState::EnemyDangerous;
                break;
            }
            case EntityCategory::Item:
                // A coin is a small, safe gain; anything else an Item can be —
                // mushroom, flower, star, 1-up — changes the player's state and
                // is worth more.
                state = dynamic_cast<const Coin*>(entity.get()) ? AICellState::Coin
                                                                : AICellState::PowerUp;
                break;
            case EntityCategory::Projectile: {
                // Ask the projectile who it can hurt rather than who threw it —
                // "does this damage me" is the question the policy actually
                // needs, and Projectile already answers it. A Hammer Bro's
                // hammer is a hazard; the agent's own fireball is not.
                const Projectile* shot = static_cast<const Projectile*>(entity.get());
                state = shot->damagesPlayer() ? AICellState::Hazard
                                              : AICellState::FriendlyProjectile;
                break;
            }
            // Entity-form blocks — pipes, question blocks, moving and falling
            // platforms — are solid to the physics but live outside the tile
            // map, so the tile pass cannot see them. Skipping them here (the
            // original code did, on the wrong belief that the tile pass covered
            // them) made every pipe an invisible wall: in bonus_1 the agent
            // stood flush against the pipe at x=58 reading "advancing" for 190
            // seconds, because its vision grid said the way was clear.
            // A question block is a reward as well as a surface, and the tile
            // pass classifies question TILES as Reward, so the entity form gets
            // the same answer.
            case EntityCategory::Block:
                state = dynamic_cast<QuestionBlock*>(entity.get())
                            ? AICellState::PowerUp
                            : AICellState::Solid;
                break;
            default: continue;
        }

        // Paint the entity's whole footprint, not its centre cell. A pipe is
        // 2x2 tiles: centre-painting marked one cell inside it and left the
        // face the agent actually collides with unmarked.
        const AABB other = entity->getBoundingBox();
        const sf::Vector2i topLeft = tileMap.worldToGrid(other.x + 1.0f, other.y + 1.0f);
        const sf::Vector2i bottomRight = tileMap.worldToGrid(other.x + other.width - 1.0f,
                                                             other.y + other.height - 1.0f);
        for (int gy = topLeft.y; gy <= bottomRight.y; ++gy) {
            for (int gx = topLeft.x; gx <= bottomRight.x; ++gx) {
                const int dx = gx - origin.x;
                const int dy = gy - origin.y;
                if (std::abs(dx) > m_visionRadiusX || std::abs(dy) > m_visionRadiusY) continue;
                const std::size_t index =
                    static_cast<std::size_t>(dy + halfH) * kAIVisionWidth + (dx + halfW);
                m_observation.grid[index] = state;
            }
        }
    }

    // --- Scalars -------------------------------------------------------------
    const float normalizer = static_cast<float>(kAIVisionWidth / 2) * Constants::TILE_SIZE;

    // The objective. With no waypoints, it is the right-hand end of the level:
    // nothing in the level format marks the exit, every campaign level runs
    // left to right, and the right edge is the honest answer rather than a
    // guess — but it is a DEGENERATE answer, "rightward, always", and it made
    // dyToGoal a dead input for the network and the goal pull blind for the
    // heuristic. With waypoints (the solvability oracle's certified route, see
    // AIWaypoints.hpp), the goal is a node a few footholds ahead on a path
    // that provably reaches the flag: dx can point LEFT when the route
    // backtracks, and dy says "climb here" before the wall is in view.
    sf::Vector2f goal{static_cast<float>(tileMap.getWidth()) * Constants::TILE_SIZE,
                      feet.y};
    if (!m_waypoints.empty() && !m_routeLost) {
        // Consume nodes the agent has reached (within 1.25 tiles of the feet
        // target) or passed (a full tile beyond it, in the direction the route
        // is locally travelling — the guard matters because certified routes
        // are allowed to backtrack, and an unguarded "x is beyond" would skip
        // the whole leftward leg).
        constexpr float kReachedPx = 40.0f;
        while (m_waypointIndex + 1 < m_waypoints.size()) {
            const sf::Vector2f& node = m_waypoints[m_waypointIndex];
            const sf::Vector2f d{node.x - feet.x, node.y - feet.y};
            const bool reached = d.x * d.x + d.y * d.y < kReachedPx * kReachedPx;
            const bool rightward = m_waypoints[m_waypointIndex + 1].x >= node.x;
            // Passing in x only counts if the agent is AT the node's height
            // (or above it). Without the height check, walking along the
            // ground under a hill "passed" every node on top of the hill —
            // the index ran 10+ tiles ahead, dxToGoal saturated at 1.0, and
            // the climb the route was trying to signal was consumed unseen.
            const bool atHeight = node.y - feet.y > -1.5f * Constants::TILE_SIZE;
            const bool passed = atHeight &&
                                (rightward
                                     ? feet.x > node.x + Constants::TILE_SIZE
                                     : feet.x < node.x - Constants::TILE_SIZE);
            if (!reached && !passed) break;
            ++m_waypointIndex;
            m_waypointStallSeconds = 0.0f;
        }
        // The goal is the first UNCONSUMED node — never a node beyond it. A
        // lookahead was tried here (aim 3 nodes ahead, for a smoother pull)
        // and it broke the one signal this channel exists to carry: a jump is
        // a single route edge, so "3 nodes ahead" at a climb is the far side
        // of the jump, 10+ tiles away — dx saturated at 1.0 exactly where dy
        // was supposed to say "climb here". On flat walking the unconsumed
        // node is only a tile ahead and dx is small; small-but-correct beats
        // smooth-but-wrong.
        goal = m_waypoints[m_waypointIndex];
        constexpr float kRouteLostSeconds = 8.0f;
        if (m_waypointStallSeconds > kRouteLostSeconds) {
            m_routeLost = true;
            goal = {static_cast<float>(tileMap.getWidth()) * Constants::TILE_SIZE,
                    feet.y};
        }
    }

    m_observation.dxToGoal = std::clamp((goal.x - feet.x) / normalizer, -1.0f, 1.0f);
    m_observation.dyToGoal =
        m_waypoints.empty()
            ? 0.0f
            : std::clamp((goal.y - feet.y) / normalizer, -1.0f, 1.0f);

    if (opponent) {
        const sf::Vector2f delta = opponent->getPosition() - m_player.getPosition();
        m_observation.dxToOpponent = std::clamp(delta.x / normalizer, -1.0f, 1.0f);
        m_observation.dyToOpponent = std::clamp(delta.y / normalizer, -1.0f, 1.0f);
    } else {
        m_observation.dxToOpponent = 0.0f;
        m_observation.dyToOpponent = 0.0f;
    }

    const sf::Vector2f velocity = m_player.getVelocity();
    m_observation.vx = std::clamp(velocity.x / Constants::RUN_SPEED, -1.0f, 1.0f);
    m_observation.vy = std::clamp(velocity.y / Constants::RUN_SPEED, -1.0f, 1.0f);
    m_observation.onGround = m_player.isOnGround();
    // Coyote frames count: the policy may legitimately jump a few frames after
    // walking off a ledge, exactly as a human can.
    m_observation.canJump =
        m_player.isOnGround() || m_player.getCoyoteFramesLeft() > 0;
    m_observation.isPoweredUp = m_player.getForm() != Player::Form::Small &&
                                m_player.getForm() != Player::Form::Mini;
}

void AIController::applyNoise(AIAction& action) {
    if (m_actionNoise <= 0.0f) return;
    if (m_unit(m_rng) >= m_actionNoise) return;

    // Perturb exactly one button. Randomising the whole action would produce
    // seizures rather than clumsiness; flipping one is what a mistimed input
    // actually looks like.
    switch (m_rng() % 4u) {
        case 0:
            // Drop the movement entirely: a hesitation.
            action.moveLeft = false;
            action.moveRight = false;
            break;
        case 1:
            action.jump = !action.jump;
            break;
        case 2:
            std::swap(action.moveLeft, action.moveRight);
            break;
        default:
            action.run = !action.run;
            break;
    }
}

void AIController::overrideAction(const AIAction& action) {
    m_action = action;
    actuate();
}

void AIController::actuate() {
    // Movement verbs are per-frame intent flags — PhysicsEngine clears them
    // after integrating — so they are re-asserted every frame from the held
    // action, exactly as InputManager::update() does for a human.
    if (m_action.moveLeft)  m_player.moveLeft();
    if (m_action.moveRight) m_player.moveRight();
    if (m_allowRun && m_action.run) m_player.run();
    if (m_action.crouch) m_player.crouch();

    // Jump is edge-triggered: the action is held between decisions, so applying
    // it every frame would bunny-hop. Consuming the flag makes one decision to
    // jump mean one jump.
    if (m_action.jump) {
        m_player.jump();
        m_action.jump = false;
    }
    if (m_allowShoot && m_action.shoot) {
        m_player.shootFireball();
        m_action.shoot = false;
    }
    if (m_allowGroundPound && m_action.groundPound) {
        m_player.groundPound();
        m_action.groundPound = false;
    }
}

void AIController::update(float dt, const Player* opponent, const TileMap& tileMap,
                          const std::vector<std::unique_ptr<Entity>>& entities) {
    if (m_paused || !m_policy) return;
    // A dead bot does not steer. Death is a falling animation on this side too,
    // and driving it mid-fall fights the death sequence.
    if (!m_player.isActive() || m_player.isDying()) return;

    // Reward accrues every frame — progress and the per-step cost — but is only
    // *consumed* when a decision is made, so a transition carries everything
    // that happened while the previous action was in effect.
    if (m_learning) m_reward.observe(m_player.getPosition());

    // The route-lost clock: scanEnvironment() zeroes this whenever the route
    // index advances, so it only accumulates while the plan is going nowhere.
    if (!m_waypoints.empty() && !m_routeLost) m_waypointStallSeconds += dt;

    m_decisionTimer -= dt;
    if (m_decisionTimer <= 0.0f) {
        // Reaction latency is the gap between decisions, so Hard's 0ms means one
        // decision per frame and Easy's 400ms means the bot commits to whatever
        // it last chose for 24 frames.
        m_decisionTimer = m_reactionLatency;

        scanEnvironment(opponent, tileMap, entities);
        AIAction decided = m_policy->decide(m_observation);
        applyNoise(decided);

        // Carry over one-shot flags that have not been consumed yet, so a jump
        // decided at the end of one window is not silently dropped by the next.
        decided.jump = decided.jump || m_action.jump;
        m_action = decided;

        if (auto* heuristic = dynamic_cast<HeuristicPolicy*>(m_policy.get())) {
            m_reason = heuristic->lastReason();
        }

        // One row per decision. Logged with the observation the decision was
        // made from and the action as finally chosen — after the noise, because
        // the noise is part of the behaviour policy and a learner told the
        // pre-noise action would be learning from something that never happened.
        if (m_learning && m_experience.isOpen()) {
            const bool terminal = m_player.isDying() || m_player.getLives() <= 0;
            m_experience.record(m_observation, m_action, m_reward.consume(), terminal);
        }
    }

    actuate();
}
