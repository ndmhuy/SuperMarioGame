#include "Graphics/PipeRenderer.hpp"
#include <cmath>
#include <vector>

void PipeRenderer::draw(sf::RenderTarget& target, 
                       const SpriteSheet* scenerySheet, 
                       sf::Vector2f position, 
                       sf::Vector2f size, 
                       float rotationDegrees, 
                       bool hasHead,
                       const std::string& color) {
    if (!scenerySheet) return;

    std::string key_head_l = "pipe_" + color + "_head_left";
    std::string key_head_r = "pipe_" + color + "_head_right";
    std::string key_body_l = "pipe_" + color + "_body_left";
    std::string key_body_r = "pipe_" + color + "_body_right";

    std::vector<sf::Sprite> sprites;
    sprites.reserve(4);
    if (hasHead) {
        sprites.push_back(scenerySheet->getSprite(key_head_l));
        sprites.push_back(scenerySheet->getSprite(key_head_r));
        sprites.push_back(scenerySheet->getSprite(key_body_l));
        sprites.push_back(scenerySheet->getSprite(key_body_r));
    } else {
        sprites.push_back(scenerySheet->getSprite(key_body_l));
        sprites.push_back(scenerySheet->getSprite(key_body_r));
        sprites.push_back(scenerySheet->getSprite(key_body_l));
        sprites.push_back(scenerySheet->getSprite(key_body_r));
    }

    // Quadrant center offsets relative to tile center (0-degree base offsets)
    // Top-Left, Top-Right, Bottom-Left, Bottom-Right
    sf::Vector2f offsets[4] = {
        {-size.x * 0.25f, -size.y * 0.25f},
        { size.x * 0.25f, -size.y * 0.25f},
        {-size.x * 0.25f,  size.y * 0.25f},
        { size.x * 0.25f,  size.y * 0.25f}
    };

    float angleRad = rotationDegrees * (3.1415926535f / 180.0f);
    float cosA = std::cos(angleRad);
    float sinA = std::sin(angleRad);

    sf::Vector2f center = position + sf::Vector2f(size.x * 0.5f, size.y * 0.5f);
    sf::Vector2f halfSize(size.x * 0.5f, size.y * 0.5f);

    for (int i = 0; i < 4; ++i) {
        sf::FloatRect bounds = sprites[i].getLocalBounds();
        if (bounds.size.x > 0.0f && bounds.size.y > 0.0f) {
            sprites[i].setOrigin(sf::Vector2f(bounds.size.x * 0.5f, bounds.size.y * 0.5f));
            sprites[i].setScale(sf::Vector2f(halfSize.x / bounds.size.x, halfSize.y / bounds.size.y));

            float rx = offsets[i].x * cosA - offsets[i].y * sinA;
            float ry = offsets[i].x * sinA + offsets[i].y * cosA;

            sprites[i].setPosition(center + sf::Vector2f(rx, ry));
            sprites[i].setRotation(sf::degrees(rotationDegrees));

            target.draw(sprites[i]);
        }
    }
}
