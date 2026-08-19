#pragma once

#include "Entities/IMovementStrategy.hpp"
#include <string>
#include <SFML/System/Vector2.hpp>

class LinearStrategy : public IMovementStrategy {
public:
    explicit LinearStrategy(float speed = 200.0f, sf::Vector2f direction = sf::Vector2f(-1.0f, 0.0f));
    virtual ~LinearStrategy() override = default;

    std::string getName() const override { return "Linear"; }

    float getSpeed() const;
    void setSpeed(float speed);
    sf::Vector2f getDirection() const;
    void setDirection(sf::Vector2f direction);

protected:
    void applyMovement(Enemy& enemy, float dt) override;

private:
    float m_speed;
    sf::Vector2f m_direction;
};
