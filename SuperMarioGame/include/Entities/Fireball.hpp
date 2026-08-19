#pragma once

#include "Entities/Projectile.hpp"

class Enemy;

// The player's fire-flower shot: bounces along the ground and kills what it
// touches. Harmless to the player who threw it.
class Fireball : public Projectile {
public:
    Fireball(sf::Vector2f pos, sf::Vector2f vel);
    ~Fireball() override = default;

    void update(float dt) override;
    void render(sf::RenderTarget& target) override;

    bool damagesEnemies() const override { return true; }
    void onHitEnemy(Enemy& enemy) override;

    float getLifetime() const { return m_lifetime; }
    int getBouncesLeft() const { return m_bouncesLeft; }

    void bounce();

private:
    float m_lifetime = 3.0f; // 3 seconds max lifetime
    int m_bouncesLeft = 4;
    float m_animTimer = 0.0f;
};
