#include "Entities/IceBlock.hpp"
#include <SFML/Graphics/RectangleShape.hpp>

IceBlock::IceBlock(sf::Vector2f position)
    : Block(position) {
    m_breakable = false;
}

void IceBlock::onHitFromBelow(Player& player) {
    // Ice block hit from below
}

void IceBlock::update(float dt) {
    Block::update(dt);
}

void IceBlock::render(sf::RenderTarget& target) {
    sf::RectangleShape rect(sf::Vector2f(boundingBox.width, boundingBox.height));
    rect.setPosition(position);
    rect.setFillColor(sf::Color(150, 220, 255)); // Light blue color for ice
    rect.setOutlineColor(sf::Color::White);
    rect.setOutlineThickness(1.0f);
    target.draw(rect);
}
