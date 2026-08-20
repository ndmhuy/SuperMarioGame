#pragma once

#include "Core/GameMode.hpp"
#include "Entities/IAIPolicy.hpp"

// The shipped AI brain: a utility-scored decision over the vision grid, with the
// archetype supplying the weights.
//
// Deliberately not a neural network. The three archetypes in
// docs/two_player_ai_plan.md §2B are described as "reward weightings", and that
// is exactly what they are here — the same rules scored differently, so a Hunter
// and a Speedrunner in the same spot make visibly different choices without
// either being a separate code path to maintain.
class HeuristicPolicy : public IAIPolicy {
public:
    explicit HeuristicPolicy(AIArchetype archetype);

    AIAction decide(const AIObservation& observation) override;
    const char* name() const override;
    void reset() override;

    void setArchetype(AIArchetype archetype);
    AIArchetype getArchetype() const { return m_archetype; }

    // What the last decision was reasoning about, for the dev overlay's AI
    // panel. A bot that walks into a wall is much easier to fix when the overlay
    // says it thought the wall was the way to the goal.
    const char* lastReason() const { return m_lastReason; }

private:
    // The archetype, as numbers. Every weight is a multiplier on one term of the
    // horizontal-direction utility, except `runBias` which gates the run button
    // and `aggression` which gates shooting and opponent-chasing.
    struct Weights {
        float goal = 1.0f;        // pull towards the level exit
        float opponent = 0.0f;    // pull towards the human player
        float reward = 0.0f;      // pull towards coins and items
        float runBias = 1.0f;     // 0 never runs, 1 always runs when it is safe
        float aggression = 0.0f;  // 0 ignores the opponent, 1 hunts it
        float caution = 0.3f;     // weight on backing away from hazards
    };

    static Weights weightsFor(AIArchetype archetype);

    // Is there a wall directly ahead that has to be jumped rather than walked
    // through? Looks at the two cells at foot and head height.
    static bool obstacleAhead(const AIObservation& obs, int direction);
    // Height of that wall in tiles (0..4), counted upward from foot level.
    // The number that matters is 3: the engine's discrete jump rises at most
    // 122.4px = 3.82 tiles (v0^2/2g minus the semi-implicit Euler half-frame),
    // so a 3-tile ledge is landable with 26px to spare and a 4-tile ledge is
    // physically out of reach no matter how the jump is timed.
    static int wallHeight(const AIObservation& obs, int direction);
    // Is there raised ground 2..5 tiles ahead whose top sits 1..3 tiles above
    // the current walking row — i.e. something a running jump from HERE can
    // land on? This is what makes a jump from a platform edge aim for the
    // ledge beyond it instead of dropping into the trough below.
    static bool raisedGroundAhead(const AIObservation& obs, int direction);
    // Is the ground about to run out? Probes the tiles under the next two steps.
    static bool gapAhead(const AIObservation& obs, int direction);
    // Nearest enemy within striking distance ahead, in tiles, or 0 if none.
    static int enemyAhead(const AIObservation& obs, int direction);
    // Same, but only enemies a stomp actually defeats. The two are separate
    // because "something is in my way" and "I can jump on it" are different
    // questions: the first says slow down, the second says jump. Before the
    // observation split these were indistinguishable, so this policy happily
    // jumped onto Spinies.
    static int stompableAhead(const AIObservation& obs, int direction);
    static bool isEnemy(AICellState state);
    static bool isReward(AICellState state);
    // Signed tile offset to the most attractive Reward cell in view, or 0.
    static int rewardDirection(const AIObservation& obs);

    AIArchetype m_archetype;
    Weights m_weights;
    const char* m_lastReason = "idle";

    // Anti-dither: a direction flip costs a little utility for a few decisions
    // after the last one, so the bot cannot oscillate on the spot when two
    // choices score almost equally.
    int m_lastDirection = 1;
    int m_commitTicks = 0;

    // Escape: when the bot is pinned against a wall it cannot climb (4+ tiles,
    // see wallHeight), pressing on is provably useless — the level_3 agent
    // spent 193 seconds jumping at one. Walking away for a while is the only
    // local move that can change anything: it puts platforms, blocks and
    // trampolines behind the bot back into play as launch points.
    int m_escapeTicks = 0;
    int m_escapeDirection = 0;
    // Stuck detector: consecutive decisions with no horizontal motion. The
    // wall-height trigger only covers the trap we understood; this covers the
    // ones we did not — bonus_1's agent spent 168 seconds bouncing on a koopa
    // shell wedged against a pipe, never grounded, never moving, and no rule
    // about walls could see that. Two seconds without horizontal movement is
    // wrong in this game no matter what is causing it.
    int m_stuckTicks = 0;
    // Wall-scaling (v4): when pinned at a wall too tall to jump, try to
    // wall-jump up it for a while before conceding and backing off.
    int m_wallScaleTicks = 0;
    int m_wallScaleDirection = 0;
};
