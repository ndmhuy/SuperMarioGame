#include "Entities/HeuristicPolicy.hpp"

#include <algorithm>
#include <cmath>

namespace {
// How far ahead a threat still counts as "in striking distance", in tiles.
constexpr int kStrikeRange = 4;
// Decisions a chosen direction is held for before a flip is free again.
constexpr int kCommitDecisions = 6;
// Utility penalty applied to reversing direction while committed.
constexpr float kFlipPenalty = 0.45f;
} // namespace

HeuristicPolicy::HeuristicPolicy(AIArchetype archetype)
    : m_archetype(archetype), m_weights(weightsFor(archetype)) {}

void HeuristicPolicy::setArchetype(AIArchetype archetype) {
    m_archetype = archetype;
    m_weights = weightsFor(archetype);
}

void HeuristicPolicy::reset() {
    m_lastDirection = 1;
    m_commitTicks = 0;
    m_lastReason = "idle";
    m_escapeTicks = 0;
    m_escapeDirection = 0;
    m_stuckTicks = 0;
}

const char* HeuristicPolicy::name() const {
    return toString(m_archetype);
}

HeuristicPolicy::Weights HeuristicPolicy::weightsFor(AIArchetype archetype) {
    switch (archetype) {
        case AIArchetype::Speedrunner:
            // Rightward progress and nothing else. Ignores coins, items and the
            // opponent unless one is physically in the way.
            return Weights{/*goal=*/1.0f,  /*opponent=*/0.0f, /*reward=*/0.0f,
                           /*runBias=*/1.0f, /*aggression=*/0.0f, /*caution=*/0.20f};
        case AIArchetype::Hunter:
            // Abandons the route to line up a stomp. Still drifts towards the
            // exit when the opponent is far away, or it would stand still all
            // game on a level the human is racing through.
            return Weights{/*goal=*/0.35f, /*opponent=*/1.0f, /*reward=*/0.10f,
                           /*runBias=*/0.80f, /*aggression=*/1.0f, /*caution=*/0.25f};
        case AIArchetype::Collector:
            // Detours for everything. Slower, but usually the one holding a fire
            // flower by the midpoint of the level.
            return Weights{/*goal=*/0.30f, /*opponent=*/0.0f, /*reward=*/1.0f,
                           /*runBias=*/0.35f, /*aggression=*/0.15f, /*caution=*/0.40f};
    }
    return Weights{};
}

bool HeuristicPolicy::obstacleAhead(const AIObservation& obs, int direction) {
    // Foot height is the agent's own row; head height is one above. A solid tile
    // at either means walking forward will not work.
    return obs.at(direction, 0) == AICellState::Solid ||
           obs.at(direction, -1) == AICellState::Solid;
}

int HeuristicPolicy::wallHeight(const AIObservation& obs, int direction) {
    int height = 0;
    while (height < 4 && obs.at(direction, -height) == AICellState::Solid) {
        ++height;
    }
    return height;
}

bool HeuristicPolicy::raisedGroundAhead(const AIObservation& obs, int direction) {
    for (int step = 2; step <= 5; ++step) {
        const int dx = direction * step;
        for (int rise = 1; rise <= 3; ++rise) {
            // Solid at -rise with clear air above it: a surface, not a ceiling.
            if (obs.at(dx, -rise) == AICellState::Solid &&
                obs.at(dx, -rise - 1) != AICellState::Solid) {
                return true;
            }
        }
    }
    return false;
}

bool HeuristicPolicy::gapAhead(const AIObservation& obs, int direction) {
    // Ground under the next step, and under the one after it. Unknown is treated
    // as solid: an easy bot with a 5x5 window should not panic at the edge of
    // its own vision, it should keep walking and find out.
    auto footing = [&](int dx) {
        const AICellState below = obs.at(dx, 1);
        return below == AICellState::Solid || below == AICellState::Unknown;
    };
    return !footing(direction) && !footing(direction * 2);
}

int HeuristicPolicy::enemyAhead(const AIObservation& obs, int direction) {
    for (int step = 1; step <= kStrikeRange; ++step) {
        const int dx = direction * step;
        // Same row or one above: an enemy on the ground the agent is walking on.
        if (obs.at(dx, 0) == AICellState::Enemy || obs.at(dx, -1) == AICellState::Enemy) {
            return step;
        }
    }
    return 0;
}

