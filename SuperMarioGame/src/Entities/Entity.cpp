#include "Entities/Entity.hpp"
#include <SFML/Graphics/RectangleShape.hpp>
#include <algorithm>

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

void Entity::drawSprite(sf::RenderTarget& target,
                        sf::Sprite sprite,
                        SpriteAnchor anchor,
                        bool flipX,
                        bool flipY,
                        float overrideScale) const {
    const sf::FloatRect bounds = sprite.getLocalBounds();
    if (bounds.size.x <= 0.0f || bounds.size.y <= 0.0f) return;

    const float scale = (overrideScale > 0.0f)
        ? overrideScale
        : std::min(m_targetSize.x / bounds.size.x, m_targetSize.y / bounds.size.y);

    if (anchor == SpriteAnchor::TopLeft) {
        sprite.setOrigin(sf::Vector2f(0.0f, 0.0f));
        sprite.setScale(sf::Vector2f(scale, scale));
        sprite.setPosition(sf::Vector2f(boundingBox.x, boundingBox.y));
    } else {
        if (flipY) {
            // Origin at center so vertical flip pivots cleanly about the horizontal center line
            sprite.setOrigin(sf::Vector2f(bounds.size.x * 0.5f, bounds.size.y * 0.5f));
            sprite.setScale(sf::Vector2f(flipX ? -scale : scale, -scale));
            sprite.setPosition(sf::Vector2f(boundingBox.x + m_targetSize.x * 0.5f,
                                            boundingBox.y + m_targetSize.y * 0.5f));
        } else {
            // Origin at the sprite's bottom-centre so entities of differing sprite
            // height still stand on the same ground line.
            sprite.setOrigin(sf::Vector2f(bounds.size.x * 0.5f, bounds.size.y));
            sprite.setScale(sf::Vector2f(flipX ? -scale : scale, scale));
            sprite.setPosition(sf::Vector2f(boundingBox.x + m_targetSize.x * 0.5f,
                                            boundingBox.y + m_targetSize.y));
        }
    }

    target.draw(sprite);
}

void Entity::drawPlaceholder(sf::RenderTarget& target, sf::Color fill) const {
    sf::RectangleShape rect(sf::Vector2f(boundingBox.width, boundingBox.height));
    rect.setPosition(sf::Vector2f(boundingBox.x, boundingBox.y));
    rect.setFillColor(fill);
    rect.setOutlineColor(sf::Color::White);
    rect.setOutlineThickness(1.0f);
    target.draw(rect);
}
