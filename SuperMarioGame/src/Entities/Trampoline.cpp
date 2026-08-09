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
    if (m_bounceTimer > 0.0f) {
        m_bounceTimer -= dt;
        if (m_bounceTimer <= 0.0f) {
            m_bounceTimer = 0.0f;
            m_bounceStage = 0;
            if (m_animator) {
                m_animator->play(&m_animation);
            }
        }
    }
    Item::update(dt);
}

void Trampoline::setupAnimations(const SpriteSheet* spriteSheet) {
    Item::setupAnimations(spriteSheet);
    m_animation = Animation("trampoline");
    m_animation.frameList = {{"trampoline", 0.15f}};

    m_bounceAnimation = Animation("trampoline_bounce");
    m_bounceAnimation.frameList = {
        {"trampoline_squished", 0.10f},
        {"trampoline_extended", 0.10f},
        {"trampoline", 0.10f}
    };
    m_bounceAnimation.isLooping = false;

    if (m_animator) {
        m_animator->play(&m_animation);
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

void Trampoline::activate(Player& player) {
    // Set upward player velocity (-831.4f)
    sf::Vector2f vel = player.getVelocity();
    vel.y = -831.4f;
    player.setVelocity(vel);

    // Start 0.3s bounce timer playing "trampoline_squished" -> "trampoline_extended" -> "trampoline"
    m_bounceTimer = 0.3f;
    m_bounceStage = 1;
    if (m_animator) {
        m_animator->play(&m_bounceAnimation);
    }
}

void Trampoline::collect() {
    // Trampoline is a reusable block, it is not consumed/collected.
}
