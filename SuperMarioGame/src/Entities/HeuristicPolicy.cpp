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
    m_wallScaleTicks = 0;
    m_wallScaleDirection = 0;
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

bool HeuristicPolicy::isEnemy(AICellState state) {
    return state == AICellState::EnemyStompable || state == AICellState::EnemyDangerous;
}

bool HeuristicPolicy::isReward(AICellState state) {
    return state == AICellState::Coin || state == AICellState::PowerUp ||
           state == AICellState::ItemStar || state == AICellState::ItemOneUp;
}

int HeuristicPolicy::enemyAhead(const AIObservation& obs, int direction) {
    for (int step = 1; step <= kStrikeRange; ++step) {
        const int dx = direction * step;
        // Same row or one above: an enemy on the ground the agent is walking on.
        if (isEnemy(obs.at(dx, 0)) || isEnemy(obs.at(dx, -1))) {
            return step;
        }
    }
    return 0;
}

int HeuristicPolicy::stompableAhead(const AIObservation& obs, int direction) {
    for (int step = 1; step <= kStrikeRange; ++step) {
        const int dx = direction * step;
        if (obs.at(dx, 0) == AICellState::EnemyStompable ||
            obs.at(dx, -1) == AICellState::EnemyStompable) {
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
            if (!isReward(obs.at(dx, dy))) continue;
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
        //
        // A dangerous enemy is deliberately NOT counted here. It was, briefly,
        // and it cost level_2 73 percentage points (75.3% -> 1.8%): a Spiny on
        // the route made the goal pull negative, so the agent turned round and
        // spent the episode oscillating. The correct response to a Spiny is to
        // jump OVER it, which is a jump trigger (below), not a reason to
        // reverse. Fleeing is right for lava, which cannot be jumped from
        // standing; it is wrong for something one tile tall.
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
    // A 4-tile wall is unjumpable but it is not unclimbable: the physics has a
    // wall-jump (press into the wall airborne, jump to kick off it), and the
    // controller maps the jump button to it when airborne and onWall. So the
    // first answer to "pinned at a tall wall" is now to try scaling it —
    // hold into the wall, jump from the ground, then jump again each time the
    // wall is touched — and only concede to the backing-off escape when a
    // scaling budget runs out. Backing off remains the answer for walls the
    // scaling cannot climb; it stops being the FIRST answer for all of them.
    constexpr int kWallScaleDecisions = 60;
    if (pinned && m_wallScaleTicks == 0 && m_escapeTicks == 0) {
        m_wallScaleTicks = kWallScaleDecisions;
        m_wallScaleDirection = direction;
    }
    bool wallScaling = false;
    if (m_wallScaleTicks > 0 && m_escapeTicks == 0) {
        --m_wallScaleTicks;
        direction = m_wallScaleDirection;
        wallScaling = true;
        // Climbed it, or the wall is gone: back to normal control.
        if (!obstacleAhead(obs, direction) && obs.onGround) {
            m_wallScaleTicks = 0;
            wallScaling = false;
        }
    }
    if ((pinned || m_stuckTicks >= kStuckDecisions) && m_escapeTicks == 0 &&
        m_wallScaleTicks == 0) {
        m_escapeTicks = kEscapeDecisions;
        m_escapeDirection = -direction;
        m_stuckTicks = 0;
    }
    if (m_escapeTicks > 0) {
        direction = m_escapeDirection;
        --m_escapeTicks;
        // Escape overrides the utility sum — including its caution term — so
        // an escaping bot walked blind. On level_3 that was fatal, repeatedly:
        // pinned at the wall after the lava trough, it backed off LEFT, over
        // the trough, and the hazard probes above only look one row down — a
        // ledge with lava four tiles below it reads as safe. Scan the floor of
        // the next two columns instead: if the first thing under either is
        // Hazard, this escape ends at a cliff edge over lava, and continuing
        // is not "backing off", it is drowning. End it and turn round.
        auto lethalDropAhead = [&](int dir) {
            for (int step = 1; step <= 2; ++step) {
                for (int dy = 1; dy <= 5; ++dy) {
                    const AICellState below = obs.at(dir * step, dy);
                    if (below == AICellState::Hazard) return true;
                    if (below == AICellState::Solid) break;   // safe floor first
                }
            }
            return false;
        };
        if (lethalDropAhead(direction)) {
            m_escapeTicks = 0;
            direction = -direction;
        }
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
    // A STOMPABLE enemy is a jump target rather than an obstacle: landing on it
    // is worth points to every archetype. A dangerous one (Spiny, Piranha,
    // Thwomp, ChainChomp, Boo) is not — jumping on it costs a hit — so it is
    // only ever an obstacle, and the caution term below handles it.
    const int stompTarget = stompableAhead(obs, direction);
    const bool stompable = stompTarget > 0 && stompTarget <= 2;
    // A dangerous enemy close ahead must be cleared, not landed on. Same jump,
    // different intent — and because it is not in `stompable`, the policy will
    // not aim for its head.
    const bool mustClear = (obs.at(direction, 0) == AICellState::EnemyDangerous) ||
                           (obs.at(direction * 2, 0) == AICellState::EnemyDangerous);
    // Reward directly overhead — a question block to punch, or a coin to reach.
    // Only a power-up is worth an unprompted jump; a floating coin is not
    // worth leaving the ground for on its own.
    const bool rewardAbove = obs.at(0, -2) == AICellState::PowerUp ||
                             obs.at(direction, -2) == AICellState::PowerUp;

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
    // The certified route climbs here. dyToGoal is zero unless waypoints are
    // loaded (AIController::setWaypoints), so this trigger is inert in the
    // shipped game. With them, it fires when the next route node is more than
    // a tile above and roughly overhead — which is knowledge the reactive
    // triggers cannot have: a platform the route climbs onto may hold no wall,
    // no gap and no reward, and every trigger above stays silent while the
    // agent walks under it. One tile is dy = -0.1 at the vision normalizer, so
    // -0.12 means "meaningfully above", not jitter.
    const bool routeClimbs = obs.dyToGoal < -0.12f && std::abs(obs.dxToGoal) < 0.35f;

    // Waiting is an action (v4). A gap with no far side in reach used to force
    // a choice between two bad options — jump anyway or turn around. If a
    // MOVING solid is in view ahead (a platform on its way, visible in the
    // motion planes), the right move is the one no trigger could express
    // before: stand still and let it arrive. Board it when it is close.
    bool waiting = false;
    if (gap && obs.onGround && !raisedGroundAhead(obs, direction)) {
        for (int step = 2; step <= 8 && !waiting; ++step) {
            for (int dy = -3; dy <= 4 && !waiting; ++dy) {
                const int dx = direction * step;
                if (obs.at(dx, dy) != AICellState::Solid) continue;
                const sf::Vector2f v = obs.velocityAt(dx, dy);
                if (std::abs(v.x) > 0.03f || std::abs(v.y) > 0.03f) {
                    waiting = true;
                }
            }
        }
    }
    if (waiting) {
        action.moveLeft = false;
        action.moveRight = false;
        action.run = false;
    }

    if (wallScaling) {
        // Grounded at the wall: jump onto it. Airborne against it: the jump
        // button is a wall-jump (AIController maps it), which kicks off and
        // regains full jump speed — pressing back into the wall between kicks
        // ratchets upward.
        if (obs.canJump || obs.onWall) action.jump = true;
    } else if (!waiting && obs.canJump &&
               (climbableWall || gap || stompable || mustClear ||
                routeClimbs ||
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
    } else if (waiting) {
        m_lastReason = "awaiting platform";
    } else if (wallScaling) {
        m_lastReason = "scaling wall";
    } else if (m_escapeTicks > 0) {
        m_lastReason = "backing off";
    } else if (launchJump) {
        m_lastReason = "launch jump";
    } else if (routeClimbs) {
        m_lastReason = "route climbs";
    } else if (gap) {
        m_lastReason = "crossing gap";
    } else if (mustClear) {
        m_lastReason = "clearing spikes";
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
        obs.at(0, 2) == AICellState::EnemyStompable) {
        action.groundPound = true;
    }

    return action;
}
