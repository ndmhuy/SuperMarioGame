#pragma once

#include <string>

class Enemy;

class IMovementStrategy {
public:
    virtual ~IMovementStrategy() = default;

    // Template Method: Defines the skeletal sequence of AI movement execution
    void execute(Enemy& enemy, float dt) {
        calculateTarget(enemy, dt);
        applyMovement(enemy, dt);
        checkConstraints(enemy, dt);
    }

    // Identification for the AI debug overlay (task 9.1). Defaulted rather than
    // pure so a strategy is never forced to implement debug plumbing, but every
    // shipped one names itself.
    virtual std::string getName() const { return "Strategy"; }
    // Whatever internal state is worth watching, or "" for a stateless strategy.
    virtual std::string getDebugState() const { return ""; }

protected:
    // Hooks for concrete strategies to override
    virtual void calculateTarget(Enemy& enemy, float dt) {}
    virtual void applyMovement(Enemy& enemy, float dt) = 0;
    virtual void checkConstraints(Enemy& enemy, float dt) {}
};
