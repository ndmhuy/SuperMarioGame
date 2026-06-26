#pragma once

#include "Physics/AABB.hpp"
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/System/Vector2.hpp>

class PhysicsEngine;
class CollisionResolver;

class Entity {
public:
    Entity() = default;
    virtual ~Entity() = default;

    // Pure virtual lifecycle methods
    virtual void update(float dt) = 0;
    virtual void render(sf::RenderTarget& target) = 0;

    // Virtual physics/state queries
    virtual const AABB& getBoundingBox() const;
    virtual bool isActive() const;
    virtual void destroy();

    // Getters for external read-only access
    sf::Vector2f getPosition() const;
    sf::Vector2f getVelocity() const;

    // Setters for coordinates
    void setPosition(sf::Vector2f pos);
    void setVelocity(sf::Vector2f vel);

protected:
    // Friends are allowed direct write access to coordinate updates
    friend class PhysicsEngine;
    friend class CollisionResolver;
    friend class IMovementStrategy;
    friend class PatrolStrategy;
    friend class ChaseStrategy;
    friend class FlyStrategy;
    friend class TimerEmergenceStrategy;
    friend class LinearStrategy;
    friend class HammerThrowStrategy;
    friend class TetheredChaseStrategy;
    friend class ProximityTriggerStrategy;
    friend int main();

    sf::Vector2f position;
    sf::Vector2f velocity;
    bool active = true;
    AABB boundingBox;
};

