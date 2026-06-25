#pragma once

#include "Entities/Enemy.hpp"

class Goomba : public Enemy {
public:
    explicit Goomba(sf::Vector2f position, bool isRed = false);
    virtual ~Goomba() override = default;

    void update(float dt) override;
    void render(sf::RenderTarget& target) override;

    void onStomped() override;
    void onHitByFireball() override;

    // Getters for state/unit testing
    bool isRed() const { return m_isRed; }
    bool isSquished() const { return m_isSquished; }
    bool isFlipped() const { return m_isFlipped; }
    float getSquishTimer() const { return m_squishTimer; }

    AABB getBoundingBox() const override;

private:
    bool m_isRed;
    bool m_isSquished = false;
    bool m_isFlipped = false;
    float m_squishTimer = 0.0f;
};
