#pragma once

#include "Entities/Character.hpp"
#include "Entities/IPlayerState.hpp"
#include "Graphics/Animator.hpp"
#include "Graphics/SpriteSheet.hpp"
#include <memory>

struct PlayerSnapshot;

class Player : public Character {
public:
    explicit Player(sf::Vector2f pos = {0.0f, 0.0f}, sf::Vector2f targetSize = {32.0f, 32.0f}) : Character(pos, targetSize) {}
    ~Player() override = default;

    // Advanced movement controls
    virtual void run();
    virtual void wallJump();
    virtual void groundPound();
    virtual void crouch();
    virtual void slide();
    virtual void shootFireball();

    // Powerup state transitions
    void powerUp(int itemType);
    void powerDown();

    // Active state methods
    IPlayerState* getCurrentState() const;
    void changeState(std::unique_ptr<IPlayerState> state);
    void update(float dt) override;
    void render(sf::RenderTarget& target) override;
    virtual void setupAnimations(const SpriteSheet* spriteSheet);

    // Action methods (enforce game rules)
    void addCoins(int amount);
    void addScore(int amount);
    void gainLife();
    void loseLife();
    void resetCombo();
    void incrementCombo();

    // Read-only getters for external consumers (HUD, UI, save)
    virtual std::string getCharacterName() const = 0;
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
    bool m_isImmortal = false;
    float m_fireballCooldownTimer = 0.0f;
    Entity* m_heldEntity = nullptr;

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
};


