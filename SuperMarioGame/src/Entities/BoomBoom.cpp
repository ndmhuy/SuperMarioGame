#include "Entities/BoomBoom.hpp"
#include "Core/EventBus.hpp"
#include "Core/Game.hpp"
#include "Core/SoundManager.hpp"
#include "Entities/Player.hpp"
#include "Utils/Constants.hpp"

#include <algorithm>
#include <cmath>

namespace {
constexpr int   BOOMBOOM_HEALTH = 3;      // SPEC 6.4: three stomps
constexpr int   BOOMBOOM_SCORE  = 2000;
constexpr float BOOMBOOM_WIDTH  = 48.0f;
constexpr float BOOMBOOM_HEIGHT = 56.0f;

constexpr float CHARGE_DURATION = 1.6f;
constexpr float SPIN_DURATION   = 2.4f;
constexpr int   SPIN_BOUNCE_LIMIT = 4;

// Pacing width when a level gives him no arena.
constexpr float DEFAULT_HALF_WIDTH = 5.0f * Constants::TILE_SIZE;
}

BoomBoom::BoomBoom(sf::Vector2f position)
    : Boss(position, "BOOM BOOM", BOOMBOOM_HEALTH, BOOMBOOM_SCORE,
           {BOOMBOOM_WIDTH, BOOMBOOM_HEIGHT}) {
    enter(Action::Charging);
}

int BoomBoom::phaseForHealth(int health) const {
    // One phase per hit landed: 3 health -> 1, 2 -> 2, 1 -> 3. Boss's default
    // is a single switch at half health, which cannot express three steps.
    return std::clamp(getMaxHealth() - health + 1, 1, getMaxHealth());
}

float BoomBoom::chargeSpeed() const {
    switch (getPhase()) {
        case 1:  return 120.0f;
        case 2:  return 170.0f;
        default: return 230.0f;   // "fastest charge"
    }
}

float BoomBoom::recoverDuration() const {
    switch (getPhase()) {
        case 1:  return 1.6f;
        case 2:  return 1.0f;     // "shorter recovery"
        default: return 0.7f;
    }
}

void BoomBoom::enter(Action action) {
    m_action = action;
    switch (action) {
        case Action::Charging:
            m_actionTimer = CHARGE_DURATION;
            break;
        case Action::Recovering:
            m_actionTimer = recoverDuration();
            velocity.x = 0.0f;
            break;
        case Action::Spinning:
            m_actionTimer = SPIN_DURATION;
            m_spinBounces = 0;
            SoundManager::getInstance().playSound("kick");
            break;
    }
}

void BoomBoom::setupAnimations(const SpriteSheet* spriteSheet) {
    Enemy::setupAnimations(spriteSheet);

    m_walkAnim = Animation("boom_boom_walk");
    m_walkAnim.frameList = {{"boom_boom_walk_0", 0.14f}, {"boom_boom_walk_1", 0.14f}};
    m_spinAnim = Animation("boom_boom_spin");
    m_spinAnim.frameList = {{"boom_boom_spin_0", 0.07f}, {"boom_boom_spin_1", 0.07f}};

    m_animation = m_walkAnim;
    if (m_animator && spriteSheet && spriteSheet->hasFrame("boom_boom_walk_0")) {
        m_animator->play(&m_animation);
        m_hasAnimation = true;
    }
}

void BoomBoom::onPhaseChanged(int newPhase) {
    // Each hit knocks him straight into a recovery, so the player gets a beat to
    // read the new speed before he comes back.
    enter(Action::Recovering);
    SoundManager::getInstance().playSound("bowserfall");
}

void BoomBoom::onTookHit() {
    velocity.x = 0.0f;
    EventBus::getInstance().publish({EventType::ScreenShakeTriggered, 0});
}

void BoomBoom::updateBehaviour(float dt) {
    if (!m_arenaResolved) {
        if (hasArena()) {
            const AABB arena = getArena();
            m_arenaLeft  = arena.x + Constants::TILE_SIZE;
            m_arenaRight = arena.x + arena.width - Constants::TILE_SIZE - BOOMBOOM_WIDTH;
        } else {
            m_arenaLeft  = position.x - DEFAULT_HALF_WIDTH;
            m_arenaRight = position.x + DEFAULT_HALF_WIDTH;
        }
        if (m_arenaRight < m_arenaLeft) std::swap(m_arenaLeft, m_arenaRight);
        m_arenaResolved = true;
    }

    m_actionTimer -= dt;

    switch (m_action) {
        case Action::Charging: {
            // Aim at the player and commit; a charge that steers is not a charge.
            if (const Player* player = Game::getInstance().getPlayer()) {
                const float toPlayer = player->getPosition().x - position.x;
                if (std::abs(toPlayer) > 4.0f) {
                    facingRight = toPlayer > 0.0f;
                }
            }
            velocity.x = facingRight ? chargeSpeed() : -chargeSpeed();

            const bool hitWall = (position.x <= m_arenaLeft && !facingRight)
                              || (position.x >= m_arenaRight && facingRight);
            if (hitWall || m_actionTimer <= 0.0f) {
                // Phase 3 answers a finished charge with the spin attack.
                enter(getPhase() >= 3 ? Action::Spinning : Action::Recovering);
            }
            break;
        }

        case Action::Spinning: {
            velocity.x = facingRight ? chargeSpeed() * 1.3f : -chargeSpeed() * 1.3f;
            if (position.x <= m_arenaLeft) {
                facingRight = true;
                ++m_spinBounces;
            } else if (position.x >= m_arenaRight) {
                facingRight = false;
                ++m_spinBounces;
            }
            if (m_actionTimer <= 0.0f || m_spinBounces >= SPIN_BOUNCE_LIMIT) {
                enter(Action::Recovering);
            }
            break;
        }

        case Action::Recovering: {
            velocity.x = 0.0f;
            if (m_actionTimer <= 0.0f) {
                enter(Action::Charging);
            }
            break;
        }
    }

    // Keep him in the room he is fought in, whatever the state machine wants.
    position.x = std::clamp(position.x, m_arenaLeft, m_arenaRight);
    boundingBox.x = position.x;

    if (m_animator && m_hasAnimation) {
        const Animation& wanted = (m_action == Action::Spinning) ? m_spinAnim : m_walkAnim;
        if (m_animation.name != wanted.name) {
            m_animation = wanted;
            m_animator->play(&m_animation);
        }
    }
}
