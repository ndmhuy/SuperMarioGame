#include "Entities/Projectile.hpp"

Projectile::Projectile(sf::Vector2f pos, sf::Vector2f targetSize)
    : Entity(pos, targetSize) {
    position = pos;
    boundingBox.x = pos.x;
    boundingBox.y = pos.y;
}
