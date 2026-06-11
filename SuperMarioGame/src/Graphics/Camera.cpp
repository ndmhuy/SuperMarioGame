#include "Graphics/Camera.hpp"

Camera::Camera() {
    // TODO: Implement by hand
}

void Camera::follow(const sf::Vector2f& target, float dt) {
    // TODO: Implement by hand
}

void Camera::setBounds(const AABB& bounds) {
    // TODO: Implement by hand
    m_bounds = bounds;
}

sf::View& Camera::getView() {
    // TODO: Implement by hand
    return m_view;
}

AABB Camera::getVisibleBounds() const {
    // TODO: Implement by hand
    return m_bounds;
}

void Camera::triggerScreenShake(float intensity, float duration) {
    // TODO: Implement by hand
}

void Camera::update(float dt) {
    // TODO: Implement by hand
}
