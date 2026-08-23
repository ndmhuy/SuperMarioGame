#include "Entities/Bowser.hpp"
#include "Entities/EntityFactory.hpp"
#include "Core/EventBus.hpp"
#include "Core/Game.hpp"
#include "Core/GameSnapshot.hpp"
#include "Core/SoundManager.hpp"
#include "Entities/Player.hpp"
#include "Utils/Constants.hpp"

#include <algorithm>
#include <cmath>

namespace {
constexpr int   BOWSER_HEALTH = 5;
constexpr int   BOWSER_SCORE  = 5000;   // SPEC 6.x
constexpr float BOWSER_WIDTH  = 64.0f;
constexpr float BOWSER_HEIGHT = 64.0f;

// Fallback pacing width when a level gives Bowser no arena.
constexpr float DEFAULT_PATROL_HALF_WIDTH = 5.0f * Constants::TILE_SIZE;

constexpr float JUMP_IMPULSE = -520.0f;
constexpr float JUMP_INTERVAL = 2.6f;    // phase 2 only

// How long the opening lasts once the fireballs have landed. Long enough to
// cross the arena and jump — under two seconds and the player is being asked to
// already be in position, which they cannot plan for.
constexpr float STAGGER_SECONDS = 3.0f;
}

Bowser::Bowser(sf::Vector2f position)
    : Boss(position, "BOWSER", BOWSER_HEALTH, BOWSER_SCORE, {BOWSER_WIDTH, BOWSER_HEIGHT}) {
    speed = Constants::BOSS_BOWSER_SPEED;
    facingRight = false;   // faces back down the level, towards the player
    m_fireTimer = fireInterval();
}

float Bowser::fireInterval() const {
    // "faster fire" in phase 2.
    return (getPhase() >= 2) ? 1.2f : 2.2f;
}

float Bowser::walkSpeed() const {
    // Phase 2 is "faster"; the base comes from Character::speed so the
    // difficulty modifier reaches it like every other enemy.
    return (getPhase() >= 2) ? speed * 1.6f : speed;
}

void Bowser::setupAnimations(const SpriteSheet* spriteSheet) {
    Enemy::setupAnimations(spriteSheet);

    m_walkLeftAnim = Animation("bowser_walk_left");
    m_walkLeftAnim.frameList = {{"bowser_move_left_0", 0.16f}, {"bowser_move_left_1", 0.16f},
                                {"bowser_move_left_2", 0.16f}, {"bowser_move_left_3", 0.16f}};
    m_walkRightAnim = Animation("bowser_walk_right");
    m_walkRightAnim.frameList = {{"bowser_move_right_0", 0.16f}, {"bowser_move_right_1", 0.16f},
                                 {"bowser_move_right_2", 0.16f}, {"bowser_move_right_3", 0.16f}};

    m_animation = m_walkLeftAnim;
    if (m_animator && spriteSheet && spriteSheet->hasFrame("bowser_move_left_0")) {
        m_animator->play(&m_animation);
        m_hasAnimation = true;
    }
}

void Bowser::breatheFire() {
    // Fire leaves the mouth, roughly two thirds up the sprite and at the facing
    // edge, so it does not spawn inside his own bounding box.
    const float muzzleX = facingRight ? position.x + BOWSER_WIDTH : position.x - 24.0f;
    const sf::Vector2f muzzle{muzzleX, position.y + BOWSER_HEIGHT * 0.35f};
    const float speed = (getPhase() >= 2) ? 320.0f : 240.0f;

    EntitySpawnRequest request;
    request.type = static_cast<int>(EntityType::BossFireball);
    request.position = muzzle;
    request.velocity = sf::Vector2f(facingRight ? speed : -speed, 0.0f);
    EventBus::getInstance().publish({EventType::EntitySpawnRequested, request});

    SoundManager::getInstance().playSound("fireball");
}

void Bowser::leap() {
    if (!onGround) return;
    velocity.y = JUMP_IMPULSE;
    onGround = false;
    EventBus::getInstance().publish({EventType::ScreenShakeTriggered, 0});
}

