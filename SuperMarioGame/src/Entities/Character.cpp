#include "Entities/Character.hpp"
#include "Utils/Constants.hpp"
#include "Utils/MathUtils.hpp"

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

void Character::applyHorizontalControl(float dt, float groundDecel) {
    const float maxSpeed = getCurrentMaxSpeed();
    const float accelRate = 1000.0f; // px/s^2 (0 -> 150 px/s in 0.15s)

    if (m_moveLeftRequested) {
        velocity.x -= accelRate * dt;
        velocity.x = MathUtils::clamp(velocity.x, -maxSpeed, maxSpeed);
    }
    else if (m_moveRightRequested) {
        velocity.x += accelRate * dt;
        velocity.x = MathUtils::clamp(velocity.x, -maxSpeed, maxSpeed);
    }
    else if (!suppressesGroundFriction()) {
        // Passive friction decay towards 0, at whatever rate the surface allows.
        if (velocity.x > 0.0f) {
            velocity.x -= groundDecel * dt;
            if (velocity.x < 0.0f) velocity.x = 0.0f;
        } else if (velocity.x < 0.0f) {
            velocity.x += groundDecel * dt;
            if (velocity.x > 0.0f) velocity.x = 0.0f;
        }
    }
}

void Character::beginPhysicsFrame() {
    // Reset ground/wall flags for the new collision detection pass. Preserve
    // the incoming value first: the X pass runs before the Y pass that
    // recomputes onGround, so anything reading it mid-frame would otherwise see
    // false for every character, grounded or not.
    wasOnGround = onGround;
    onGround = false;
    onWall = false;
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
