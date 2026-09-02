#pragma once

#include <string>
#include <SFML/System/Vector2.hpp>

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

    // The level has moved the enemy this strategy drives; move whatever fixed
    // point the strategy steers it towards by the same amount.
    //
    // A strategy that anchors itself somewhere — a piranha plant's pipe mouth, a
    // chain chomp's post — will otherwise drag the enemy straight back to the
    // old anchor on its next tick, undoing the move. Asked of the strategy
    // rather than cast for by the enemy, because which strategies hold an anchor
    // is the strategies' own business; a stateless one does nothing.
    virtual void translateAnchor(sf::Vector2f delta) { (void)delta; }

protected:
    // Hooks for concrete strategies to override
    virtual void calculateTarget(Enemy& enemy, float dt) {}
    virtual void applyMovement(Enemy& enemy, float dt) = 0;
    virtual void checkConstraints(Enemy& enemy, float dt) {}
};
