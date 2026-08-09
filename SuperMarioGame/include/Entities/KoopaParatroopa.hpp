#pragma once

#include "Entities/KoopaTroopa.hpp"

class KoopaParatroopa : public KoopaTroopa {
public:
    explicit KoopaParatroopa(sf::Vector2f position, bool isRed = false);
    virtual ~KoopaParatroopa() override = default;

    void update(float dt) override;
    void setupAnimations(const SpriteSheet* spriteSheet) override;
    void onStomped() override;
    void onHitByFireball() override;

    // Getters for state/unit testing
    bool hasWings() const { return m_hasWings; }

private:
    bool m_hasWings = true;
};
