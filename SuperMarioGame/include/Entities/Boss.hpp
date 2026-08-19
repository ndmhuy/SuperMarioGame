#pragma once

#include "Entities/Enemy.hpp"
#include "Physics/AABB.hpp"
#include <string>

// A multi-hit enemy with a health bar, phases and an arena.
//
// HudData has carried bossActive / bossName / bossHealth / bossMaxHealth since
// the HUD was written, and Hud::draw has always drawn them — but nothing ever
// produced them, because no boss existed. This is the type that produces them.
//
// Bosses do not use an IMovementStrategy. A strategy describes one repeatable
// motion; a boss is a sequenced fight whose behaviour changes with its own
// health, so the behaviour lives in the subclass and is driven from here
// through updateBehaviour(). Everything a fight shares — health, i-frames,
// phase transitions, the defeat sequence, the arena — is here so a new boss
// only writes its own attack pattern.
class Boss : public Enemy {
public:
    Boss(sf::Vector2f position, std::string displayName, int maxHealth,
         int scoreValue, sf::Vector2f size);
    ~Boss() override = default;

    // Sequenced here, so no subclass can forget the i-frames or the defeat
    // sequence. Subclasses extend the fight through updateBehaviour().
    void update(float dt) final;

    // Enemy's two damage hooks both funnel into takeHit(), so a boss can never
    // be removed by the one-stomp path an ordinary Enemy uses.
    void onStomped() override;
    void onHitByFireball() override;

    int getHealth() const { return m_health; }
    int getMaxHealth() const { return m_maxHealth; }
    int getPhase() const { return m_phase; }
    const std::string& getDisplayName() const { return m_displayName; }
    bool isDefeated() const { return m_health <= 0; }

    // The room the fight happens in, in world coordinates. PlayingState locks
    // the camera to it and keeps the player inside while the boss lives. A
    // zero-width arena means "no arena" and the camera is left alone.
    const AABB& getArena() const { return m_arena; }
    void setArena(const AABB& arena) { m_arena = arena; }
    bool hasArena() const { return m_arena.width > 0.0f && m_arena.height > 0.0f; }

    // A boss is only dead once its defeat animation has finished, so the HUD bar
    // does not vanish the instant the last hit lands.
    bool isDeadOrDying() const override { return !active || isDefeated(); }

protected:
    // The subclass's own fight logic, called once per frame while the boss is
    // alive. Gravity and collision are still the physics engine's job.
    virtual void updateBehaviour(float dt) = 0;

    // Which phase the given health is in. The default is the two-phase split
    // the SPEC describes for Bowser: phase 2 below half health.
    virtual int phaseForHealth(int health) const;

    // Notifications. Default to nothing so a simple boss overrides neither.
    virtual void onPhaseChanged(int newPhase) {}
    virtual void onTookHit() {}
    virtual void onDefeated() {}

    // Costs one health point unless the boss is still in its i-frames.
    // Returns true if the hit actually landed.
    bool takeHit(int amount = 1);
    bool isInvulnerable() const { return m_invulnerableTimer > 0.0f; }

    // True while the defeat sequence is playing, for subclasses that want to
    // stop attacking without checking the health themselves.
    bool isDying() const { return isDefeated() && active; }

private:
    std::string m_displayName;
    int m_maxHealth;
    int m_health;
    int m_phase = 1;

    // Contact damage arrives every frame the player overlaps the boss, so
    // without this a single stomp would drain the whole bar.
    float m_invulnerableTimer = 0.0f;
    float m_defeatTimer = 0.0f;
    bool m_defeatAnnounced = false;

    AABB m_arena;
};
