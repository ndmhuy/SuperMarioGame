#pragma once

#include "Entities/IMovementStrategy.hpp"
#include <SFML/System/Vector2.hpp>
#include <string>

// Thwomp's state machine: wait on the ceiling, slam when something walks under,
// sit on the floor, climb back up (task 9.1, "Idle -> Slam -> Rise").
//
// It was a bare int with the four values only described in comments, which is
// why Thwomp picked its own sprite from `position.y > 140.0f` instead of asking
// the machine what it was doing.
enum class ProximityState {
    Idle,      // parked at home, watching for the player
    Slamming,  // falling fast
    Resting,   // landed, briefly inert
    Rising     // climbing back to home
};

class ProximityTriggerStrategy : public IMovementStrategy {
public:
    explicit ProximityTriggerStrategy(sf::Vector2f homePos = sf::Vector2f(0.f, 0.f));
    virtual ~ProximityTriggerStrategy() override = default;

    std::string getName() const override { return "ProximityTrigger"; }
    std::string getDebugState() const override;

    sf::Vector2f getHomePos() const;
    void setHomePos(sf::Vector2f homePos);
    ProximityState getState() const;
    void setState(ProximityState state);

protected:
    void calculateTarget(Enemy& enemy, float dt) override;
    void applyMovement(Enemy& enemy, float dt) override;
    void checkConstraints(Enemy& enemy, float dt) override;

private:
    sf::Vector2f m_homePos;
    ProximityState m_state;
    float m_timer;
    bool m_homeInitialized;
};
