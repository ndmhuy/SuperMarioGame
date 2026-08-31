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
    bool onPlayerTouch(Player& player, const CollisionInfo& info, bool stomped) override;

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

    // Public so the collision resolver can ask before paying out a stomp.
    //
    // This was protected, so the resolver could not tell an invulnerable boss
    // from a vulnerable one: it called onStomped() every frame of contact and
    // awarded score and combo regardless of whether takeHit() actually landed.
    // Standing on BoomBoom therefore paid 2000 x combo *per frame* and still
    // landed one real hit per second, which is the whole fight in three seconds.
    bool isInvulnerable() const { return m_invulnerableTimer > 0.0f; }

    // Contact with a boss is only a stomp if the player is genuinely coming down
    // onto it. Standing on top is not a stomp, and neither is resting there while
    // the i-frames lapse.
    static constexpr float STOMP_MIN_DESCENT_SPEED = 60.0f;

    // Take a stomp and report whether it actually cost health.
    //
    // onStomped() returns void and swallows takeHit()'s result, so the resolver
    // had no way to tell a landed hit from one absorbed by i-frames — and paid
    // score for both. This is the same hit, with the answer.
    bool tryStomp() { return takeHit(); }

    // --- Stagger: the window in which a boss can be hit safely ---------------
    //
    // Bowser was reported as nearly impossible, and the reason is structural
    // rather than a matter of numbers. A boss carries a second of i-frames
    // after every hit, and CollisionResolver treats contact during those frames
    // as ordinary contact — it damages the player. So the only safe input is a
    // fast descending stomp landed at a moment the player cannot see coming,
    // against an enemy who is walking at them and breathing fire.
    //
    // A stagger is the answer: a period during which the boss stops attacking,
    // stops moving, drops its guard, and can be struck without hurting whoever
    // strikes it. It is what turns an unreadable fight into a rhythm — apply
    // pressure, wait for the opening, take it. How a boss earns a stagger is
    // its own business; Bowser earns one by absorbing fireballs.
    bool isStaggered() const { return m_staggerTimer > 0.0f; }
    float getStaggerTimer() const { return m_staggerTimer; }

    // End the fight immediately, whatever the health bar says.
    //
    // For the non-combat solution: the axe at the end of Bowser's bridge does
    // not chip away at him, it drops the floor out from under him. Routed
    // through takeHit() so the score, the BossDefeated event and the defeat
    // animation all happen exactly as they would after a fifth stomp.
    void defeatNow();

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

    // Open the window. Clears the i-frames as well, so a stagger earned one
    // frame after a hit is not swallowed by the invulnerability from that hit —
    // which would present the player with an opening that is not really there.
    void stagger(float seconds);

    // Close the window early. A boss that takes its hit should recover rather
    // than stand open for the rest of the three seconds — otherwise one opening
    // pays for the whole health bar, because the i-frames are gone while it
    // runs and a player can bounce twice a second.
    void endStagger();

    // Called once when a stagger begins and once when it ends, so a subclass can
    // change its sprite or play a cue. Default to nothing.
    virtual void onStaggerBegan() {}
    virtual void onStaggerEnded() {}

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
    float m_staggerTimer = 0.0f;
    float m_defeatTimer = 0.0f;
    bool m_defeatAnnounced = false;

    AABB m_arena;
};
