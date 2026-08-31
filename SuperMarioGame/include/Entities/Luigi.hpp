#pragma once

#include <string>

#include "Entities/Player.hpp"

class Luigi : public Player {
public:
    explicit Luigi(sf::Vector2f pos);
    ~Luigi() override = default;

    std::string getTypeName() const override { return "luigi"; }

    void update(float dt) override;
    void render(sf::RenderTarget& target) override;
    void setupAnimations(const SpriteSheet* spriteSheet) override;

    void jump() override;
    void doubleJump();
    float getGravityMultiplier() const override;
    float getRunSpeed() const override;
    std::string getCharacterName() const override;

private:
    bool m_hasDoubleJumped = false;
};
