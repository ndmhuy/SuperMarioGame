#include "Entities/Boss.hpp"
#include "Core/EventBus.hpp"
#include "Core/Game.hpp"
#include "Core/SoundManager.hpp"
#include "Entities/Player.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <utility>

namespace {
// Seconds of immunity after a hit lands. Long enough that one stomp is one hit
// even though the overlap persists for several frames.
constexpr float HIT_INVULNERABILITY = 1.0f;
// How long the defeat sequence plays before the boss is removed.
constexpr float DEFEAT_DURATION = 2.0f;
}

namespace {
// A boss's health bar is where difficulty is felt most, so it is scaled here
// rather than at every call site. Always at least one hit.
int scaledHealth(int maxHealth) {
    const float scale = Game::getInstance().difficulty().bossHealthScale();
    return std::max(1, static_cast<int>(std::lround(static_cast<float>(maxHealth) * scale)));
}
}

Boss::Boss(sf::Vector2f position, std::string displayName, int maxHealth,
           int scoreValue, sf::Vector2f size)
    : Enemy(position, scoreValue, size),
      m_displayName(std::move(displayName)),
      m_maxHealth(scaledHealth(std::max(1, maxHealth))),
      m_health(m_maxHealth) {
    m_phase = phaseForHealth(m_health);
}

int Boss::phaseForHealth(int health) const {
    // Two phases, switching at half health — the split the SPEC gives Bowser.
    return (health * 2 <= m_maxHealth) ? 2 : 1;
}

bool Boss::takeHit(int amount) {
    if (isDefeated() || isInvulnerable() || amount <= 0) return false;

    m_health = std::max(0, m_health - amount);
    m_invulnerableTimer = HIT_INVULNERABILITY;
    SoundManager::getInstance().playSound("bump");
    onTookHit();

    const int newPhase = phaseForHealth(m_health);
    if (newPhase != m_phase) {
        m_phase = newPhase;
        std::cout << "[Boss] " << m_displayName << " entering phase " << m_phase << std::endl;
        onPhaseChanged(m_phase);
    }

    if (isDefeated()) {
        std::cout << "[Boss] " << m_displayName << " defeated." << std::endl;
        m_defeatTimer = DEFEAT_DURATION;
        // Stop dead: the defeat sequence should not carry momentum.
        velocity = {0.0f, 0.0f};
        onDefeated();
    }
    return true;
}

void Boss::onStomped() {
    takeHit();
}

void Boss::onHitByFireball() {
    // Fire does the same damage as a stomp. Overriding this is what makes a
    // boss immune to fire; Bowser and Boom Boom both take it.
    takeHit();
}

void Boss::stagger(float seconds) {
    if (isDefeated() || seconds <= 0.0f) return;

    const bool wasStaggered = isStaggered();
    m_staggerTimer = std::max(m_staggerTimer, seconds);
    // The i-frames go with it. A stagger that still leaves the boss invulnerable
    // is an opening the player can see and cannot use, which is worse than no
    // opening at all — CollisionResolver reads isInvulnerable() to decide
    // whether a stomp lands.
    m_invulnerableTimer = 0.0f;
    // Nothing carries momentum through a stagger; it is a stop, not a slow.
    velocity.x = 0.0f;

    if (!wasStaggered) {
        std::cout << "[Boss] " << m_displayName << " staggered for " << seconds
                  << "s." << std::endl;
        onStaggerBegan();
    }
}

void Boss::defeatNow() {
    if (isDefeated()) return;
    // Both guards cleared first: takeHit() refuses while the i-frames run, and a
    // boss that happened to be mid-stagger would otherwise survive the bridge.
    m_invulnerableTimer = 0.0f;
    m_staggerTimer = 0.0f;
    takeHit(m_health);
}

void Boss::endStagger() {
    if (!isStaggered()) return;
    m_staggerTimer = 0.0f;
    onStaggerEnded();
}

void Boss::update(float dt) {
    if (!active) return;

    if (m_invulnerableTimer > 0.0f) {
        m_invulnerableTimer = std::max(0.0f, m_invulnerableTimer - dt);
    }

    // Ticked before the defeat check, so a boss killed during its own stagger
    // still runs its defeat sequence rather than staying frozen.
    if (m_staggerTimer > 0.0f) {
        m_staggerTimer = std::max(0.0f, m_staggerTimer - dt);
        if (m_staggerTimer <= 0.0f) onStaggerEnded();
    }

    if (isDefeated()) {
        m_defeatTimer -= dt;

        // Score and the BossDefeated event fire once, at the start of the
        // sequence, so the celebration music and the achievement do not wait
        // for the animation.
        if (!m_defeatAnnounced) {
            m_defeatAnnounced = true;
            if (Player* player = Game::getInstance().getNearestPlayer(getPosition())) {
                player->addScore(getScoreValue());
            }
            EventBus::getInstance().publish({EventType::BossDefeated, getScoreValue()});
        }

        if (m_animator && m_hasAnimation) {
            m_animator->update(dt);
        }
        if (m_defeatTimer <= 0.0f) {
            destroy();
        }
        return;
    }

    // A staggered boss does not act. This is the whole point of the window: it
    // stops walking into the player and stops attacking, so the opening is a
    // real one rather than a moment the player has to survive as well as use.
    if (!isStaggered()) {
        updateBehaviour(dt);
    } else {
        velocity.x = 0.0f;
    }

    if (m_animator && m_hasAnimation) {
        m_animator->update(dt);
    }
}
