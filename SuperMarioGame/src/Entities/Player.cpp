#include "Entities/Player.hpp"
#include "Core/EventBus.hpp"
#include "Core/GameSnapshot.hpp"
#include "Utils/Constants.hpp"
#include <algorithm>

void Player::run() {
    m_runRequested = true;
}

void Player::wallJump() {
    if (onWall) {
        // Wall jump pushes away from the wall. If facing right (clinging to a wall on the right),
        // we push left and face left. If facing left, we push right and face right.
        if (facingRight) {
            this->velocity.x = -Constants::RUN_SPEED;
            this->facingRight = false;
        } else {
            this->velocity.x = Constants::RUN_SPEED;
            this->facingRight = true;
        }
        this->velocity.y = -this->jumpForce; // Propel upward with jumpForce
    }
}

void Player::groundPound() {
    if (!onGround) { // Ground pound can only be initiated in the air
        this->velocity.x = 0.0f;
        this->velocity.y = Constants::GROUND_POUND_SPEED;
    }
}

void Player::crouch() {
    m_crouchRequestedThisFrame = true;
}

void Player::slide() {
    m_crouchRequestedThisFrame = true;
}

void Player::shootFireball() {
    if (m_fireballCooldownTimer > 0.0f) return;

    EventBus::getInstance().publish({EventType::PlayerShotFireball, this});
    m_fireballCooldownTimer = 0.3f; // 0.3s cooldown between shots
}

void Player::powerUp(int itemType) {
    if (itemType == 0) { // Mushroom: Small -> Super
        IPlayerState* baseState = m_currentState.get();
        while (auto* decorator = dynamic_cast<PlayerStateDecorator*>(baseState)) {
            baseState = decorator->getWrappedState();
        }
        if (dynamic_cast<SmallState*>(baseState)) {
            changeState(std::make_unique<SuperState>());
        }
    } else if (itemType == 1) { // FireFlower: -> Fire
        changeState(std::make_unique<FireState>());
    } else if (itemType == 2) { // CapeFeather: -> Cape
        changeState(std::make_unique<CapeState>());
    } else if (itemType == 3) { // MiniMushroom: -> Mini
        changeState(std::make_unique<MiniState>());
    } else if (itemType == 4) { // Star: Wrap current state with StarDecorator
        if (m_currentState) {
            auto starDecorator = std::make_unique<StarDecorator>(std::move(m_currentState));
            m_currentState = std::move(starDecorator);
            m_currentState->enter(*this);
        }
    } else if (itemType == 5) { // MegaMushroom: Wrap current state with MegaDecorator (triggers size growth)
        if (m_currentState) {
            auto megaDecorator = std::make_unique<MegaDecorator>(std::move(m_currentState));
            changeState(std::move(megaDecorator));
        }
    }
    EventBus::getInstance().publish({EventType::PowerUpCollected, itemType});
}

void Player::powerDown() {
    // If temporarily or permanently invincible (Star/Mega), ignore damage
    if (invincibilityTimer > 0.0f) return;

    IPlayerState* state = m_currentState.get();
    bool isInvincible = false;
    bool isMega = false;
    while (auto* decorator = dynamic_cast<PlayerStateDecorator*>(state)) {
        if (dynamic_cast<StarDecorator*>(decorator)) {
            isInvincible = true;
        }
        if (dynamic_cast<MegaDecorator*>(decorator)) {
            isMega = true;
        }
        state = decorator->getWrappedState();
    }

    if (isInvincible || isMega) return;

    // Apply temporary invincibility frames
    invincibilityTimer = 2.0f; // 2 seconds of flashing invincibility

    // Powerdown transition: Fire/Cape/Mini -> Super -> Small -> Death
    if (dynamic_cast<FireState*>(state) || dynamic_cast<CapeState*>(state) || dynamic_cast<MiniState*>(state)) {
        changeState(std::make_unique<SuperState>());
        EventBus::getInstance().publish({EventType::PlayerDamaged, this});
    } else if (dynamic_cast<SuperState*>(state)) {
        changeState(std::make_unique<SmallState>());
        EventBus::getInstance().publish({EventType::PlayerDamaged, this});
    } else if (dynamic_cast<SmallState*>(state)) {
        loseLife();
        EventBus::getInstance().publish({EventType::PlayerDied, this});
    }
}

IPlayerState* Player::getCurrentState() const {
    return m_currentState.get();
}

void Player::changeState(std::unique_ptr<IPlayerState> state) {
    if (m_currentState) {
        m_currentState->exit(*this);
    }
    m_currentState = std::move(state);
    if (m_currentState) {
        m_currentState->enter(*this);
        
        // Dynamically adjust player bounding box size to match the new state
        sf::Vector2f newSize = m_currentState->getSize();
        float heightDiff = newSize.y - boundingBox.height;
        
        // Adjust position.y so player's feet stay grounded when growing/shrinking
        position.y -= heightDiff;
        
        setTargetSize(newSize);
        boundingBox.x = position.x;
        boundingBox.y = position.y;
    }
}

void Player::setupAnimations(const SpriteSheet* spriteSheet) {
    setupCharacterAnimations(spriteSheet, "mario_small");
}

