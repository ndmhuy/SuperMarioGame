#pragma once

#include "Entities/Character.hpp"
#include "Entities/IPlayerState.hpp"
#include "Graphics/Animator.hpp"
#include "Graphics/SpriteSheet.hpp"
#include <memory>

struct PlayerSnapshot;

class Player : public Character {
public:
    bool hasArtwork() const override { return m_animator && m_hasAnimation; }
    sf::Vector2f artworkSize() const override {
        if (!m_animator || !m_hasAnimation) return {0.0f, 0.0f};
        const auto b = m_animator->getSprite().getLocalBounds();
        return {b.size.x, b.size.y};
    }

    explicit Player(sf::Vector2f pos = {0.0f, 0.0f}, sf::Vector2f targetSize = {32.0f, 32.0f}) : Character(pos, targetSize) {}
    ~Player() override = default;

    // Jump with coyote time and jump buffering. A press slightly after walking off
    // a ledge still jumps; a press slightly before landing is queued and fires on
    // touchdown. Both counters existed but were never read before (audit A-4).
    void jump() override;

    // Advanced movement controls
    virtual void run();
    virtual void wallJump();
    virtual void groundPound();
    virtual void crouch();
    virtual void slide();
    virtual void shootFireball();

    // True only in the Fire state (through any decorator) and off cooldown.
    // shootFireball() had NO state check at all: Small, Super, Cape and Mini
    // Mario could all throw fireballs, which made the Fire Flower pointless.
    bool canShootFireball() const;

    // Powerup state transitions
    void powerUp(int itemType);
    void powerDown();

    // Active state methods
    IPlayerState* getCurrentState() const;
    void changeState(std::unique_ptr<IPlayerState> state);

    // Innermost base form, with any Star/Mega decorators unwrapped.
    IPlayerState* getBaseState() const;
    // Swap the base form while leaving active decorators in place. Base-form
    // power-ups must use this; changeState() would discard an active Star or
    // Mega (audit A-8).
    void setBaseState(std::unique_ptr<IPlayerState> newBase);
    void update(float dt) override;
    void render(sf::RenderTarget& target) override;
    virtual void setupAnimations(const SpriteSheet* spriteSheet);

    // Action methods (enforce game rules)
    void addCoins(int amount);
    void addScore(int amount);
    void gainLife();
    void loseLife();

    // Silently install carried-over stats when swapping the player instance
    // (character select, level transition, save load). Unlike addCoins()/loseLife()
    // this publishes no events, so statistics and achievements are not inflated.
    void restoreStats(int newLives, int newCoins, int newScore);
    void resetCombo();
    void incrementCombo();

    // Read-only getters for external consumers (HUD, UI, save)
    virtual std::string getCharacterName() const = 0;
    EntityCategory getCategory() const override { return EntityCategory::Player; }
    int getLives() const { return lives; }
    int getCoins() const { return coins; }
    int getScore() const { return score; }
    float getInvincibilityTimer() const { return invincibilityTimer; }
    // Grant invincibility for the given duration (used by test harnesses and Star power)
    void setInvincible(float duration) { invincibilityTimer = duration; }
    bool isImmortal() const { return m_isImmortal; }
    void setImmortal(bool immortal) { m_isImmortal = immortal; }
    void takeDamage(int amount) override;
    int getCoyoteFramesLeft() const { return coyoteFramesLeft; }
    int getJumpBufferFramesLeft() const { return jumpBufferFramesLeft; }
    int getComboCounter() const { return comboCounter; }
    bool isCrouched() const { return crouched; }
    bool isSliding() const { return sliding; }
    bool isRunRequested() const { return m_runRequested; }
    void clearMovementRequests() override;

    // Held entity mechanics (e.g. carrying Koopa Shells)
    Entity* getHeldEntity() const { return m_heldEntity; }
    void holdEntity(Entity* entity) { m_heldEntity = entity; }
    void releaseHeldEntity() { m_heldEntity = nullptr; }

    // Throws whatever is being carried in the facing direction and returns true
    // if something was thrown. Nothing used to clear m_heldEntity, so a picked-up
    // shell was carried for the rest of the level with no way to put it down.
    bool throwHeldEntity();

    // --- Cape ---------------------------------------------------------------
    //
    // Which pad drives this player, so a state can ask the InputManager about
    // the *bound* jump key rather than naming a physical one.
    int getPlayerIndex() const;

    // Set by CapeState while the player is drifting down under the cape. Read by
    // the animation picker and by the physics glide clamp.
    bool isGliding() const { return m_gliding; }
    void setGliding(bool gliding) { m_gliding = gliding; }

    // Swings the cape. Returns false when the player is not caped, so the fire
    // button can fall through to the fireball. While the spin lasts, touching an
    // enemy defeats it instead of hurting the player.
    bool spinCape();
    bool isSpinningCape() const { return m_capeSpinTimer > 0.0f; }
    float getCapeSpinTimer() const { return m_capeSpinTimer; }

    // Drops it in place without launching it. Used when the player is hurt or
    // dies: a carried shell must not follow a corpse around.
    void dropHeldEntity();

    // Encapsulated Memento pattern methods
    PlayerSnapshot createSnapshot() const;
    void restoreMemento(const PlayerSnapshot& snapshot);

protected:
    friend class Serializer;
    friend class PlayingState;
    friend class TimeRewindManager;
    std::unique_ptr<IPlayerState> m_currentState;

    // Player stats — modified only through action methods or friends
    int lives = 3;
    int coins = 0;
    int score = 0;

    // Active mechanics counters
    float invincibilityTimer = 0.0f;
    int coyoteFramesLeft = 0;
    int jumpBufferFramesLeft = 0;
    int comboCounter = 0;
    bool crouched = false;
    bool sliding = false;
    bool m_crouchRequestedThisFrame = false;
    bool m_runRequested = false;
    bool m_groundPounding = false;
    bool m_isImmortal = false;
    float m_fireballCooldownTimer = 0.0f;
    Entity* m_heldEntity = nullptr;
    bool m_gliding = false;
    float m_capeSpinTimer = 0.0f;

    std::unique_ptr<Animator> m_animator;
    const SpriteSheet* m_spriteSheet = nullptr;
    Animation m_animation;
    Animation m_animIdle;
    Animation m_animWalk;
    Animation m_animRun;
    Animation m_animJump;
    Animation m_animCrouch;
    Animation m_animHurt;
    Animation m_animHoldIdle;
    Animation m_animHoldWalk;
    Animation m_animHoldCrouch;
    std::string m_currentAnimName;
    bool m_hasAnimation = false;

    void setupCharacterAnimations(const SpriteSheet* spriteSheet, const std::string& prefix);

    // Unconditional jump impulse. Callers are responsible for deciding whether a
    // jump is allowed; this just performs it and clears both jump counters.
    void performJump();

    // Resize the bounding box to the active state, keeping the feet planted.
    void applyStateSize();
    // Re-point the animator at the innermost base form's sprite prefix.
    void refreshStateAnimations();
    // Unwrap any timed decorator that reported isExpired(). Must be called from
    // update() after the state's own update() has returned — never from inside a
    // state, which would free the object mid-call (audit A-7).
    void unwrapExpiredState();
};


