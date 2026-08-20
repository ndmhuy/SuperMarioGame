#pragma once

#include <string>

#include "Entities/Enemy.hpp"

class Boo : public Enemy {
public:
    explicit Boo(sf::Vector2f position);
    virtual ~Boo() override = default;

    std::string getTypeName() const override { return "boo"; }

    void update(float dt) override;
    void render(sf::RenderTarget& target) override;
    void setupAnimations(const SpriteSheet* spriteSheet) override;

    void onStomped() override;
    // Invulnerable to stomp — onStomped() is a no-op, so a stomp can never remove it.
    bool isStompSafe() const override { return false; }
    void onHitByFireball() override;

    float getGravityMultiplier() const override { return 0.0f; }
    bool collidesWithTiles() const override { return false; }

private:
    Animation m_seenAnim;
    Animation m_moveAnim;
};
