#pragma once

#include "Entities/Entity.hpp"

class Enemy;

class Fireball : public Entity {
public:
    Fireball(sf::Vector2f pos, sf::Vector2f vel);
    ~Fireball() override = default;

    void update(float dt) override;
    void render(sf::RenderTarget& target) override;
    EntityCategory getCategory() const override { return EntityCategory::Projectile; }

    float getLifetime() const { return m_lifetime; }
    int getBouncesLeft() const { return m_bouncesLeft; }

    void bounce();

private:
    float m_lifetime = 3.0f; // 3 seconds max lifetime
    int m_bouncesLeft = 4;
    float m_animTimer = 0.0f;
};
