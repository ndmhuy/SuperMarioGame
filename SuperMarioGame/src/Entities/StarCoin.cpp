#include "Entities/StarCoin.hpp"
#include "Entities/Player.hpp"
#include "Core/EventBus.hpp"
#include <SFML/Graphics/CircleShape.hpp>
#include <cmath>

StarCoin::StarCoin(sf::Vector2f pos) : Item(pos) {
    velocity = sf::Vector2f{0.0f, 0.0f};
}

void StarCoin::update(float dt) {
    // StarCoins are stationary
}

void StarCoin::render(sf::RenderTarget& target) {
    if (!active) return;
    
    // Draw a larger gold coin representing a StarCoin
    sf::CircleShape coin(boundingBox.width / 2.0f);
    coin.setPosition(position);
    coin.setFillColor(sf::Color(255, 165, 0)); // Orange-Gold
    coin.setOutlineColor(sf::Color::White);
    coin.setOutlineThickness(2.0f);
    target.draw(coin);
}

void StarCoin::activate(Player& player) {
    player.addScore(1000);
    EventBus::getInstance().publish({EventType::StarCoinCollected, this});
}
