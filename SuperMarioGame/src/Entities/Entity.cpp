#include "Entities/Entity.hpp"

std::uint32_t Entity::s_nextId = 1;

Entity::Entity(sf::Vector2f pos, sf::Vector2f targetSize) : m_id(s_nextId++) {
    position = pos;
    m_targetSize = targetSize;
    boundingBox = AABB{pos.x, pos.y, targetSize.x, targetSize.y};
}

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
