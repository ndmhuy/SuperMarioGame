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
    // Is the ground about to run out? Probes the tiles under the next two steps.
    static bool gapAhead(const AIObservation& obs, int direction);
    // Nearest enemy within striking distance ahead, in tiles, or 0 if none.
    static int enemyAhead(const AIObservation& obs, int direction);
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
};
