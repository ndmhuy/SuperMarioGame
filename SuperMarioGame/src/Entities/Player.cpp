#include "Entities/Player.hpp"

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
    // TODO: Implement by hand
    if (m_currentState) {
        m_currentState->exit(*this);
    }
    m_currentState = std::move(state);
    if (m_currentState) {
        m_currentState->enter(*this);
    }
}
