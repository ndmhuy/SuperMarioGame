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

private:
    float m_eggTimer = 0.0f;
    int m_spawnCount = 0;
};
