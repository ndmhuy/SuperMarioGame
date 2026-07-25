#include "Entities/Entity.hpp"

const AABB& Entity::getBoundingBox() const {
    return boundingBox;
}

bool Entity::isActive() const {
    return active;
}

void Entity::destroy() {
    active = false;
}

sf::Vector2f Entity::getPosition() const {
    return position;
}

sf::Vector2f Entity::getVelocity() const {
    return velocity;
}

void Entity::setPosition(sf::Vector2f pos) {
    position = pos;
    boundingBox.x = pos.x;
    boundingBox.y = pos.y;
}

void Entity::setVelocity(sf::Vector2f vel) {
    velocity = vel;
}
