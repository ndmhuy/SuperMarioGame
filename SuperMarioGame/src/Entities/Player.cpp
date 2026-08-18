#include "Entities/Player.hpp"
#include "Core/EventBus.hpp"
#include "Core/GameSnapshot.hpp"
#include "Core/Game.hpp"
#include "Utils/TileMap.hpp"
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

void Player::takeDamage(int amount) {
    if (invincibilityTimer > 0.0f) return;
    Character::takeDamage(amount);
    powerDown();
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

void Player::applyStateSize() {
    if (!m_currentState) return;

    // Resize the bounding box to the active state, keeping the feet planted so
    // growing or shrinking never pushes the player through the floor.
    const sf::Vector2f newSize = m_currentState->getSize();
    const float heightDiff = newSize.y - boundingBox.height;
    position.y -= heightDiff;

    setTargetSize(newSize);
    boundingBox.x = position.x;
    boundingBox.y = position.y;
}

void Player::refreshStateAnimations() {
    if (!m_spriteSheet || !m_currentState) return;

    // Sprite prefix comes from the innermost base form; decorators do not have
    // their own sprite sets.
    IPlayerState* baseState = m_currentState.get();
    while (auto* decorator = dynamic_cast<PlayerStateDecorator*>(baseState)) {
        baseState = decorator->getWrappedState();
    }
    std::string stateSuffix = "small";
    if (dynamic_cast<MiniState*>(baseState)) {
        stateSuffix = "tiny"; // Mini state maps to _tiny sprite frames
    }
    setupCharacterAnimations(m_spriteSheet, getCharacterName() + "_" + stateSuffix);
}

void Player::changeState(std::unique_ptr<IPlayerState> state) {
    if (m_currentState) {
        m_currentState->exit(*this);
    }
    m_currentState = std::move(state);
    if (m_currentState) {
        m_currentState->enter(*this);
        applyStateSize();
        refreshStateAnimations();
    }
}

void Player::unwrapExpiredState() {
    // Retire expired timed decorators. Called from update() *after* the state's own
    // update() has returned, so destroying it here is safe. Loops because Star and
    // Mega can be stacked and may lapse on the same frame.
    while (m_currentState && m_currentState->isExpired()) {
        auto* decorator = dynamic_cast<PlayerStateDecorator*>(m_currentState.get());
        if (!decorator) break;   // nothing to fall back to

        std::unique_ptr<IPlayerState> inner = decorator->releaseWrappedState();
        if (!inner) break;

        // Only the decorator's own teardown runs: its m_wrappedState is already
        // null, so the inner state is not exited — it never left, and deliberately
        // does not get enter() called on it again.
        m_currentState->exit(*this);
        m_currentState = std::move(inner);
        applyStateSize();
        refreshStateAnimations();
    }
}

void Player::setupAnimations(const SpriteSheet* spriteSheet) {
    setupCharacterAnimations(spriteSheet, "mario_small");
}

void Player::setupCharacterAnimations(const SpriteSheet* spriteSheet, const std::string& prefix) {
    if (!spriteSheet) return;
    m_animator = std::make_unique<Animator>(spriteSheet);
    m_spriteSheet = spriteSheet;

    std::string idleFrame = prefix + "_idle";
    if (!spriteSheet->hasFrame(idleFrame)) {
        idleFrame = prefix + "_walk_0";
    }

    std::string crouchFrame = prefix + "_crouch";
    if (!spriteSheet->hasFrame(crouchFrame)) {
        crouchFrame = idleFrame;
    }

    std::string hurtFrame = prefix + "_hurt";
    if (!spriteSheet->hasFrame(hurtFrame)) {
        hurtFrame = idleFrame;
    }

    std::string walkFrame0 = prefix + "_walk_0";
    std::string walkFrame1 = prefix + "_walk_1";
    if (!spriteSheet->hasFrame(walkFrame0)) walkFrame0 = idleFrame;
    if (!spriteSheet->hasFrame(walkFrame1)) walkFrame1 = idleFrame;

    // Hold/carry frames in atlas are named prefix + "_run_0" / "_run_1" and "_crouch_hold"
    std::string holdFrame0 = prefix + "_run_0";
    std::string holdFrame1 = prefix + "_run_1";
    if (!spriteSheet->hasFrame(holdFrame0)) holdFrame0 = walkFrame0;
    if (!spriteSheet->hasFrame(holdFrame1)) holdFrame1 = walkFrame1;

    std::string holdCrouchFrame = prefix + "_crouch_hold";
    if (!spriteSheet->hasFrame(holdCrouchFrame)) holdCrouchFrame = holdFrame0;

    std::string jumpFrame = prefix + "_walk_1";
    if (!spriteSheet->hasFrame(jumpFrame)) jumpFrame = walkFrame1;

    m_animIdle = Animation(prefix + "_idle");
    m_animIdle.frameList = {{idleFrame, 0.15f}};

    m_animWalk = Animation(prefix + "_walk");
    m_animWalk.frameList = {{walkFrame0, 0.12f}, {walkFrame1, 0.12f}};

    // Running animation uses the same sprites as walking, but faster
    m_animRun = Animation(prefix + "_run");
    m_animRun.frameList = {{walkFrame0, 0.07f}, {walkFrame1, 0.07f}};

    m_animJump = Animation(prefix + "_jump");
    m_animJump.frameList = {{jumpFrame, 0.15f}};

    m_animCrouch = Animation(prefix + "_crouch");
    m_animCrouch.frameList = {{crouchFrame, 0.15f}};

    m_animHurt = Animation(prefix + "_hurt");
    m_animHurt.frameList = {{hurtFrame, 0.15f}};

    // Hold animations for carrying objects/shells
    m_animHoldIdle = Animation(prefix + "_hold_idle");
    m_animHoldIdle.frameList = {{holdFrame0, 0.15f}};

    m_animHoldWalk = Animation(prefix + "_hold_walk");
    m_animHoldWalk.frameList = {{holdFrame0, 0.10f}, {holdFrame1, 0.10f}};

    m_animHoldCrouch = Animation(prefix + "_hold_crouch");
    m_animHoldCrouch.frameList = {{holdCrouchFrame, 0.15f}};

    m_animator->play(&m_animIdle);
    m_currentAnimName = m_animIdle.name;
    m_hasAnimation = true;
}

void Player::update(float dt) {
    if (m_animator && m_hasAnimation) {
        Animation* targetAnim = &m_animIdle;
        if (invincibilityTimer > 1.7f && invincibilityTimer < 9000.0f) {
            targetAnim = &m_animHurt;
        } else if (m_heldEntity != nullptr) {
            if (crouched) {
                targetAnim = &m_animHoldCrouch;
            } else if (std::abs(velocity.x) > 10.0f || !onGround) {
                targetAnim = &m_animHoldWalk;
            } else {
                targetAnim = &m_animHoldIdle;
            }
        } else if (crouched) {
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
            float standingHeight = m_currentState->getSize().y;
            float heightDiff = standingHeight - boundingBox.height;

            bool canStand = true;
            TileMap* tileMap = Game::getInstance().getTileMap();
            if (tileMap) {
                AABB targetBox {
                    boundingBox.x,
                    boundingBox.y - heightDiff,
                    boundingBox.width,
                    standingHeight
                };

                int startX = static_cast<int>(std::floor(targetBox.x / Constants::TILE_SIZE));
                int endX = static_cast<int>(std::floor((targetBox.x + targetBox.width) / Constants::TILE_SIZE));
                int startY = static_cast<int>(std::floor(targetBox.y / Constants::TILE_SIZE));
                int endY = static_cast<int>(std::floor((targetBox.y + targetBox.height) / Constants::TILE_SIZE));

                for (int y = startY; y <= endY; ++y) {
                    for (int x = startX; x <= endX; ++x) {
                        TileType tile = tileMap->getTileType(x, y);
                        if (TileMap::getInfo(tile).isSolid) {
                            AABB tileBox {
                                x * Constants::TILE_SIZE,
                                y * Constants::TILE_SIZE,
                                Constants::TILE_SIZE,
                                Constants::TILE_SIZE
                            };
                            if (targetBox.intersects(tileBox)) {
                                canStand = false;
                                break;
                            }
                        }
                    }
                    if (!canStand) break;
                }
            }

            if (canStand) {
                crouched = false;
                position.y -= heightDiff; // Stand upward
                setTargetSize({m_currentState->getSize().x, standingHeight});
                boundingBox.y = position.y;
            }
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

    // 6. Retire any expired timed state, now that its update() has returned and
    // destroying it is safe.
    unwrapExpiredState();
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

            // Hurt invincibility visual alpha flicker (15Hz modulation between 100 and 255 alpha)
            if (invincibilityTimer > 0.0f && invincibilityTimer < 9000.0f) {
                bool dim = (static_cast<int>(invincibilityTimer * 30.0f) % 2 == 0);
                std::uint8_t alpha = dim ? 100 : 255;
                sprite.setColor(sf::Color(255, 255, 255, alpha));
            }

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
    if (m_isImmortal) {
        lives = 1;
        changeState(std::make_unique<SmallState>());
        return; // Prevent dying in behavior test
    }
    if (lives > 0) {
        --lives;
    }
    if (lives <= 0) {
        EventBus::getInstance().publish({EventType::GameOver, 0});
    }
}

void Player::restoreStats(int newLives, int newCoins, int newScore) {
    lives = newLives;
    coins = newCoins;
    score = newScore;
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


