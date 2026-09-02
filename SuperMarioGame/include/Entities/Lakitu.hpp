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

private:
    float m_eggTimer = 0.0f;
    int m_spawnCount = 0;
};
