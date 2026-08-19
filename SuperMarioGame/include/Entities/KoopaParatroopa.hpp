#pragma once

#include <string>

#include "Entities/KoopaTroopa.hpp"

class KoopaParatroopa : public KoopaTroopa {
public:
    explicit KoopaParatroopa(sf::Vector2f position, bool isRed = false);
    virtual ~KoopaParatroopa() override = default;

    std::string getTypeName() const override { return "koopa_paratroopa"; }

    void update(float dt) override;
    void setupAnimations(const SpriteSheet* spriteSheet) override;
    void render(sf::RenderTarget& target) override;
    void onStomped() override;
    void onHitByFireball() override;
    float getGravityMultiplier() const override;

    // Getters for state/unit testing
    bool hasWings() const { return m_hasWings; }
    bool isTransforming() const { return m_transformInvincibilityTimer > 0.0f; }

private:
    bool m_hasWings = true;
    float m_transformInvincibilityTimer = 0.0f;
    Animation m_flyAnim;
};
