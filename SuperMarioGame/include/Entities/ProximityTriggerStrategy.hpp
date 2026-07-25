#pragma once

#include "Entities/IMovementStrategy.hpp"
#include <SFML/System/Vector2.hpp>

class ProximityTriggerStrategy : public IMovementStrategy {
public:
    explicit ProximityTriggerStrategy(sf::Vector2f homePos = sf::Vector2f(0.f, 0.f));
    virtual ~ProximityTriggerStrategy() override = default;

    sf::Vector2f getHomePos() const;
    void setHomePos(sf::Vector2f homePos);
    int getState() const;
    void setState(int state);

protected:
    void calculateTarget(Enemy& enemy, float dt) override;
    void applyMovement(Enemy& enemy, float dt) override;
    void checkConstraints(Enemy& enemy, float dt) override;

private:
    sf::Vector2f m_homePos;
    int m_state; // 0: Idle/Ceiling, 1: Slamming, 2: Resting, 3: Rising
    float m_timer;
    bool m_homeInitialized;
};
