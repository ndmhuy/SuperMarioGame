#include "Entities/Player.hpp"
#include "Entities/KoopaTroopa.hpp"
#include "Core/InputManager.hpp"
#include "Core/EventBus.hpp"
#include "Core/SoundManager.hpp"
#include "Core/GameSnapshot.hpp"
#include "Core/Game.hpp"
#include "Utils/TileMap.hpp"
#include "Utils/Constants.hpp"
#include "Graphics/SpriteColorFilter.hpp"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>

void Player::performJump() {
    velocity.y = -jumpForce;
    onGround = false;
    coyoteFramesLeft = 0;      // one jump per grace window
    jumpBufferFramesLeft = 0;  // request satisfied
    SoundManager::getInstance().playSound("jump_small");
}

void Player::jump() {
    if (m_dying) return;
    if (onGround || coyoteFramesLeft > 0) {
        performJump();
    } else {
        // Airborne: remember the request briefly. Player::update fires it the
        // moment we touch down, so a press a few frames early is not swallowed.
        jumpBufferFramesLeft = Constants::JUMP_BUFFER_FRAMES;
    }
}

void Player::run() {
    if (m_dying) return;
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
        m_groundPounding = true;
    }
}

void Player::crouch() {
    m_crouchRequestedThisFrame = true;
}

void Player::slide() {
    m_crouchRequestedThisFrame = true;
}

bool Player::canShootFireball() const {
    // Debug > Cheats' INFINITE FIREBALLS lifts the cooldown; PlayingState's
    // PlayerShotFireball handler lifts the two-on-screen cap. Both are needed —
    // either one alone still rations the throws.
    if (m_fireballCooldownTimer > 0.0f &&
        !Game::getInstance().debugCheats().liftsFireballCap()) {
        return false;
    }
    // getBaseState() unwraps Star and Mega, so an invincible or giant Fire Mario
    // keeps his fireballs — the decorators wrap the form, they do not replace it.
    return dynamic_cast<FireState*>(getBaseState()) != nullptr;
}

Player::Form Player::getForm() const {
    IPlayerState* base = getBaseState();
    if (dynamic_cast<SuperState*>(base)) return Form::Super;
    if (dynamic_cast<FireState*>(base))  return Form::Fire;
    if (dynamic_cast<CapeState*>(base))  return Form::Cape;
    if (dynamic_cast<MiniState*>(base))  return Form::Mini;
    return Form::Small;
}

void Player::setStartingForm(Form form) {
    const sf::Vector2f keep = position;
    setForm(form);
    setPosition(keep);
}

void Player::setForm(Form form) {
    switch (form) {
        case Form::Super: setBaseState(std::make_unique<SuperState>()); break;
        case Form::Fire:  setBaseState(std::make_unique<FireState>());  break;
        case Form::Cape:  setBaseState(std::make_unique<CapeState>());  break;
        case Form::Mini:  setBaseState(std::make_unique<MiniState>());  break;
        case Form::Small: setBaseState(std::make_unique<SmallState>()); break;
    }
}

void Player::moveLeft() {
    if (m_dying) return;
    Character::moveLeft();
}

void Player::moveRight() {
    if (m_dying) return;
    Character::moveRight();
}

void Player::beginDeathFall() {
    if (m_dying) return;
    m_dying = true;
    dropHeldEntity();
    clearMovementRequests();
    // Up first, then gravity takes it down through the floor: collidesWithTiles()
    // is false for the whole fall.
    velocity = {0.0f, -420.0f};
    onGround = false;
    m_gliding = false;
    m_capeSpinTimer = 0.0f;
    invincibilityTimer = 0.0f;
}

int Player::getPlayerIndex() const {
    // Two pads, and the InputManager is the only thing that knows which is which.
    return (this == InputManager::getInstance().getPlayer(1)) ? 1 : 0;
}

bool Player::spinCape() {
    if (!dynamic_cast<CapeState*>(getBaseState())) return false;
    if (m_capeSpinTimer > 0.0f) return true;   // already mid-swing
    m_capeSpinTimer = 0.35f;
    SoundManager::getInstance().playSound("kick");
    return true;
}

