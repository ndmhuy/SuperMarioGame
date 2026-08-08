#pragma once

#include "Entities/Block.hpp"

class Flagpole : public Block {
public:
    explicit Flagpole(sf::Vector2f position, float poleHeight = 300.0f);
    ~Flagpole() override = default;

    void update(float dt) override;
    void onHitFromBelow(Player& player) override;
    void render(sf::RenderTarget& target) override;

    // Calculates completion score based on player's collision Y coordinate
    void onPlayerCollision(Player& player, float collisionY);

    float getPoleHeight() const { return m_poleHeight; }
    bool isTriggered() const { return m_triggered; }
    float getFlagY() const { return m_flagY; }

private:
    float m_poleHeight = 300.0f;
    bool m_triggered = false;
    float m_flagY = 0.0f;
    float m_targetFlagY = 0.0f;
    float m_animTimer = 0.0f;
};
