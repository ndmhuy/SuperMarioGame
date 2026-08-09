#pragma once

#include "Entities/Block.hpp"

class Flagpole : public Block {
public:
    explicit Flagpole(sf::Vector2f position, float poleHeight = 300.0f);
    ~Flagpole() override = default;

    void onHitFromBelow(Player& player) override;
    void render(sf::RenderTarget& target) override;
    void setupAnimations(const SpriteSheet* spriteSheet) override;

    // Calculates completion score based on player's collision Y coordinate
    void onPlayerCollision(Player& player, float collisionY);

    float getPoleHeight() const { return m_poleHeight; }
    bool isTriggered() const { return m_triggered; }

private:
    float m_poleHeight = 300.0f;
    bool m_triggered = false;
};
