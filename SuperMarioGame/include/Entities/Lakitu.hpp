#pragma once

#include <string>

#include "Entities/Enemy.hpp"

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
    int getSpawnCount() const { return m_spawnCount; }

    // How many Spinies one Lakitu will ever drop. FlyStrategy::FollowPlayer
    // tracks the player from the moment the level loads and never gives up, so
    // an uncapped Lakitu is an endless Spiny fountain for the rest of the
    // stage: at one egg every 4 s that is fifteen-plus Spinies over a 200-tile
    // level, and a Spiny cannot be stomped. m_spawnCount was already being
    // counted and never read — this is the limit it was counting towards.
    static constexpr int MAX_SPINIES = 3;

private:
    float m_eggTimer = 0.0f;
    int m_spawnCount = 0;
};
