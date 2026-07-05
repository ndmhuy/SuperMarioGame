#pragma once

#include "Entities/Character.hpp"
#include "Entities/IPlayerState.hpp"
#include <memory>

class Player : public Character {
public:
    Player() = default;
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

    // Action methods (enforce game rules)
    void addCoins(int amount);
    void addScore(int amount);
    void gainLife();
    void loseLife();
    void resetCombo();
    void incrementCombo();

    // Read-only getters for external consumers (HUD, UI, save)
    int getLives() const { return lives; }
    int getCoins() const { return coins; }
    int getScore() const { return score; }
    float getInvincibilityTimer() const { return invincibilityTimer; }
    int getCoyoteFramesLeft() const { return coyoteFramesLeft; }
    int getJumpBufferFramesLeft() const { return jumpBufferFramesLeft; }
    int getComboCounter() const { return comboCounter; }
    bool isCrouched() const { return crouched; }
    bool isSliding() const { return sliding; }
    bool isRunRequested() const { return m_runRequested; }

protected:
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
    float m_fireballCooldownTimer = 0.0f;
};

