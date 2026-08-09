#include "Entities/Item.hpp"
#include "Entities/Player.hpp"
#include <SFML/Graphics/RectangleShape.hpp>
#include <algorithm>

Item::Item(sf::Vector2f pos, sf::Vector2f targetSize) : Entity(pos, targetSize) {
    active = true;
    collected = false;
}

void Item::activate(Player& player) {
    // Overridden by subclasses
}

void Item::collect() {
    collected = true;
    destroy();
}

void Item::setupAnimations(const SpriteSheet* spriteSheet) {
    if (!spriteSheet) return;
    m_animator = std::make_unique<Animator>(spriteSheet);
}

void Item::update(float dt) {
    if (m_animator && m_hasAnimation) {
        m_animator->update(dt);
    }
}

void Item::render(sf::RenderTarget& target) {
    if (!active || collected) return;
    if (m_animator && m_hasAnimation) {
        sf::Sprite sprite = m_animator->getSprite();
        sf::FloatRect bounds = sprite.getLocalBounds();
        if (bounds.size.x > 0.0f && bounds.size.y > 0.0f) {
            if (m_baseScale <= 0.0f) {
                m_baseScale = std::min(m_targetSize.x / bounds.size.x, m_targetSize.y / bounds.size.y);
            }
            float scale = m_baseScale;

            // Base AABB remains locked to m_targetSize during all animation frames
            boundingBox.width = m_targetSize.x;
            boundingBox.height = m_targetSize.y;

            sprite.setOrigin(sf::Vector2f(bounds.size.x * 0.5f, bounds.size.y));
            sprite.setScale(sf::Vector2f(scale, scale));
            sprite.setPosition(sf::Vector2f(boundingBox.x + m_targetSize.x * 0.5f, boundingBox.y + m_targetSize.y));

            target.draw(sprite);
        }
    } else {
        sf::RectangleShape rect(sf::Vector2f(boundingBox.width, boundingBox.height));
        rect.setPosition(sf::Vector2f(boundingBox.x, boundingBox.y));
        rect.setFillColor(sf::Color::Yellow);
        rect.setOutlineColor(sf::Color::White);
        rect.setOutlineThickness(1.0f);
        target.draw(rect);
    }
}