int HeuristicPolicy::rewardDirection(const AIObservation& obs) {
    // Nearest reward cell in view, by Manhattan distance. Ties go to the right,
    // which keeps a Collector making forward progress on a symmetric level
    // instead of standing between two identical coins.
    int best = 0;
    int bestDistance = kAIVisionWidth + kAIVisionHeight;
    const int halfW = kAIVisionWidth / 2;
    const int halfH = kAIVisionHeight / 2;

    for (int dy = -halfH; dy <= halfH; ++dy) {
        for (int dx = -halfW; dx <= halfW; ++dx) {
            if (obs.at(dx, dy) != AICellState::Reward) continue;
            const int distance = std::abs(dx) + std::abs(dy);
            if (distance < bestDistance || (distance == bestDistance && dx > 0)) {
                bestDistance = distance;
                best = (dx == 0) ? 0 : (dx > 0 ? 1 : -1);
            }
        }
    }
    return best;
}

AIAction HeuristicPolicy::decide(const AIObservation& obs) {
    AIAction action;

    // --- Horizontal direction: score right against left ---------------------
    //
    // Each term is a signed pull in tile-space, scaled by its archetype weight.
    // Summing them and taking the sign is what makes the archetypes read as
    // personalities rather than as modes: a Hunter with the opponent behind it
    // genuinely wants to turn round, and a Speedrunner in the same spot does not.
    auto directionalUtility = [&](int direction) {
        const float d = static_cast<float>(direction);
        float utility = 0.0f;

        // Goal pull. dxToGoal is normalized, so this is "is the exit that way".
        utility += m_weights.goal * d * (obs.dxToGoal >= 0.0f ? 1.0f : -1.0f);

        // Opponent pull, only while the opponent is worth chasing. Squared
        // falloff, so a Hunter commits hard when close and drifts goalwards when
        // the human is half a level away.
        if (m_weights.opponent > 0.0f) {
            const float distance = std::abs(obs.dxToOpponent);
            const float proximity = 1.0f / (1.0f + distance * distance * 8.0f);
            utility += m_weights.opponent * proximity * d *
                       (obs.dxToOpponent >= 0.0f ? 1.0f : -1.0f);
        }

        // Reward pull.
        if (m_weights.reward > 0.0f) {
            const int rewardDir = rewardDirection(obs);
            if (rewardDir != 0) {
                utility += m_weights.reward * d * static_cast<float>(rewardDir);
            }
        }

        // Caution: walking into a hazard or off a ledge is worth less than not.
        if (obs.at(direction, 0) == AICellState::Hazard ||
            obs.at(direction, 1) == AICellState::Hazard) {
            utility -= m_weights.caution * 4.0f;
        }
        // A gap is only a deterrent if it cannot be jumped. Airborne, it is.
        if (gapAhead(obs, direction) && !obs.canJump) {
            utility -= m_weights.caution * 3.0f;
        }

        // Anti-dither.
        if (m_commitTicks > 0 && direction != m_lastDirection) {
            utility -= kFlipPenalty;
        }
        return utility;
    };

    const float rightUtility = directionalUtility(1);
    const float leftUtility = directionalUtility(-1);
    int direction = (rightUtility >= leftUtility) ? 1 : -1;

    // --- Escape from unclimbable walls ----------------------------------------
    // Pinned (no horizontal speed) against a wall too tall for the physics to
    // ever clear: stop pressing into it and walk away for a while. kEscape
    // decisions at the Hard cadence is about 1.3 seconds — roughly six tiles of
    // backtrack, enough to bring the previous platform or block into reach.
    constexpr int kEscapeDecisions = 80;
    constexpr int kStuckDecisions = 120;   // ~2s of no horizontal motion
    if (std::abs(obs.vx) < 0.05f) ++m_stuckTicks; else m_stuckTicks = 0;
    const bool pinned = std::abs(obs.vx) < 0.05f &&
                        obstacleAhead(obs, direction) &&
                        wallHeight(obs, direction) >= 4;
    if ((pinned || m_stuckTicks >= kStuckDecisions) && m_escapeTicks == 0) {
        m_escapeTicks = kEscapeDecisions;
        m_escapeDirection = -direction;
        m_stuckTicks = 0;
    }
    if (m_escapeTicks > 0) {
        direction = m_escapeDirection;
        --m_escapeTicks;
    }

    if (direction != m_lastDirection) {
        m_commitTicks = kCommitDecisions;
        m_lastDirection = direction;
    } else if (m_commitTicks > 0) {
        --m_commitTicks;
    }

    if (direction > 0) action.moveRight = true;
    else               action.moveLeft = true;

    // --- Jump ----------------------------------------------------------------
    const bool wall = obstacleAhead(obs, direction);
    const bool gap = gapAhead(obs, direction);
    const int threat = enemyAhead(obs, direction);
    // An enemy is a jump target rather than an obstacle: landing on it is worth
    // points to every archetype, and worth a bounce towards the opponent to a
    // Hunter. Only jump once it is close enough that the arc will actually land.
    const bool stompable = threat > 0 && threat <= 2;
    // Reward directly overhead — a question block to punch, or a coin to reach.
    const bool rewardAbove = obs.at(0, -2) == AICellState::Reward ||
                             obs.at(direction, -2) == AICellState::Reward;

    // A wall only earns a jump if the physics can actually clear it — jumping
    // at a 4-tile face is the exact futile loop the escape state exists to
    // break, so the two must agree on what "climbable" means.
    const bool climbableWall = wall && wallHeight(obs, direction) <= 3;
    // A jump from an edge toward raised ground beyond it. An earlier version
    // also pressed run here, to stretch the arc toward level_3's wall top —
    // measured result: every gap-jump in every level became a 300px/s dive,
    // and bonus_1's agent overflew its falling platform into the same pit 59
    // times in a row. The run-boost helped one hand-analysed jump and endangered
    // all the others, so it is gone; the walk-speed arc crosses every pit the
    // old policy crossed.
    const bool launchJump = gap && raisedGroundAhead(obs, direction);

    if (obs.canJump && (climbableWall || gap || stompable ||
                        (rewardAbove && m_weights.reward > 0.5f))) {
        action.jump = true;
    }

    // --- Run -----------------------------------------------------------------
    // Running into an unseen gap is how a bot kills itself, so the run button is
    // gated on the ground ahead being known-good.
    // Two speed gates were tried here and both measurably backfired: a
    // jump-only-at-walk-speed rule broke level_2 (ice halves deceleration, so
    // the bot could never slow enough to be ALLOWED to jump), and a
    // no-run-in-mid-air rule traded level_3 +18 / bonus_1 +47 for level_1 -16 /
    // level_2 -73. Measured conclusion, kept here so nobody re-walks the loop:
    // this policy's outcomes are chaotically sensitive to speed tweaks — each
    // one reshuffles WHICH level fails instead of removing failures. That is a
    // ceiling of reactive control, and it is the concrete case for the learned
    // policy this seam exists to host.
    const bool clearAhead = !wall && !gap && threat == 0;
    if (clearAhead && m_weights.runBias > 0.5f) {
        action.run = true;
    }

    // --- Shoot ---------------------------------------------------------------
    // At the opponent if this archetype hunts and the opponent is roughly level
    // and in front; otherwise at whatever enemy is in the way.
    if (m_weights.aggression > 0.5f && std::abs(obs.dyToOpponent) < 0.15f &&
        ((direction > 0) == (obs.dxToOpponent > 0.0f)) &&
        std::abs(obs.dxToOpponent) < 0.5f) {
        action.shoot = true;
        m_lastReason = "hunting";
    } else if (threat > 0 && threat <= kStrikeRange) {
        action.shoot = true;
        m_lastReason = stompable ? "stomping" : "clearing";
    } else if (m_escapeTicks > 0) {
        m_lastReason = "backing off";
    } else if (launchJump) {
        m_lastReason = "launch jump";
    } else if (gap) {
        m_lastReason = "crossing gap";
    } else if (wall && !climbableWall) {
        m_lastReason = "wall too tall";
    } else if (wall) {
        m_lastReason = "climbing";
    } else if (m_weights.reward > 0.5f && rewardDirection(obs) != 0) {
        m_lastReason = "collecting";
    } else {
        m_lastReason = "advancing";
    }

    // --- Ground pound --------------------------------------------------------
    // Only ever as a fast way down onto something worth landing on, and only
    // from a height. A bot that ground-pounds on the flat just stops moving.
    if (!obs.onGround && obs.vy > 0.2f && threat > 0 &&
        obs.at(0, 2) == AICellState::Enemy) {
        action.groundPound = true;
    }

    return action;
}
