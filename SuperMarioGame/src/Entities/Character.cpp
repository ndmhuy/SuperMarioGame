#include "Entities/Character.hpp"
#include "Utils/Constants.hpp"

void Character::moveLeft() {
    m_moveLeftRequested = true;
    this->facingRight = false;
}

void Character::moveRight() {
    m_moveRightRequested = true;
    this->facingRight = true;
}

void Character::clearMovementRequests() {
    m_moveLeftRequested = false;
    m_moveRightRequested = false;
}

void Character::jump() {
    if (onGround || onWall) {
        this->velocity.y = -this->jumpForce;
    }
}

void Character::takeDamage(int amount) {
    this->health -= amount;
}
