#pragma once

#include <string>

#include "Entities/Player.hpp"

class Mario : public Player {
public:
    explicit Mario(sf::Vector2f pos);
    ~Mario() override = default;

    std::string getTypeName() const override { return "mario"; }

    void update(float dt) override;
    void render(sf::RenderTarget& target) override;
    void setupAnimations(const SpriteSheet* spriteSheet) override;

    std::string getCharacterName() const override;
};
