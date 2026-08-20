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
    float m_throwCooldownTimer;
    float m_jumpCooldownTimer;
    float m_throwCooldownMax;
    float m_jumpCooldownMax;

    std::function<void(sf::Vector2f position, bool faceRight)> m_throwCallback;
    std::function<void(sf::Vector2f position, sf::Vector2f velocity)> m_throwVelCallback;
};
