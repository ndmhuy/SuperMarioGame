#include "Graphics/Camera.hpp"
#include "Utils/Constants.hpp"
#include "Utils/MathUtils.hpp"
#include <algorithm>
#include <cstdlib>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

Camera::Camera() {
    m_view.setSize(sf::Vector2f(Constants::WINDOW_WIDTH, Constants::WINDOW_HEIGHT));
    m_position = sf::Vector2f(Constants::WINDOW_WIDTH / 2.0f, Constants::WINDOW_HEIGHT / 2.0f);
    m_view.setCenter(m_position);
    m_bounds = AABB{0.0f, 0.0f, static_cast<float>(Constants::WINDOW_WIDTH), static_cast<float>(Constants::WINDOW_HEIGHT)};
    m_shakeTimer = 0.0f;
    m_shakeElapsedTime = 0.0f;
    subscribeToEvents();
}

Camera::~Camera() {
    unsubscribeFromEvents();
}

void Camera::follow(const sf::Vector2f& target, float dt) {
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

void Camera::triggerScreenShake(ShakePreset preset) {
    ShakeParams params;
    switch (preset) {
        case ShakePreset::Light:
            params.intensity = 2.0f;
            params.duration = 0.10f;
            params.direction = {0.0f, 0.0f};
            params.useDecay = true;
            params.frequency = 30.0f;
            break;
        case ShakePreset::Medium:
            params.intensity = 4.0f;
            params.duration = 0.20f;
            params.direction = {0.0f, 0.0f};
            params.useDecay = true;
            params.frequency = 30.0f;
            break;
        case ShakePreset::Heavy:
            params.intensity = 6.0f;
            params.duration = 0.30f;
            params.direction = {0.0f, 0.0f};
            params.useDecay = true;
            params.frequency = 30.0f;
            break;
        case ShakePreset::Custom:
            params.intensity = 4.0f;
            params.duration = 0.20f;
            break;
    }
    triggerScreenShake(params);
}

void Camera::triggerScreenShake(float intensity, float duration, sf::Vector2f direction, bool useDecay) {
    ShakeParams params;
    params.intensity = intensity;
    params.duration = duration;
    params.direction = direction;
    params.useDecay = useDecay;
    params.frequency = 30.0f;
    triggerScreenShake(params);
}

void Camera::triggerScreenShake(const ShakeParams& params) {
    m_activeShake = params;
    m_shakeTimer = params.duration;
    m_shakeElapsedTime = 0.0f;
}

bool Camera::isShaking() const {
    return m_shakeTimer > 0.0f;
}

const ShakeParams& Camera::getActiveShakeParams() const {
    return m_activeShake;
}

float Camera::getShakeRemainingTime() const {
    return std::max(0.0f, m_shakeTimer);
}

float Camera::getShakeElapsedTime() const {
    return m_shakeElapsedTime;
}

void Camera::subscribeToEvents() {
    if (m_subscribedToEvents) return;

    EventBus& bus = EventBus::getInstance();

    m_subscriptionIds.push_back(bus.subscribe(EventType::POWBlockHit, [this](const GameEvent&) {
        triggerScreenShake(6.0f, 0.30f, sf::Vector2f(0.0f, 1.0f), true);
    }));

    m_subscriptionIds.push_back(bus.subscribe(EventType::ThwompSlam, [this](const GameEvent&) {
        triggerScreenShake(5.0f, 0.25f, sf::Vector2f(0.0f, 1.0f), true);
    }));

    m_subscriptionIds.push_back(bus.subscribe(EventType::GroundPoundSlam, [this](const GameEvent&) {
        triggerScreenShake(4.0f, 0.20f, sf::Vector2f(0.0f, 1.0f), true);
    }));

    m_subscriptionIds.push_back(bus.subscribe(EventType::PlayerDamaged, [this](const GameEvent&) {
        triggerScreenShake(ShakePreset::Medium);
    }));

    m_subscriptionIds.push_back(bus.subscribe(EventType::BossDefeated, [this](const GameEvent&) {
        triggerScreenShake(8.0f, 0.60f, sf::Vector2f(0.0f, 0.0f), true);
    }));

    m_subscriptionIds.push_back(bus.subscribe(EventType::BlockBroken, [this](const GameEvent&) {
        triggerScreenShake(ShakePreset::Light);
    }));

    m_subscriptionIds.push_back(bus.subscribe(EventType::ScreenShakeTriggered, [this](const GameEvent& ev) {
        if (ev.data.has_value()) {
            try {
                if (ev.data.type() == typeid(ShakeParams)) {
                    triggerScreenShake(std::any_cast<ShakeParams>(ev.data));
                    return;
                } else if (ev.data.type() == typeid(ShakePreset)) {
                    triggerScreenShake(std::any_cast<ShakePreset>(ev.data));
                    return;
                }
            } catch (...) {}
        }
        triggerScreenShake(ShakePreset::Medium);
    }));

    m_subscribedToEvents = true;
}

void Camera::unsubscribeFromEvents() {
    if (!m_subscribedToEvents) return;
    EventBus& bus = EventBus::getInstance();
    for (auto id : m_subscriptionIds) {
        bus.unsubscribe(id);
    }
    m_subscriptionIds.clear();
    m_subscribedToEvents = false;
}

sf::Vector2f Camera::calculateShakeOffset(float dt) {
    if (m_shakeTimer <= 0.0f || m_activeShake.duration <= 0.0f) {
        return {0.0f, 0.0f};
    }

    m_shakeTimer -= dt;
    m_shakeElapsedTime += dt;

    if (m_shakeTimer <= 0.0f) {
        m_shakeTimer = 0.0f;
        return {0.0f, 0.0f};
    }

    float decayFactor = 1.0f;
    if (m_activeShake.useDecay) {
        decayFactor = std::max(0.0f, m_shakeTimer / m_activeShake.duration);
    }

    float currentIntensity = m_activeShake.intensity * decayFactor;

    sf::Vector2f offset(0.0f, 0.0f);

    float dirLen = std::sqrt(m_activeShake.direction.x * m_activeShake.direction.x +
                             m_activeShake.direction.y * m_activeShake.direction.y);

    if (dirLen > 0.001f) {
        sf::Vector2f normDir(m_activeShake.direction.x / dirLen, m_activeShake.direction.y / dirLen);
        float osc = std::sin(2.0f * M_PI * m_activeShake.frequency * m_shakeElapsedTime);
        offset = normDir * (osc * currentIntensity);
    } else {
        float randMaxF = static_cast<float>(RAND_MAX);
        float rx = (static_cast<float>(std::rand()) / randMaxF * 2.0f - 1.0f);
        float ry = (static_cast<float>(std::rand()) / randMaxF * 2.0f - 1.0f);
        offset = sf::Vector2f(rx * currentIntensity, ry * currentIntensity);
    }

    return offset;
}

void Camera::update(float dt) {
    float halfWidth = m_view.getSize().x / 2.0f;
    float halfHeight = m_view.getSize().y / 2.0f;

    float clampedX = m_position.x;
    if (m_bounds.width > m_view.getSize().x) {
        clampedX = MathUtils::clamp(m_position.x, m_bounds.x + halfWidth, m_bounds.x + m_bounds.width - halfWidth);
    } else {
        clampedX = m_bounds.x + m_bounds.width / 2.0f;
    }

    float clampedY = m_position.y;
    if (m_bounds.height > m_view.getSize().y) {
        clampedY = MathUtils::clamp(m_position.y, m_bounds.y + halfHeight, m_bounds.y + m_bounds.height - halfHeight);
    } else {
        clampedY = m_bounds.y + m_bounds.height / 2.0f;
    }

    sf::Vector2f finalCenter(clampedX, clampedY);

    sf::Vector2f shakeOffset = calculateShakeOffset(dt);
    finalCenter += shakeOffset;

    m_view.setCenter(finalCenter);
}

