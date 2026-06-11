#pragma once

#include <SFML/Graphics/View.hpp>
#include "Physics/AABB.hpp"

class Camera {
public:
    Camera();
    ~Camera() = default;

    // Follow target, clamp within bounds
    void follow(const sf::Vector2f& target, float dt);
    void setBounds(const AABB& bounds);

    // Camera view modifiers
    sf::View& getView();
    AABB getVisibleBounds() const;

    // Visual camera modifiers
    void triggerScreenShake(float intensity, float duration);
    void update(float dt);

private:
    sf::View m_view;
    AABB m_bounds;
    sf::Vector2f m_position;

    float m_shakeIntensity = 0.0f;
    float m_shakeDuration = 0.0f;
};
