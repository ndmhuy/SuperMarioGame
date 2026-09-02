#pragma once

#include "Entities/IMovementStrategy.hpp"
#include <string>
#include <SFML/System/Vector2.hpp>
#include <functional>

class HammerThrowStrategy : public IMovementStrategy {
public:
    explicit HammerThrowStrategy(float throwCooldown = 1.5f, float jumpCooldown = 3.0f);
    virtual ~HammerThrowStrategy() override = default;

    std::string getName() const override { return "HammerThrow"; }

    // Callback to allow spawning projectiles without circular dependencies
    void setThrowCallback(std::function<void(sf::Vector2f position, bool faceRight)> callback);
    void setThrowCallbackVel(std::function<void(sf::Vector2f position, sf::Vector2f velocity)> callback);

protected:
    void calculateTarget(Enemy& enemy, float dt) override;
    void applyMovement(Enemy& enemy, float dt) override;

private:
    // Shuffle speed, unchanged from the literal it replaces. Deliberately not
    // Enemy::speed: the shuffle is a fidget in place, and driving it from the
    // tuned walk speed would send a Hammer Bro pacing across the screen.
    static constexpr float SHUFFLE_SPEED = 30.0f;
    // How far past the leading edge the ledge check looks, matching
    // PatrolStrategy's own probe distance so both turn around in the same place.
    static constexpr float SHUFFLE_PROBE_AHEAD = 4.0f;

    // The last shuffle direction the ledge check approved while grounded, held
    // across the periodic hop so the jump cannot carry him off the edge.
    float m_groundShuffleDir = 0.0f;

    float m_throwCooldownTimer;
    float m_jumpCooldownTimer;
    float m_throwCooldownMax;
    float m_jumpCooldownMax;

    std::function<void(sf::Vector2f position, bool faceRight)> m_throwCallback;
    std::function<void(sf::Vector2f position, sf::Vector2f velocity)> m_throwVelCallback;
};
