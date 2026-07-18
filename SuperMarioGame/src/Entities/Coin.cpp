#include "Entities/Coin.hpp"
#include "Entities/Player.hpp"
#include <SFML/Graphics/CircleShape.hpp>
#include <cmath>

Coin::Coin(sf::Vector2f pos) : Item(pos) {
    velocity = sf::Vector2f{0.0f, 0.0f};
}

void Coin::update(float dt) {
    // Coins are stationary
}

void Coin::render(sf::RenderTarget& target) {
    if (!active) return;
    
    // Draw gold coin circle
    sf::CircleShape coin(boundingBox.width / 2.0f);
    coin.setPosition(position);
    coin.setFillColor(sf::Color(255, 215, 0)); // Gold
    coin.setOutlineColor(sf::Color(218, 165, 32)); // Darker gold outline
    coin.setOutlineThickness(1.0f);
    target.draw(coin);
}

void Coin::activate(Player& player) {
    player.addCoins(1);
    player.addScore(200);
}
