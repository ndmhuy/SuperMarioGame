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

    // Player stats
    int lives = 3;
    int coins = 0;
    int score = 0;

    // Active mechanics counters
    float invincibilityTimer = 0.0f;
    int coyoteFramesLeft = 0;
    int jumpBufferFramesLeft = 0;
    int comboCounter = 0;

protected:
    std::unique_ptr<IPlayerState> m_currentState;
};
