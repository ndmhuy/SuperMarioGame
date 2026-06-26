#include "Entities/Player.hpp"
#include "Core/EventBus.hpp"
#include "Utils/Constants.hpp"

void Player::run() {
    // TODO: Implement by hand
}

void Player::wallJump() {
    // TODO: Implement by hand
}

void Player::groundPound() {
    // TODO: Implement by hand
}

void Player::crouch() {
    // TODO: Implement by hand
}

void Player::slide() {
    // TODO: Implement by hand
}

void Player::shootFireball() {
    // TODO: Implement by hand
}

void Player::powerUp(int itemType) {
    // TODO: Implement by hand
}

void Player::powerDown() {
    // TODO: Implement by hand
}

IPlayerState* Player::getCurrentState() const {
    // TODO: Implement by hand
    return m_currentState.get();
}

void Player::changeState(std::unique_ptr<IPlayerState> state) {
    if (m_currentState) {
        m_currentState->exit(*this);
    }
    m_currentState = std::move(state);
    if (m_currentState) {
        m_currentState->enter(*this);
        sf::Vector2f size = m_currentState->getSize();
        boundingBox.width = size.x;
        boundingBox.height = size.y;
    }
}

void Player::addCoins(int amount) {
    coins += amount;
    while (coins >= Constants::COINS_FOR_LIFE) {
        coins -= Constants::COINS_FOR_LIFE;
        gainLife();
    }
    EventBus::getInstance().publish({EventType::CoinCollected, amount});
}

void Player::addScore(int amount) {
    score += amount;
}

void Player::gainLife() {
    ++lives;
}

void Player::loseLife() {
    if (lives > 0) {
        --lives;
    }
    if (lives <= 0) {
        EventBus::getInstance().publish({EventType::GameOver, 0});
    }
}

void Player::resetCombo() {
    comboCounter = 0;
}

void Player::incrementCombo() {
    ++comboCounter;
    EventBus::getInstance().publish({EventType::ComboHit, comboCounter});
}