void Player::setupCharacterAnimations(const SpriteSheet* spriteSheet, const std::string& prefix) {
    if (!spriteSheet) return;
    m_animator = std::make_unique<Animator>(spriteSheet);

    m_animIdle = Animation(prefix + "_idle");
    m_animIdle.frameList = {{prefix + "_idle", 0.15f}};

    m_animWalk = Animation(prefix + "_walk");
    m_animWalk.frameList = {{prefix + "_walk_0", 0.12f}, {prefix + "_walk_1", 0.12f}};

    m_animRun = Animation(prefix + "_run");
    m_animRun.frameList = {{prefix + "_run_0", 0.08f}, {prefix + "_run_1", 0.08f}};

    m_animJump = Animation(prefix + "_jump");
    m_animJump.frameList = {{prefix + "_walk_1", 0.15f}};

    m_animCrouch = Animation(prefix + "_crouch");
    m_animCrouch.frameList = {{prefix + "_crouch", 0.15f}};

    m_animator->play(&m_animIdle);
    m_currentAnimName = m_animIdle.name;
    m_hasAnimation = true;
}

void Player::update(float dt) {
    if (m_animator && m_hasAnimation) {
        Animation* targetAnim = &m_animIdle;
        if (crouched) {
            targetAnim = &m_animCrouch;
        } else if (!onGround) {
            targetAnim = &m_animJump;
        } else if (std::abs(velocity.x) > Constants::WALK_SPEED * 1.1f) {
            targetAnim = &m_animRun;
        } else if (std::abs(velocity.x) > 10.0f) {
            targetAnim = &m_animWalk;
        }

        if (m_currentAnimName != targetAnim->name) {
            m_currentAnimName = targetAnim->name;
            m_animator->play(targetAnim);
        }

        m_animator->update(dt);
    }

    // 1. Update invincibility frames timer
    if (invincibilityTimer > 0.0f) {
        invincibilityTimer -= dt;
        if (invincibilityTimer < 0.0f) {
            invincibilityTimer = 0.0f;
        }
    }

    if (m_fireballCooldownTimer > 0.0f) {
        m_fireballCooldownTimer -= dt;
        if (m_fireballCooldownTimer < 0.0f) {
            m_fireballCooldownTimer = 0.0f;
        }
    }

    // 2. Update coyote time and jump buffer counters
    if (coyoteFramesLeft > 0) {
        --coyoteFramesLeft;
    }
    if (jumpBufferFramesLeft > 0) {
        --jumpBufferFramesLeft;
    }
    
    if (onGround) {
        coyoteFramesLeft = Constants::COYOTE_FRAMES;
    }

    // 3. Handle Crouching transitions
    if (m_crouchRequestedThisFrame) {
        // Only allow crouching if standing height is greater than 32px (Super/Fire/Cape states)
        if (!crouched && m_currentState && m_currentState->getSize().y > 32.0f) {
            crouched = true;
            float crouchedHeight = boundingBox.height * 0.5f;
            position.y += crouchedHeight; // Grow downward from top (feet stay grounded)
            setTargetSize({boundingBox.width, crouchedHeight});
            boundingBox.y = position.y;
        }
    } else {
        // Stand up if crouch key was released
        if (crouched && m_currentState) {
            crouched = false;
            float standingHeight = m_currentState->getSize().y;
            float heightDiff = standingHeight - boundingBox.height;
            position.y -= heightDiff; // Stand upward
            setTargetSize({m_currentState->getSize().x, standingHeight});
            boundingBox.y = position.y;
        }
    }

    // 4. Handle sliding physics
    if (crouched && std::abs(velocity.x) > 0.0f) {
        sliding = (std::abs(velocity.x) > Constants::WALK_SPEED);
        
        // Apply high slide friction to decelerate the player horizontally
        float deceleration = Constants::WALK_SPEED * 1.5f;
        if (velocity.x > 0.0f) {
            velocity.x -= deceleration * dt;
            if (velocity.x < 0.0f) velocity.x = 0.0f;
        } else if (velocity.x < 0.0f) {
            velocity.x += deceleration * dt;
            if (velocity.x > 0.0f) velocity.x = 0.0f;
        }
    } else {
        sliding = false;
    }

    m_crouchRequestedThisFrame = false;

    // 5. Delegate frame update to the active state
    if (m_currentState) {
        m_currentState->update(*this, dt);
    }
}

void Player::render(sf::RenderTarget& target) {
    if (!active) return;
    if (m_animator && m_hasAnimation) {
        sf::Sprite sprite = m_animator->getSprite();
        sf::FloatRect bounds = sprite.getLocalBounds();
        if (bounds.size.x > 0.0f && bounds.size.y > 0.0f) {
            float scale = std::min(m_targetSize.x / bounds.size.x, m_targetSize.y / bounds.size.y);
            float scaledW = bounds.size.x * scale;
            float scaledH = bounds.size.y * scale;

            // Base AABB remains locked to m_targetSize during all animation frames
            boundingBox.width = m_targetSize.x;
            boundingBox.height = m_targetSize.y;

            sprite.setOrigin(sf::Vector2f(bounds.size.x * 0.5f, bounds.size.y));
            float scaleX = facingRight ? -scale : scale;  // sprite faces left by default in atlas
            sprite.setScale(sf::Vector2f(scaleX, scale));
            sprite.setPosition(sf::Vector2f(boundingBox.x + m_targetSize.x * 0.5f, boundingBox.y + m_targetSize.y));

            target.draw(sprite);
        }
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

void Player::clearMovementRequests() {
    Character::clearMovementRequests();
    m_runRequested = false;
}

PlayerSnapshot Player::createSnapshot() const {
    PlayerSnapshot snap;
    snap.position = getPosition();
    snap.velocity = getVelocity();
    snap.score = score;
    snap.coins = coins;
    snap.lives = lives;
    snap.onGround = onGround;
    return snap;
}

void Player::restoreMemento(const PlayerSnapshot& snapshot) {
    setPosition(snapshot.position);
    setVelocity(snapshot.velocity);
    score = snapshot.score;
    coins = snapshot.coins;
    lives = snapshot.lives;
    onGround = snapshot.onGround;
}


