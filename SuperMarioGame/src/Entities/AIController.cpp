#include "Entities/AIController.hpp"

#include "Entities/Enemy.hpp"
#include "Entities/HeuristicPolicy.hpp"
#include "Entities/Item.hpp"
#include "Entities/Player.hpp"
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
    if (tile == TileType::Lava) return AICellState::Hazard;
    // A question block or a coin tile is worth going out of the way for.
    if (tile == TileType::Question || tile == TileType::Coin) return AICellState::Reward;
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

void AIController::reset() {
    m_decisionTimer = 0.0f;
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
            case EntityCategory::Enemy: state = AICellState::Enemy;  break;
            case EntityCategory::Item:  state = AICellState::Reward; break;
            // Projectiles in flight are threats worth dodging; blocks are
            // already covered by the tile pass or are scenery.
            case EntityCategory::Projectile: state = AICellState::Hazard; break;
            default: continue;
        }

        const AABB other = entity->getBoundingBox();
        const sf::Vector2i cell = tileMap.worldToGrid(other.x + other.width * 0.5f,
                                                     other.y + other.height * 0.5f);
        const int dx = cell.x - origin.x;
        const int dy = cell.y - origin.y;
        if (std::abs(dx) > m_visionRadiusX || std::abs(dy) > m_visionRadiusY) continue;

        const std::size_t index =
            static_cast<std::size_t>(dy + halfH) * kAIVisionWidth + (dx + halfW);
        m_observation.grid[index] = state;
    }

    // --- Scalars -------------------------------------------------------------
    // The objective is the right-hand end of the level. Nothing in the level
    // format marks the exit, and every campaign level runs left to right, so the
    // map's right edge is the honest answer rather than a guess.
    const float goalX = static_cast<float>(tileMap.getWidth()) * Constants::TILE_SIZE;
    const float normalizer = static_cast<float>(kAIVisionWidth / 2) * Constants::TILE_SIZE;

    m_observation.dxToGoal = std::clamp((goalX - feet.x) / normalizer, -1.0f, 1.0f);
    m_observation.dyToGoal = 0.0f;

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
