#pragma once

#include <SFML/Graphics.hpp>
#include "Graphics/SpriteSheet.hpp"
#include <string>

class PipeRenderer {
public:
    static void draw(sf::RenderTarget& target, 
                     const SpriteSheet* scenerySheet, 
                     sf::Vector2f position, 
                     sf::Vector2f size, 
                     float rotationDegrees = 0.0f, 
                     bool hasHead = true,
                     const std::string& color = "green");
};