void Bowser::onPhaseChanged(int newPhase) {
    // Re-arm both clocks at the new rate so the escalation is immediate rather
    // than waiting out the phase-1 interval that was already running.
    m_fireTimer = fireInterval() * 0.5f;
    m_jumpTimer = JUMP_INTERVAL * 0.5f;
    SoundManager::getInstance().playSound("bowserfall");
}

void Bowser::onTookHit() {
    // Knock him back a little, so a stomp reads as landing.
    velocity.x = facingRight ? -80.0f : 80.0f;
    // A landed hit ends the opening it was taken through. Otherwise one stagger
    // pays for the rest of the health bar: the window is three seconds, his
    // i-frames are gone while it runs, and a player who can bounce twice a
    // second would finish the fight from a single flower.
    m_fireHits = 0;
    // The opening closes with the hit it paid for. Four more fireballs buy the
    // next one, so every point of the health bar costs the same.
    endStagger();
}

int Bowser::getFireHitsToStagger() const {
    return std::max(0, FIRE_HITS_PER_STAGGER - m_fireHits);
}

void Bowser::onHitByFireball() {
    // No health is lost — he breathes the stuff. What it costs him is his
    // guard: four hits and he reels, and while he reels he can be stomped.
    if (isStaggered()) return;   // already open; further fire is wasted

    ++m_fireHits;
    SoundManager::getInstance().playSound("bump");

    if (m_fireHits >= FIRE_HITS_PER_STAGGER) {
        m_fireHits = 0;
        stagger(STAGGER_SECONDS);
    }
}

void Bowser::onStaggerBegan() {
    // Stops mid-stride and drops the fire. Re-arming both clocks means he does
    // not breathe the instant he recovers, which would burn the player as they
    // land the stomp the window was for.
    velocity.x = 0.0f;
    m_fireTimer = fireInterval();
    m_jumpTimer = JUMP_INTERVAL;
    SoundManager::getInstance().playSound("bowserfall");
}

void Bowser::onStaggerEnded() {
    // Back on his feet with a full clock, for the same reason.
    m_fireTimer = fireInterval();
    m_jumpTimer = JUMP_INTERVAL;
}

void Bowser::updateBehaviour(float dt) {
    if (!m_patrolInitialised) {
        // Pace the width of the arena when one was assigned; otherwise a fixed
        // span around wherever the level put him.
        if (hasArena()) {
            const AABB arena = getArena();
            m_patrolLeft  = arena.x + Constants::TILE_SIZE;
            m_patrolRight = arena.x + arena.width - Constants::TILE_SIZE - BOWSER_WIDTH;
        } else {
            m_patrolLeft  = position.x - DEFAULT_PATROL_HALF_WIDTH;
            m_patrolRight = position.x + DEFAULT_PATROL_HALF_WIDTH;
        }
        if (m_patrolRight < m_patrolLeft) {
            std::swap(m_patrolLeft, m_patrolRight);
        }
        m_patrolInitialised = true;
    }

    // --- Walk ---
    velocity.x = facingRight ? walkSpeed() : -walkSpeed();
    if (position.x <= m_patrolLeft) {
        facingRight = true;
    } else if (position.x >= m_patrolRight) {
        facingRight = false;
    }

    // Face the player when they get behind him, so he is never harmlessly
    // breathing fire at a wall.
    if (const Player* player = Game::getInstance().getNearestPlayer(getPosition())) {
        const float toPlayer = player->getPosition().x - position.x;
        if (std::abs(toPlayer) > BOWSER_WIDTH) {
            facingRight = toPlayer > 0.0f;
        }
    }

    if (m_animator && m_hasAnimation) {
        // Directional frames, not a mirrored sprite: the atlas ships both.
        const Animation& wanted = facingRight ? m_walkRightAnim : m_walkLeftAnim;
        if (m_animation.name != wanted.name) {
            m_animation = wanted;
            m_animator->play(&m_animation);
        }
    }

    // --- Fire breath ---
    m_fireTimer -= dt;
    if (m_fireTimer <= 0.0f) {
        m_fireTimer = fireInterval();
        breatheFire();
    }

    // --- Jump, phase 2 only ---
    if (getPhase() >= 2) {
        m_jumpTimer -= dt;
        if (m_jumpTimer <= 0.0f) {
            m_jumpTimer = JUMP_INTERVAL;
            leap();
        }
    }
}
