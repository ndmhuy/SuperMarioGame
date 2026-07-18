#include "Entities/Character.hpp"
#include "Utils/Constants.hpp"

void Character::moveLeft() {
    this->velocity.x = -Constants::WALK_SPEED;
    this->facingRight = false;
}

void Character::moveRight() {
    this->velocity.x = Constants::WALK_SPEED;
    this->facingRight = true;
}

void Character::jump() {
    if (onGround || onWall) {
        this->velocity.y = -this->jumpForce;
    }
}

void Character::takeDamage(int amount) {
    this->health -= amount;
}
