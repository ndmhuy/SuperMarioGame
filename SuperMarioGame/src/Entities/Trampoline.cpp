#include "Entities/Trampoline.hpp"
#include "Entities/Player.hpp"
#include "Physics/CollisionDetector.hpp"
#include "Core/SoundManager.hpp"
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/CircleShape.hpp>
#include <cmath>

Trampoline::Trampoline(sf::Vector2f pos) : Item(pos) {
    velocity = sf::Vector2f{0.0f, 0.0f};
    setTargetSize({32.0f, 32.0f});
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
        // Pinned to the 16px source frame rather than aspect-fitted, so the
        // squash animation reads as compression instead of rescaling.
        drawSprite(target, m_animator->getSprite(), SpriteAnchor::BottomCenter,
                   /*flipX=*/false, /*flipY=*/false,
                   /*overrideScale=*/m_targetSize.x / 16.0f);
    } else {
        drawPlaceholder(target, sf::Color::Yellow);
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

ItemTouch Trampoline::onPlayerTouch(Player& player, const CollisionInfo& info) {
    // A trampoline only launches a player who comes down onto it. Reached from
    // the side or from underneath it is just a solid box, which is what stops
    // it from flinging anyone who brushes past it while rising.
    if (info.normal.y == -1.0f || player.getVelocity().y >= 0.0f) {
        activate(player);
        return ItemTouch::Consumed;   // activate() already set the launch velocity
    }
    return ItemTouch::Solid;
}

void Trampoline::collect() {
    // Trampoline is a reusable block, it is not consumed/collected.
}
