#pragma once

#include <string>

#include "Entities/Block.hpp"

enum class FallingPlatformState {
    Idle,
    Shaking,
    Falling,
    Respawning
};

class FallingPlatform : public Block {
public:
    explicit FallingPlatform(sf::Vector2f position);
    ~FallingPlatform() override = default;

    std::string getTypeName() const override { return "falling_platform"; }

    void onHitFromBelow(Player& player) override;
    void update(float dt) override;
    void render(sf::RenderTarget& target) override;
    void setupAnimations(const SpriteSheet* spriteSheet) override;
    bool isCollidable() const override;

    FallingPlatformState getState() const { return m_state; }

private:
    bool isPlayerStandingOnTop() const;

    FallingPlatformState m_state = FallingPlatformState::Idle;
    float m_shakeTimer = 0.0f;
    float m_respawnTimer = 0.0f;
    sf::Vector2f m_shakeOffset;
};
