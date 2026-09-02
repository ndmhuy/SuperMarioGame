#pragma once

#include <string>

#include "Entities/Enemy.hpp"
#include "Utils/Constants.hpp"

class Lakitu : public Enemy {
public:
    explicit Lakitu(sf::Vector2f position);
    ~Lakitu() override = default;

    std::string getTypeName() const override { return "lakitu"; }

    void onStomped() override;
    void onHitByFireball() override;
    void update(float dt) override;
    void setupAnimations(const SpriteSheet* spriteSheet) override;

    float getGravityMultiplier() const override { return 0.0f; }
    bool collidesWithTiles() const override { return false; }

    float getEggTimer() const { return m_eggTimer; }
    // Lifetime tally of eggs thrown. Kept for diagnostics and the HUD-less
    // harnesses; it is deliberately no longer what limits the drops — see
    // MAX_SPINIES.
    int getSpawnCount() const { return m_spawnCount; }

    // Engaged seconds banked towards the next Fire Flower, and the lifetime
    // tally of flowers actually dropped. Diagnostics only.
    float getFlowerTimer() const { return m_flowerTimer; }
    int getFlowerCount() const { return m_flowerCount; }

    // Is the player close enough for this to be a fight at all?
    bool isEngaged() const;

    // How many Spinies one Lakitu keeps in play AT ONCE.
    //
    // FlyStrategy::FollowPlayer tracks the player from the moment the level
    // loads and never gives up, so an ungated Lakitu is an endless Spiny
    // fountain: at one egg every 4 s that is fifteen-plus Spinies over a
    // 200-tile level, and a Spiny cannot be stomped.
    //
    // This used to be a LIFETIME cap, counted by m_spawnCount and never
    // decremented, and that is what produced the "Lakitu sometimes doesn't drop
    // the Spiny" report (R21 D8). The Lakitu in level_1 sits at tile (175,11),
    // some 5500px from the player's spawn; it burned all three drops off-camera
    // at t=4/8/12s, arrived around t=50s, and could then never drop again.
    // Counting live Spinies instead (Spiny::liveCount()) keeps the anti-flood
    // property the cap was written for while restoring the encounter.
    static constexpr int MAX_SPINIES = 3;

    // How far away the player may be and still have Lakitu winding up an egg.
    //
    // A screen width: the camera is 1280px wide, so this is "on screen, or
    // about to be". The concurrent cap alone would have made Lakitu materially
    // harder — three unstompable Spinies replenished forever — and the two
    // changes are only correct together, which is why the range lives here
    // beside the cap.
    static constexpr float ENGAGE_RANGE_X = static_cast<float>(Constants::WINDOW_WIDTH);

    // How long a player must stay in the fight before Lakitu drops a Fire
    // Flower instead of an egg.
    //
    // WHY THIS EXISTS AT ALL: Bowser needs FIRE_HITS_PER_STAGGER fireball hits
    // before a stomp does anything, so a player who arrives at the bridge as
    // Small Mario has no route to win — there is no other guaranteed flower on
    // the way. Lakitu is the recovery path.
    //
    // ENGAGED seconds, not seconds since level load: the same reason the egg
    // clock is gated (see ENGAGE_RANGE_X). A wall-clock timer would have burnt
    // the mercy drop off-camera exactly as the lifetime Spiny cap did (R21 D8).
    //
    // 20s is five egg cycles at the 4s throw rate, so the player has already
    // had to survive a full concurrent field of Spinies plus two refills before
    // the flower arrives; it cannot be rushed. It is also short enough to be
    // *reachable*: Lakitu tracks at ENEMY_LAKITU_SPEED (100 px/s) against a
    // 150 px/s walk, so even a player who simply walks away stays inside
    // ENGAGE_RANGE_X for ~25s, and one who stops to fight banks it sooner. A
    // longer interval would make the drop unobservable in ordinary play, which
    // is the "shipped and inert" failure all over again.
    static constexpr float FLOWER_DROP_INTERVAL = 20.0f;

private:
    // Publish a Fire Flower spawn request unless the player is already Fire.
    // Returns whether anything was dropped, so the caller knows whether to
    // restart the clock.
    bool dropFireFlower();

    float m_eggTimer = 0.0f;
    int m_spawnCount = 0;
    float m_flowerTimer = 0.0f;
    int m_flowerCount = 0;
};
