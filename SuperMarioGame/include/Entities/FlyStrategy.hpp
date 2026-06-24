#pragma once

#include "Entities/IMovementStrategy.hpp"

enum class FlyMode {
    SinusoidalPatrol = 0,
    VerticalBounce = 1,
    FollowPlayer = 2
};

class FlyStrategy : public IMovementStrategy {
public:
    explicit FlyStrategy(FlyMode mode = FlyMode::SinusoidalPatrol, bool movingRight = false);
    virtual ~FlyStrategy() override = default;

    FlyMode getFlyMode() const;
    void setFlyMode(FlyMode mode);

protected:
    void calculateTarget(Enemy& enemy, float dt) override;
    void applyMovement(Enemy& enemy, float dt) override;

private:
    FlyMode m_flyMode;
    float m_timer;
    float m_amplitude;
    float m_frequency;
    float m_baseY;
    bool m_baseYInitialized;
    bool m_movingRight;
};
