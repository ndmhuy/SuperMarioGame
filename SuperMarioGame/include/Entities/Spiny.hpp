#pragma once

#include "Entities/Enemy.hpp"

class Spiny : public Enemy {
public:
    explicit Spiny(sf::Vector2f position);
    ~Spiny() override = default;

    void onStomped() override;
    void onHitByFireball() override;
    void update(float dt) override;
    const AABB& getBoundingBox() const override;

    bool isFlipped() const { return m_isFlipped; }

private:
    bool m_isFlipped = false;
};