bool Player::throwHeldEntity() {
    if (!m_heldEntity) return false;

    // Only Koopa shells are carryable today, but the hook is on Player so the
    // rule stays "throw what you are holding" rather than "throw a shell".
    if (auto* koopa = dynamic_cast<KoopaTroopa*>(m_heldEntity)) {
        m_heldEntity = nullptr;
        koopa->throwShell();
        return true;
    }

    m_heldEntity = nullptr;
    return true;
}

void Player::dropHeldEntity() {
    if (!m_heldEntity) return;
    if (auto* koopa = dynamic_cast<KoopaTroopa*>(m_heldEntity)) {
        koopa->release();
    }
    m_heldEntity = nullptr;
}

void Player::shootFireball() {
    // One button, three meanings, in the order the originals use:
    //   carrying something -> throw it
    //   wearing the cape   -> swing it
    //   fire form          -> shoot
    if (throwHeldEntity()) return;
    if (spinCape()) return;

    if (!canShootFireball()) return;

    EventBus::getInstance().publish({EventType::PlayerShotFireball, this});
    m_fireballCooldownTimer = 0.3f; // 0.3s cooldown between shots
}

IPlayerState* Player::getBaseState() const {
    IPlayerState* baseState = m_currentState.get();
    while (auto* decorator = dynamic_cast<PlayerStateDecorator*>(baseState)) {
        baseState = decorator->getWrappedState();
    }
    return baseState;
}

void Player::setBaseState(std::unique_ptr<IPlayerState> newBase) {
    if (!newBase) return;

    // No decorators active: this is an ordinary state change.
    auto* outermost = dynamic_cast<PlayerStateDecorator*>(m_currentState.get());
    if (!outermost) {
        changeState(std::move(newBase));
        return;
    }

    // Walk to the innermost decorator and swap the base form underneath it, so an
    // active Star or Mega survives collecting a Fire Flower (audit A-8).
    PlayerStateDecorator* innermost = outermost;
    while (auto* next = dynamic_cast<PlayerStateDecorator*>(innermost->getWrappedState())) {
        innermost = next;
    }

    if (IPlayerState* oldBase = innermost->getWrappedState()) {
        oldBase->exit(*this);
    }
    newBase->enter(*this);
    innermost->setWrappedState(std::move(newBase));

    applyStateSize();
    refreshStateAnimations();
}

