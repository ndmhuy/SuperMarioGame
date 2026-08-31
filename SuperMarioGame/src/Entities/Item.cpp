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

ItemTouch Item::onPlayerTouch(Player& player, const CollisionInfo& info) {
    // The ordinary powerup: walking into it picks it up, from any direction.
    (void)player;
    (void)info;
    return ItemTouch::Collect;
}

void Item::setupAnimations(const SpriteSheet* spriteSheet) {
    if (!spriteSheet) return;
    m_spriteSheet = spriteSheet;
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
        const sf::Sprite sprite = m_animator->getSprite();
        // Cache the scale from the first frame so frames of differing size do not
        // make the item pulse as it animates.
        if (m_baseScale <= 0.0f) {
            const sf::FloatRect bounds = sprite.getLocalBounds();
            if (bounds.size.x > 0.0f && bounds.size.y > 0.0f) {
                m_baseScale = std::min(m_targetSize.x / bounds.size.x,
                                       m_targetSize.y / bounds.size.y);
            }
        }
        drawSprite(target, sprite, SpriteAnchor::BottomCenter,
                   /*flipX=*/false, /*flipY=*/false, /*overrideScale=*/m_baseScale);
    } else {
        drawPlaceholder(target, sf::Color::Yellow);
    }
}

