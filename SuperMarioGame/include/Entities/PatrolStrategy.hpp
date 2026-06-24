#pragma once

#include "Entities/IMovementStrategy.hpp"

class PatrolStrategy : public IMovementStrategy {
public:
    explicit PatrolStrategy(bool ledgeAware = false, bool movingRight = false);
    virtual ~PatrolStrategy() override = default;

    bool isLedgeAware() const;
    void setLedgeAware(bool ledgeAware);
    bool isMovingRight() const;
    void setMovingRight(bool movingRight);

protected:
    void calculateTarget(Enemy& enemy, float dt) override;
    void applyMovement(Enemy& enemy, float dt) override;

private:
    bool m_ledgeAware;
    bool m_movingRight;
};
