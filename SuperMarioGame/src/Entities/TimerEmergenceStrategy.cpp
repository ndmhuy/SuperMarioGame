#include "Entities/TimerEmergenceStrategy.hpp"
#include "Entities/Enemy.hpp"

TimerEmergenceStrategy::TimerEmergenceStrategy(sf::Vector2f anchorPos)
    : m_timer(0.0f),
      m_anchorPos(anchorPos),
      m_anchorInitialized(anchorPos != sf::Vector2f(0.f, 0.f)),
      m_state(EmergenceState::Retracted) {}

sf::Vector2f TimerEmergenceStrategy::getAnchorPos() const {
    return m_anchorPos;
}

void TimerEmergenceStrategy::setAnchorPos(sf::Vector2f anchorPos) {
    m_anchorPos = anchorPos;
    m_anchorInitialized = true;
}

void TimerEmergenceStrategy::calculateTarget(Enemy& enemy, float dt) {
    if (!m_anchorInitialized) {
        m_anchorPos = enemy.position;
        m_anchorInitialized = true;
    }

    m_timer += dt;
    // Total cycle is 7 seconds:
    // 0s to 2s: Retracted
    // 2s to 3s: Emerging
    // 3s to 6s: Emerged
    // 6s to 7s: Retreating
    if (m_timer >= 7.0f) {
        m_timer = 0.0f;
    }

    if (m_timer < 2.0f) {
        m_state = EmergenceState::Retracted;
    } else if (m_timer < 3.0f) {
        m_state = EmergenceState::Emerging;
    } else if (m_timer < 6.0f) {
        m_state = EmergenceState::Emerged;
    } else {
        m_state = EmergenceState::Retreating;
    }
}

void TimerEmergenceStrategy::applyMovement(Enemy& enemy, float dt) {
    // Travel exactly the emerger's own height, not a hardcoded 2 tiles. A
    // PiranhaPlant is 48px tall (its target size), so a hardcoded 64px used to
    // push it 16px above the anchor (the pipe mouth) at full extension —
    // moving/reading this from the entity keeps the anchor-to-emerged offset
    // consistent with whatever occupies this strategy.
    const float emergeHeight = enemy.getTargetSize().y;

    switch (m_state) {
        case EmergenceState::Retracted:
            enemy.position = m_anchorPos;
            enemy.velocity = sf::Vector2f(0.0f, 0.0f);
            break;
        case EmergenceState::Emerging:
            enemy.velocity = sf::Vector2f(0.0f, -emergeHeight); // moves up over the 1s emerging window
            break;
        case EmergenceState::Emerged:
            enemy.position = m_anchorPos - sf::Vector2f(0.0f, emergeHeight);
            enemy.velocity = sf::Vector2f(0.0f, 0.0f);
            break;
        case EmergenceState::Retreating:
            enemy.velocity = sf::Vector2f(0.0f, emergeHeight); // moves down over the 1s retreating window
            break;
    }
}