void Player::powerUp(int itemType) {
    if (itemType == 0) { // Mushroom: Small -> Super
        if (dynamic_cast<SmallState*>(getBaseState())) {
            setBaseState(std::make_unique<SuperState>());
        }
    } else if (itemType == 1) { // FireFlower: -> Fire
        setBaseState(std::make_unique<FireState>());
    } else if (itemType == 2) { // CapeFeather: -> Cape
        setBaseState(std::make_unique<CapeState>());
    } else if (itemType == 3) { // MiniMushroom: -> Mini
        setBaseState(std::make_unique<MiniState>());
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

bool Player::hasStarPower() const {
    IPlayerState* state = m_currentState.get();
    while (auto* decorator = dynamic_cast<PlayerStateDecorator*>(state)) {
        if (dynamic_cast<StarDecorator*>(decorator)) return true;
        state = decorator->getWrappedState();
    }
    return false;
}

bool Player::collidesWithTiles() const {
    return !m_dying && !Game::getInstance().debugCheats().passesThroughSolids();
}

float Player::getGravityMultiplier() const {
    // NOCLIP is a ghost, not a very light player: falling while phased through
    // the floor would drop straight past the void plane. Luigi and Peach chain
    // to this, so the rule lives in exactly one place.
    return Game::getInstance().debugCheats().passesThroughSolids() ? 0.0f : 1.0f;
}

void Player::takeDamage(int amount) {
    if (m_dying) return;
    // Debug > Cheats' INVINCIBLE. Ahead of the i-frame check rather than
    // implemented as a very long invincibilityTimer, which is what the console's
    // `god` command used to do: that value is also what drives the hurt
    // animation and the sprite dimming, so it could not be told apart from
    // having just been hit, and it protected only the paths that happen to read
    // it. This is the single door all ten damage sources come through.
    if (Game::getInstance().debugCheats().blocksDamage()) return;
    if (invincibilityTimer > 0.0f || hasStarPower()) return;
    // Taking a hit breaks the chain — that is what makes a long combo a risk
    // worth taking rather than a number that only grows.
    resetCombo();
    dropHeldEntity();
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
        setBaseState(std::make_unique<SuperState>());
        EventBus::getInstance().publish({EventType::PlayerDamaged, this});
    } else if (dynamic_cast<SuperState*>(state)) {
        setBaseState(std::make_unique<SmallState>());
        EventBus::getInstance().publish({EventType::PlayerDamaged, this});
    } else if (dynamic_cast<SmallState*>(state)) {
        // Report the death; do not account for it. Life accounting and the
        // death sequence belong to PlayingState, which is the only thing that
        // knows about checkpoints and game over. This used to call loseLife()
        // here and publish an event nothing in PlayingState subscribed to, so
        // an enemy killing Small Mario silently docked a life and left him
        // standing where he was.
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
    // Each base state has its own sprite set. Super, Fire and Cape used to fall
    // through to "small", so every power-up looked identical in play (the art
    // simply did not exist until tools/powerup-frames derived it).
    std::string stateSuffix = "small";
    if (dynamic_cast<MiniState*>(baseState)) {
        stateSuffix = "tiny"; // Mini state maps to _tiny sprite frames
    } else if (dynamic_cast<FireState*>(baseState)) {
        stateSuffix = "fire";
    } else if (dynamic_cast<CapeState*>(baseState)) {
        stateSuffix = "cape";
    } else if (dynamic_cast<SuperState*>(baseState)) {
        stateSuffix = "super";
    }

    // Fall back rather than render nothing if an atlas predates the derived
    // frames: setupCharacterAnimations() would otherwise ask for frames that are
    // not there and leave the player invisible.
    const std::string prefix = getCharacterName() + "_" + stateSuffix;
    if (stateSuffix != "small" && !m_spriteSheet->hasFrame(prefix + "_idle")
                               && !m_spriteSheet->hasFrame(prefix + "_walk_0")) {
        std::cerr << "[Player] Atlas has no \"" << prefix
                  << "\" frames; falling back to _small." << std::endl;
        setupCharacterAnimations(m_spriteSheet, getCharacterName() + "_small");
        return;
    }
    setupCharacterAnimations(m_spriteSheet, prefix);
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

    std::string deathFrame = prefix + "_death";
    if (!spriteSheet->hasFrame(deathFrame)) {
        std::string charName = prefix;
        size_t underPos = charName.find('_');
        if (underPos != std::string::npos) {
            charName = charName.substr(0, underPos);
        }
        deathFrame = charName + "_death";
    }
    if (!spriteSheet->hasFrame(deathFrame)) {
        deathFrame = "mario_death";
    }
    m_animDeath = Animation(prefix + "_death");
    m_animDeath.frameList = {{deathFrame, 0.15f}};

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
    // Debug > Cheats' NOCLIP. getGravityMultiplier() has already switched gravity
    // off, so a jump impulse would carry the ghost upward forever and nothing
    // would ever bring it down; the damping turns jump into an upward nudge and
    // crouch is the other direction. Deliberately reuses the jump and crouch the
    // recorder already has under their fingers rather than adding two more
    // bindings to learn. m_crouchRequestedThisFrame is written by
    // InputManager::update, which PlayingState runs before this (step 2 vs 3).
    if (Game::getInstance().debugCheats().passesThroughSolids()) {
        if (m_crouchRequestedThisFrame) velocity.y = Constants::WALK_SPEED;
        velocity.y *= 0.88f;
        if (std::abs(velocity.y) < 1.0f) velocity.y = 0.0f;
        onGround = false;
    }

    if (m_capeSpinTimer > 0.0f) {
        m_capeSpinTimer -= dt;
        if (m_capeSpinTimer < 0.0f) m_capeSpinTimer = 0.0f;
    }
    // The combo chain expires. Nothing used to tick this, so the counter only
    // ever went up.
    if (comboTimer > 0.0f) {
        comboTimer -= dt;
        if (comboTimer <= 0.0f) resetCombo();
    }
    if (onGround) m_gliding = false;

    if (m_animator && m_hasAnimation) {
        Animation* targetAnim = &m_animIdle;
        if (m_dying) {
            targetAnim = &m_animDeath;
        } else if (invincibilityTimer > 1.7f && invincibilityTimer < 9000.0f) {
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

    // 1b. A ground pound that has reached the floor. GroundPoundSlam had two
    // subscribers waiting — camera shake and the impact SFX — and no publisher,
    // so pounding landed with no feedback at all (audit G-4).
    if (m_groundPounding && onGround) {
        m_groundPounding = false;
        EventBus::getInstance().publish({EventType::GroundPoundSlam, this});
    }

    // 2. Coyote time and jump buffering.
    if (onGround) {
        // Refresh the grace window while grounded, and satisfy any jump that was
        // pressed just before touchdown.
        coyoteFramesLeft = Constants::COYOTE_FRAMES;
        if (jumpBufferFramesLeft > 0) {
            performJump();
        }
    } else if (coyoteFramesLeft > 0) {
        // Airborne: burn down the post-ledge grace window.
        --coyoteFramesLeft;
    }

    if (jumpBufferFramesLeft > 0) {
        --jumpBufferFramesLeft;
    }

    // 3. Handle Crouching transitions
    if (m_crouchRequestedThisFrame) {
        if (!crouched) {
            crouched = true;
            // Shrink hitbox height only if standing height is greater than 32px (Super/Fire/Cape states)
            if (m_currentState && m_currentState->getSize().y > 32.0f) {
                float crouchedHeight = boundingBox.height * 0.5f;
                position.y += crouchedHeight; // Grow downward from top (feet stay grounded)
                setTargetSize({boundingBox.width, crouchedHeight});
                boundingBox.y = position.y;
            }
        }
    } else {
        // Stand up if crouch key was released
        if (crouched) {
            if (m_currentState && m_currentState->getSize().y > 32.0f) {
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
            } else {
                crouched = false;
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

        // Hurt i-frames flicker the sprite between 100 and 255 alpha at ~15Hz.
        if (invincibilityTimer > 0.0f && invincibilityTimer < 9000.0f) {
            const bool dim = (static_cast<int>(invincibilityTimer * 30.0f) % 2 == 0);
            sprite.setColor(sf::Color(255, 255, 255, dim ? 100 : 255));
        } else {
            // Star power cycles the sprite through the authentic multi-color
            // invincibility palette for as long as a StarDecorator wraps the
            // current state.
            IPlayerState* state = m_currentState.get();
            while (auto* decorator = dynamic_cast<PlayerStateDecorator*>(state)) {
                if (auto* star = dynamic_cast<StarDecorator*>(decorator)) {
                    const float elapsed = Constants::STAR_DURATION - star->getTimeLeft();
                    sprite.setColor(SpriteColorFilter::getMarioStarPaletteColor(elapsed, 0.06f));
                    break;
                }
                state = decorator->getWrappedState();
            }
        }

        // Sprites face left in the atlas, so facingRight is the flip case.
        drawSprite(target, sprite, SpriteAnchor::BottomCenter, /*flipX=*/facingRight);
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
    // Debug > Cheats' INFINITE LIVES. Distinct from m_isImmortal below, which is
    // ShadowMario's "cannot be removed from the level" and forcibly resets the
    // count to 1 and the form to Small: a recording must keep the life count the
    // HUD is showing exactly where it was.
    if (Game::getInstance().debugCheats().preservesLives()) return;
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
    comboTimer = 0.0f;
}

void Player::incrementCombo() {
    ++comboCounter;
    // Every increment refreshes the window. A combo is a *chain* — kills close
    // together — and without this the counter was monotonic for the whole life of
    // the Player: resetCombo() existed and had no callers anywhere, so nothing
    // ever brought it back down. Every stomp in the level raised it permanently,
    // which both pinned an "x7!" to the middle of the screen forever and
    // permanently inflated the score multiplier.
    comboTimer = COMBO_WINDOW;
    EventBus::getInstance().publish({EventType::ComboHit, comboCounter});
}

void Player::clearMovementRequests() {
    Character::clearMovementRequests();
    m_runRequested = false;
}

float Player::getRunSpeed() const {
    return Constants::RUN_SPEED;
}

float Player::getCurrentMaxSpeed() const {
    return m_runRequested ? getRunSpeed() : speed;
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


