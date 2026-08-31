#pragma once

#include <string>

#include "Entities/Player.hpp"

class Toad : public Player {
public:
    explicit Toad(sf::Vector2f pos);
    ~Toad() override = default;

    std::string getTypeName() const override { return "toad"; }

    void update(float dt) override;
    void render(sf::RenderTarget& target) override;
    void setupAnimations(const SpriteSheet* spriteSheet) override;
    float getRunSpeed() const override;
    std::string getCharacterName() const override;
};
