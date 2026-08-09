#include "Entities/FallingPlatform.hpp"
#include "Entities/Player.hpp"
#include "Core/Game.hpp"
#include "Utils/Constants.hpp"
#include <SFML/Graphics/RectangleShape.hpp>
#include <cmath>

FallingPlatform::FallingPlatform(sf::Vector2f position)
    : Block(position, {64.0f, 16.0f}), m_state(FallingPlatformState::Idle) {
    m_breakable = false;
}

void FallingPlatform::onHitFromBelow(Player& player) {
    // Falling platform hit from below
}

bool FallingPlatform::isPlayerStandingOnTop() const {
    Player* player = Game::getInstance().getPlayer();
    if (!player) return false;

    AABB pBox = player->getBoundingBox();
    AABB platBox = getBoundingBox();

    // If platform is respawning, its bounding box is empty and player cannot stand on it
    if (m_state == FallingPlatformState::Respawning) return false;

    bool xOverlap = (pBox.x + pBox.width > platBox.x) && (pBox.x < platBox.x + platBox.width);
    bool yOverlap = std::abs((pBox.y + pBox.height) - platBox.y) < 3.0f;
    bool resting = player->getVelocity().y >= 0.0f;

    return xOverlap && yOverlap && resting;
}

void FallingPlatform::update(float dt) {
    Player* player = Game::getInstance().getPlayer();
    
    switch (m_state) {
        case FallingPlatformState::Idle: {
            if (isPlayerStandingOnTop()) {
                m_state = FallingPlatformState::Shaking;
                m_shakeTimer = Constants::FALLING_PLATFORM_SHAKE_TIME;
                m_shakeOffset = sf::Vector2f(0.0f, 0.0f);
            }
            break;
        }
        case FallingPlatformState::Shaking: {
            m_shakeTimer -= dt;
            // Generate horizontal visual shaking offset
            m_shakeOffset.x = std::sin(m_shakeTimer * 50.0f) * 2.0f;
            
            if (m_shakeTimer <= 0.0f) {
                m_state = FallingPlatformState::Falling;
                m_shakeOffset = sf::Vector2f(0.0f, 0.0f);
                velocity.y = 0.0f;
            }
            break;
        }
        case FallingPlatformState::Falling: {
            // Apply gravity (1800 px/s^2)
            velocity.y += 1800.0f * dt;
            float dy = velocity.y * dt;
            
            // If player is standing on the platform, drag player down
            if (isPlayerStandingOnTop() && player) {
                player->setPosition(player->getPosition() + sf::Vector2f(0.0f, dy));
            }
            
            position.y += dy;
            boundingBox.y = position.y;
            
            // Transition to respawn after falling 400px
            if (position.y > m_originalPosition.y + 400.0f) {
                m_state = FallingPlatformState::Respawning;
                m_respawnTimer = Constants::FALLING_PLATFORM_RESPAWN_TIME;
            }
            break;
        }
        case FallingPlatformState::Respawning: {
            m_respawnTimer -= dt;
            if (m_respawnTimer <= 0.0f) {
                m_state = FallingPlatformState::Idle;
                setPosition(m_originalPosition);
                velocity = sf::Vector2f(0.0f, 0.0f);
            }
            break;
        }
    }
}

void FallingPlatform::setupAnimations(const SpriteSheet* spriteSheet) {
    Block::setupAnimations(spriteSheet);
    m_animation = Animation("falling_platform");
    m_animation.frameList = {{"falling_platform_medium", 0.15f}};
    if (m_animator) {
        m_animator->play(&m_animation);
        m_hasAnimation = true;
    }
}

void FallingPlatform::render(sf::RenderTarget& target) {
    if (m_state == FallingPlatformState::Respawning) return;
    Block::render(target);
}

const AABB& FallingPlatform::getBoundingBox() const {
    static const AABB emptyBox{0.f, 0.f, 0.f, 0.f};
    if (m_state == FallingPlatformState::Respawning) {
        return emptyBox;
    }
    return boundingBox;
}
