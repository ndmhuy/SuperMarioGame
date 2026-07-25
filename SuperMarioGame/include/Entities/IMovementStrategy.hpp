#pragma once

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

protected:
    // Hooks for concrete strategies to override
    virtual void calculateTarget(Enemy& enemy, float dt) {}
    virtual void applyMovement(Enemy& enemy, float dt) = 0;
    virtual void checkConstraints(Enemy& enemy, float dt) {}
};
