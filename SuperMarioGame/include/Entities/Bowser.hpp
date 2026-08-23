#pragma once

#include "Entities/Boss.hpp"
#include <string>

// Task 9.3 — the Bowser fight.
//
// SPEC 6.x: "Phase 1 (walk + breathe fire), Phase 2 (jump + faster fire)",
// multi-hit with a health bar, 5000 points. Phase 2 begins at half health,
// which is Boss's default split.
//
// Bowser is immune to fire — he breathes it — so the only way through the bar
// is stomping him, which is what makes the fight a fight.
class Bowser : public Boss {
public:
    explicit Bowser(sf::Vector2f position);
    ~Bowser() override = default;

    std::string getTypeName() const override { return "bowser"; }

    void setupAnimations(const SpriteSheet* spriteSheet) override;

    // Breathing fire does not make him vulnerable to it — but it does wear him
    // down. Every fireball that lands counts; the fourth staggers him, and a
    // staggered Bowser can be stomped safely and without i-frames.
    //
    // This is what makes the fight winnable. Before it, fire did literally
    // nothing and the only route through the health bar was five clean
    // descending stomps landed on a boss who is walking at you, breathing fire,
    // and hurting you on contact for a second after every hit you land.
    void onHitByFireball() override;

    // How many fireballs are still needed for the next opening. Drawn under the
    // boss health bar, because a mechanic the player cannot see the state of is
    // a mechanic they will not find.
    int getFireHitsToStagger() const;
    static constexpr int FIRE_HITS_PER_STAGGER = 4;

protected:
    void updateBehaviour(float dt) override;
    void onPhaseChanged(int newPhase) override;
    void onTookHit() override;
    void onStaggerBegan() override;
    void onStaggerEnded() override;

private:
    void breatheFire();
    // Not named jump(): Character::jump() is the ordinary character jump, and
    // silently overriding it would route Bowser's phase-2 leap through it.
    void leap();
    // Fire interval and walk speed both come from the current phase, so the
    // fight visibly escalates rather than relying on the health bar alone.
    float fireInterval() const;
    float walkSpeed() const;

    float m_fireTimer = 0.0f;
    float m_jumpTimer = 0.0f;
    // Fireballs absorbed since the last stagger. Reset when the window opens,
    // so the cost of each opening is the same all through the fight.
    int m_fireHits = 0;
    // Bowser paces between these two x values. Set from the arena when one is
    // assigned, so level design controls the pacing width.
    float m_patrolLeft = 0.0f;
    float m_patrolRight = 0.0f;
    bool m_patrolInitialised = false;

    Animation m_walkLeftAnim;
    Animation m_walkRightAnim;
};
