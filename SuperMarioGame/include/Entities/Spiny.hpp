#pragma once

#include "Entities/Enemy.hpp"

class Spiny : public Enemy {
public:
    explicit Spiny(sf::Vector2f position, bool isEgg = false);
    ~Spiny() override = default;

    void onStomped() override;
    void onHitByFireball() override;
    void update(float dt) override;
    void setupAnimations(const SpriteSheet* spriteSheet) override;
    const AABB& getBoundingBox() const override;

    bool isFlipped() const { return m_isFlipped; }
    bool isEgg() const { return m_isEgg; }
    void setEgg(bool isEgg) { m_isEgg = isEgg; }

private:
    bool m_isFlipped = false;
    bool m_isEgg = false;
    Animation m_eggAnim;
};
