#pragma once

#include "Entities/Character.hpp"
#include "Entities/IMovementStrategy.hpp"
#include <memory>

class Enemy : public Character {
public:
    Enemy(sf::Vector2f position, int scoreValue = 100);
    virtual ~Enemy() override = default;

    // Overrides Entity lifecycle
    void update(float dt) override;
    void render(sf::RenderTarget& target) override;

    // Strategy setters/getters
    void setStrategy(std::unique_ptr<IMovementStrategy> strategy);
    IMovementStrategy* getStrategy() const;

    // Virtual interaction handlers
    virtual void onStomped() = 0;
    virtual void onHitByFireball() = 0;

    // Score properties
    int getScoreValue() const;
    void setScoreValue(int value);

protected:
    std::unique_ptr<IMovementStrategy> m_aiStrategy;
    int m_scoreValue;
};

