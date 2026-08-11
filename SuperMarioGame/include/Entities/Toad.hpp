#pragma once

#include "Entities/Player.hpp"

class Toad : public Player {
public:
    explicit Toad(sf::Vector2f pos);
    ~Toad() override = default;

    void update(float dt) override;
    void render(sf::RenderTarget& target) override;
    void setupAnimations(const SpriteSheet* spriteSheet) override;
    std::string getCharacterName() const override;
};
