#pragma once

#include "Entities/IMovementStrategy.hpp"
#include <string>

class ChaseStrategy : public IMovementStrategy {
public:
    ChaseStrategy() = default;
    virtual ~ChaseStrategy() override = default;

    std::string getName() const override { return "Chase"; }

protected:
    void calculateTarget(Enemy& enemy, float dt) override;
    void applyMovement(Enemy& enemy, float dt) override;

private:
    bool m_shouldChase = false;
    float m_targetDx = 0.0f;
    float m_targetDy = 0.0f;
    float m_targetDist = 0.0f;
};
