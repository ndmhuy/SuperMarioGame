#include "Entities/Trampoline.hpp"
#include "Entities/Player.hpp"
#include "Core/SoundManager.hpp"
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/CircleShape.hpp>
#include <cmath>

Trampoline::Trampoline(sf::Vector2f pos) : Item(pos) {
    velocity = sf::Vector2f{0.0f, 0.0f};
    boundingBox.width = 32.0f;
    boundingBox.height = 32.0f;
}

void Trampoline::update(float dt) {
    if (m_isBouncing) {
        m_bounceTimer -= dt;
        float elapsed = 0.3f - m_bounceTimer;
        if (elapsed < 0.1f) {
            if (m_animator) {
                m_animator->play(&m_squishAnim);
            }
        } else if (elapsed < 0.25f) {
            if (m_animator) {
                m_animator->play(&m_extendAnim);
            }
        } else {
            m_isBouncing = false;
            m_bounceTimer = 0.0f;
            if (m_animator) {
                m_animator->play(&m_idleAnim);
            }
        }
    }
    Item::update(dt);
}

void Trampoline::setupAnimations(const SpriteSheet* spriteSheet) {
    Item::setupAnimations(spriteSheet);
    m_idleAnim = Animation("trampoline");
    m_idleAnim.frameList = {{"trampoline", 0.15f}};

    m_squishAnim = Animation("trampoline_squished");
    m_squishAnim.frameList = {{"trampoline_squished", 0.10f}};

    m_extendAnim = Animation("trampoline_extended");
    m_extendAnim.frameList = {{"trampoline_extended", 0.15f}};

    if (m_animator) {
        m_animator->play(&m_idleAnim);
        m_hasAnimation = true;
    }
}

void Trampoline::render(sf::RenderTarget& target) {
    if (!active || collected) return;
    if (m_animator && m_hasAnimation) {
        sf::Sprite sprite = m_animator->getSprite();
        sf::FloatRect bounds = sprite.getLocalBounds();
        if (bounds.size.x > 0.0f && bounds.size.y > 0.0f) {
            // Base boundingBox remains fixed at base size (32x32)
            boundingBox.width = m_targetSize.x;
            boundingBox.height = m_targetSize.y;

            float scale = m_targetSize.x / 16.0f;

            // Sprite origin set to bottom-center and positioned at bottom-center of AABB
            sprite.setOrigin(sf::Vector2f(bounds.size.x * 0.5f, bounds.size.y));
            sprite.setScale(sf::Vector2f(scale, scale));
            sprite.setPosition(sf::Vector2f(boundingBox.x + 16.0f, boundingBox.y + 32.0f));

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

#include "Core/SoundManager.hpp"

void Trampoline::activate(Player& player) {
    m_isBouncing = true;
    m_bounceTimer = 0.3f;
    if (m_animator) {
        m_animator->play(&m_squishAnim);
    }

    sf::Vector2f vel = player.getVelocity();
    vel.y = -831.4f;
    player.setVelocity(vel);

    SoundManager::getInstance().playSound("boing");
}

void Trampoline::collect() {
    // Trampoline is a reusable block, it is not consumed/collected.
}
