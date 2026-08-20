#pragma once

#include <string>

#include "Entities/Block.hpp"

class Flagpole : public Block {
public:
    explicit Flagpole(sf::Vector2f position, float poleHeight = 300.0f);
    ~Flagpole() override = default;

    std::string getTypeName() const override { return "flagpole"; }

    void update(float dt) override;
    void onHitFromBelow(Player& player) override;
    void render(sf::RenderTarget& target) override;
    void setupAnimations(const SpriteSheet* spriteSheet) override;

    // Calculates completion score based on player's collision Y coordinate
    void onPlayerCollision(Player& player, float collisionY);

    float getPoleHeight() const { return m_poleHeight; }
    bool isTriggered() const { return m_triggered; }
    // How far the flag has slid down the pole, in pixels from the top. Zero
    // until the player touches it, m_poleHeight once the descent finishes.
    float getFlagY() const { return m_flagY; }

private:
    // Two clips: the flag waiting at the top, and the one-shot slide down the
    // pole played when the player touches it. Block::m_animation holds a copy of
    // whichever is current; these are kept as the definitions to copy from.
    Animation m_raisedAnimation;
    Animation m_descentAnimation;

    float m_poleHeight = 300.0f;
    bool m_triggered = false;
    // These three were placeholders for a slide-down that was never written —
    // m_flagY stayed at zero forever, so getFlagY() always answered "at the top"
    // and two of them were dead weight (-Wunused-private-field). They now track
    // the real descent animation.
    float m_flagY = 0.0f;
    float m_targetFlagY = 0.0f;
    float m_animTimer = 0.0f;
};
