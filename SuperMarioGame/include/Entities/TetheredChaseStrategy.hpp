#pragma once

#include "Entities/IMovementStrategy.hpp"
#include <SFML/System/Vector2.hpp>

class TetheredChaseStrategy : public IMovementStrategy {
public:
    explicit TetheredChaseStrategy(sf::Vector2f anchorPos = sf::Vector2f(0.f, 0.f), float tetherRadius = 128.0f);
    virtual ~TetheredChaseStrategy() override = default;

    sf::Vector2f getAnchorPos() const;
    void setAnchorPos(sf::Vector2f anchorPos);
    float getTetherRadius() const;
    void setTetherRadius(float radius);

protected:
    void calculateTarget(Enemy& enemy, float dt) override;
    void applyMovement(Enemy& enemy, float dt) override;
    void checkConstraints(Enemy& enemy, float dt) override;

private:
    sf::Vector2f m_anchorPos;
    float m_tetherRadius;
    float m_timer;
    bool m_anchorInitialized;
    bool m_isLunging;
    sf::Vector2f m_lungeDir;
};
