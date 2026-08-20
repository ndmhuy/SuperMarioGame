#pragma once

#include "Entities/Boss.hpp"
#include <string>

// Task 9.2 — the Boom Boom mid-boss.
//
// SPEC 6.4: three stomps, an enclosed arena with no escape, and an escalation
// per hit — "Hit 1: normal charge. Hit 2: faster charge + shorter recovery.
// Hit 3: fastest charge + spin attack." Defeating him opens the path to the
// Level 2 flagpole, which the arena lock enforces for free.
//
// So this is a three-phase boss, not the two-phase split Boss defaults to.
class BoomBoom : public Boss {
public:
    explicit BoomBoom(sf::Vector2f position);
    ~BoomBoom() override = default;

    std::string getTypeName() const override { return "boom_boom"; }

    void setupAnimations(const SpriteSheet* spriteSheet) override;

protected:
    void updateBehaviour(float dt) override;

    // One phase per hit taken, rather than a single switch at half health.
    int phaseForHealth(int health) const override;

    void onPhaseChanged(int newPhase) override;
    void onTookHit() override;

private:
    // Charge at the player, then stand and pant. The pause is the fight: it is
    // the only window in which he is easy to stomp.
    enum class Action { Charging, Recovering, Spinning };

    void enter(Action action);
    float chargeSpeed() const;
    float recoverDuration() const;

    Action m_action = Action::Charging;
    float m_actionTimer = 0.0f;
    // Bounces off the arena walls during the spin, counted so the spin ends.
    int m_spinBounces = 0;

    float m_arenaLeft = 0.0f;
    float m_arenaRight = 0.0f;
    bool m_arenaResolved = false;

    Animation m_walkAnim;
    Animation m_spinAnim;
};
