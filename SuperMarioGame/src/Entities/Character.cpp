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

#include "Core/SoundManager.hpp"

void Character::jump() {
    // Grounded only. Accepting onWall here let a held jump key climb any wall
    // indefinitely (audit A-4); wall jumps go through WallJumpCommand ->
    // Player::wallJump(), which applies the horizontal push-off as well.
    if (onGround) {
        this->velocity.y = -this->jumpForce;
        SoundManager::getInstance().playSound("jump_small");
    }
}

void Character::takeDamage(int amount) {
    this->health -= amount;
}
