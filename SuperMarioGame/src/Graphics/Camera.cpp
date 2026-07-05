#include "Graphics/Camera.hpp"
#include "Utils/Constants.hpp"
#include "Utils/MathUtils.hpp"
#include <algorithm>
#include <cstdlib>
#include <cmath>

Camera::Camera() {
    m_view.setSize(sf::Vector2f(Constants::WINDOW_WIDTH, Constants::WINDOW_HEIGHT));
    m_position = sf::Vector2f(Constants::WINDOW_WIDTH / 2.0f, Constants::WINDOW_HEIGHT / 2.0f);
    m_view.setCenter(m_position);
    m_bounds = AABB{0.0f, 0.0f, static_cast<float>(Constants::WINDOW_WIDTH), static_cast<float>(Constants::WINDOW_HEIGHT)};
    m_shakeIntensity = 0.0f;
    m_shakeDuration = 0.0f;
}

void Camera::follow(const sf::Vector2f& target, float dt) {
    // Smoothly interpolate current camera position towards the target coordinates (LERP)
    // 5.0f defines the tracking coefficient (higher means faster follow lag)
    m_position.x += (target.x - m_position.x) * 5.0f * dt;
    m_position.y += (target.y - m_position.y) * 5.0f * dt;
}

void Camera::setBounds(const AABB& bounds) {
    m_bounds = bounds;
}

sf::View& Camera::getView() {
    return m_view;
}

AABB Camera::getVisibleBounds() const {
    sf::Vector2f center = m_view.getCenter();
    sf::Vector2f size = m_view.getSize();
    return AABB{ center.x - size.x / 2.0f, center.y - size.y / 2.0f, size.x, size.y };
}

void Camera::triggerScreenShake(float intensity, float duration) {
    m_shakeIntensity = intensity;
    m_shakeDuration = duration;
}

void Camera::update(float dt) {
    float halfWidth = m_view.getSize().x / 2.0f;
    float halfHeight = m_view.getSize().y / 2.0f;

    // 1. Clamp logical camera center (m_position) to the level boundary limits
    float clampedX = m_position.x;
    if (m_bounds.width > m_view.getSize().x) {
        clampedX = MathUtils::clamp(m_position.x, m_bounds.x + halfWidth, m_bounds.x + m_bounds.width - halfWidth);
    } else {
        // Center view horizontally if map is smaller than view width
        clampedX = m_bounds.x + m_bounds.width / 2.0f;
    }

    float clampedY = m_position.y;
    if (m_bounds.height > m_view.getSize().y) {
        clampedY = MathUtils::clamp(m_position.y, m_bounds.y + halfHeight, m_bounds.y + m_bounds.height - halfHeight);
    } else {
        // Center view vertically if map is smaller than view height
        clampedY = m_bounds.y + m_bounds.height / 2.0f;
    }

    sf::Vector2f finalCenter(clampedX, clampedY);

    // 2. Add screen shake displacement offsets if shake duration is active
    if (m_shakeDuration > 0.0f) {
        m_shakeDuration -= dt;
        
        float randMaxF = static_cast<float>(RAND_MAX);
        float offsetX = (static_cast<float>(std::rand()) / randMaxF * 2.0f - 1.0f) * m_shakeIntensity;
        float offsetY = (static_cast<float>(std::rand()) / randMaxF * 2.0f - 1.0f) * m_shakeIntensity;
        
        finalCenter.x += offsetX;
        finalCenter.y += offsetY;
        
        if (m_shakeDuration <= 0.0f) {
            m_shakeDuration = 0.0f;
            m_shakeIntensity = 0.0f;
        }
    }

    // 3. Update the underlying SFML view center
    m_view.setCenter(finalCenter);
}
