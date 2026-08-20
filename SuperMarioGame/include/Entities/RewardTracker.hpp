#pragma once

#include "Core/EventBus.hpp"

#include <SFML/System/Vector2.hpp>

// The reinforcement signal, assembled from events the game already publishes.
//
// The reward function is an EventBus subscriber rather than code inside the
// policy, and that is the whole point: coins, kills, damage, power-ups and level
// completion are all already events with existing publishers, so shaping the
// reward means changing the weights here and nothing else. No gameplay code
// knows a reward exists.
//
// One instance per learning agent. It accumulates between decisions and is
// drained by consume(), so the value handed to a trainer is "what happened since
// you last acted" — which is what a transition needs, rather than a running
// total that would credit every step with the whole episode.
class RewardTracker {
public:
    // Per-event weights. Public and plain because tuning them is the main lever
    // anyone training against this will pull.
    struct Weights {
        float coin = 1.0f;
        float enemyDefeated = 2.0f;
        float powerUp = 5.0f;
        float starCoin = 10.0f;
        float levelComplete = 100.0f;
        float damaged = -10.0f;
        float died = -50.0f;
        // Extra fine, ON TOP of `died`, for dying in the void off a JUMP —
        // leaping into a pit is a chosen risk in a way that being caught by an
        // enemy is not, and pricing it separately teaches "walk to the edge
        // and look" without pricing exploration itself (which a bigger flat
        // death penalty does — measured: it teaches standing still).
        float voidJumpDeath = -8.0f;
        // Per pixel of rightward progress. Small, because it is paid every step
        // and would otherwise dwarf everything else — but non-zero, because
        // without it an agent that stands still is never told it is wrong.
        float progressPerPixel = 0.01f;
        // Paid every decision regardless. Mildly negative, so dawdling costs.
        float timeStep = -0.001f;
    };

    RewardTracker();
    ~RewardTracker();

    RewardTracker(const RewardTracker&) = delete;
    RewardTracker& operator=(const RewardTracker&) = delete;

    // Call once per frame with the agent's position, so rightward progress can be
    // credited. Progress is measured against the furthest point reached, not the
    // previous frame: paying for every step right and refunding every step left
    // would let an agent farm reward by pacing back and forth.
    void observe(sf::Vector2f agentPosition);

    // Take everything accumulated since the last call and reset to zero.
    float consume();

    // Total for the episode so far, for the dev overlay. Not reset by consume().
    float episodeTotal() const { return m_episodeTotal; }

    // New episode: forget the progress mark and the running total.
    void reset(sf::Vector2f startPosition);

    Weights& weights() { return m_weights; }
    const Weights& weights() const { return m_weights; }

    // Bank an out-of-band reward event — the trainer uses it for penalties
    // only it can attribute, like the void-after-a-jump fine. Flows into the
    // same accumulator the event subscriptions feed.
    void add(float amount);

private:

    Weights m_weights;
    float m_pending = 0.0f;
    float m_episodeTotal = 0.0f;
    float m_furthestX = 0.0f;
    bool m_haveMark = false;

    // Scoped so the subscriptions cannot outlive this object — the callbacks
    // capture `this`, and a stale one is a use-after-free the next time a coin
    // is collected (audit X-7).
    EventBus::ScopedSubscription m_coinSub;
    EventBus::ScopedSubscription m_enemySub;
    EventBus::ScopedSubscription m_powerUpSub;
    EventBus::ScopedSubscription m_starCoinSub;
    EventBus::ScopedSubscription m_levelSub;
    EventBus::ScopedSubscription m_damagedSub;
    EventBus::ScopedSubscription m_diedSub;
};
