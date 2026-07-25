#pragma once

#include "Entities/IMovementStrategy.hpp"
#include <SFML/System/Vector2.hpp>

enum class EmergenceState {
    Retracted,
    Emerging,
    Emerged,
    Retreating
};

class TimerEmergenceStrategy : public IMovementStrategy {
public:
    explicit TimerEmergenceStrategy(sf::Vector2f anchorPos = sf::Vector2f(0.f, 0.f));
    virtual ~TimerEmergenceStrategy() override = default;

    sf::Vector2f getAnchorPos() const;
    void setAnchorPos(sf::Vector2f anchorPos);

protected:
    void calculateTarget(Enemy& enemy, float dt) override;
    void applyMovement(Enemy& enemy, float dt) override;

private:
    float m_timer;
    sf::Vector2f m_anchorPos;
    bool m_anchorInitialized;
    EmergenceState m_state;
};
