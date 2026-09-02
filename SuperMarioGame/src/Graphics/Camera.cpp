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
    follow(target, sf::Vector2f{0.0f, 0.0f}, dt);
}

void Camera::follow(const sf::Vector2f& target, const sf::Vector2f& targetVelocity, float dt) {
    if (m_scrollMode == ScrollMode::Locked) {
        // The bounds are doing the work — a boss arena narrower than the view
        // centres itself through clampToBounds — so the camera must not chase.
        //
        // Do NOT reintroduce `m_position = clampToBounds(target)` here (384250f).
        // It was added to stop a stale m_position jumping the view when the mode
        // is released, but that is already handled at the release sites, which
        // snapTo() after restoring the bounds — PlayingState::releaseBossArena()
        // and setupTestScene(). Writing the target here instead makes Locked
        // follow the player exactly whenever the bounds are wider than the
        // arena, which is the opposite of what the mode is named for.
        return;
    }

    // Lookahead: bias the camera in the direction of travel, so the player sees
    // where they are going rather than where they have been. Scaled by how fast
    // they are actually moving, and eased rather than snapped — an offset that
    // flicks across on every turn is unreadable.
    float desiredLookahead = 0.0f;
    if (m_lookaheadStrength > 0.0f && Constants::RUN_SPEED > 0.0f) {
        const float normalised =
            MathUtils::clamp(targetVelocity.x / Constants::RUN_SPEED, -1.0f, 1.0f);
        desiredLookahead = normalised * m_lookaheadStrength;
    }
    m_lookaheadOffset += (desiredLookahead - m_lookaheadOffset) * 2.0f * dt;

    m_position.x += ((target.x + m_lookaheadOffset) - m_position.x) * 5.0f * dt;

    if (m_scrollMode == ScrollMode::Free) {
        m_position.y += (target.y - m_position.y) * 5.0f * dt;
    }
    // Horizontal mode leaves y where it is; clampToBounds still anchors the
    // ground line, which is what keeps a short sub-level looking right (C-1).
}

void Camera::setPosition(const sf::Vector2f& pos) {
    m_position = pos;
    m_view.setCenter(m_position);
}

void Camera::move(const sf::Vector2f& offset) {
    m_position += offset;
    m_view.setCenter(m_position);
}

sf::Vector2f Camera::getPosition() const {
    return m_position;
}

void Camera::setBoundsEnabled(bool enabled) {
    m_boundsEnabled = enabled;
}

bool Camera::isBoundsEnabled() const {
    return m_boundsEnabled;
}

void Camera::setBounds(const AABB& bounds) {
    m_bounds = bounds;
}

const AABB& Camera::getBounds() const {
    return m_bounds;
}

const sf::View& Camera::getView() const {
    return m_view;
}

void Camera::setViewSize(sf::Vector2f size) {
    m_view.setSize(size);
}

void Camera::setViewViewport(const sf::FloatRect& viewport) {
    m_view.setViewport(viewport);
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

    m_subscriptions.push_back(EventBus::ScopedSubscription(EventType::POWBlockHit, [this](const GameEvent&) {
        triggerScreenShake(6.0f, 0.30f, sf::Vector2f(0.0f, 1.0f), true);
    }));

    m_subscriptions.push_back(EventBus::ScopedSubscription(EventType::ThwompSlam, [this](const GameEvent&) {
        triggerScreenShake(5.0f, 0.25f, sf::Vector2f(0.0f, 1.0f), true);
    }));

    m_subscriptions.push_back(EventBus::ScopedSubscription(EventType::GroundPoundSlam, [this](const GameEvent&) {
        triggerScreenShake(4.0f, 0.20f, sf::Vector2f(0.0f, 1.0f), true);
    }));

    m_subscriptions.push_back(EventBus::ScopedSubscription(EventType::PlayerDamaged, [this](const GameEvent&) {
        triggerScreenShake(ShakePreset::Medium);
    }));

    m_subscriptions.push_back(EventBus::ScopedSubscription(EventType::BossDefeated, [this](const GameEvent&) {
        triggerScreenShake(8.0f, 0.60f, sf::Vector2f(0.0f, 0.0f), true);
    }));

    m_subscriptions.push_back(EventBus::ScopedSubscription(EventType::BlockBroken, [this](const GameEvent&) {
        triggerScreenShake(ShakePreset::Light);
    }));

    m_subscriptions.push_back(EventBus::ScopedSubscription(EventType::ScreenShakeTriggered, [this](const GameEvent& ev) {
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
    m_subscriptions.clear();
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

sf::Vector2f Camera::clampToBounds(sf::Vector2f center) const {
    if (!m_boundsEnabled) return center;

    const float halfWidth  = m_view.getSize().x / 2.0f;
    const float halfHeight = m_view.getSize().y / 2.0f;

    // Horizontal: standard clamp when the map is wider than the view.
    if (m_bounds.width > m_view.getSize().x) {
        center.x = MathUtils::clamp(center.x,
                                    m_bounds.x + halfWidth,
                                    m_bounds.x + m_bounds.width - halfWidth);
    } else {
        center.x = m_bounds.x + m_bounds.width / 2.0f;
    }

    // Vertical: same clamp when the map is taller than the view.
    if (m_bounds.height > m_view.getSize().y) {
        center.y = MathUtils::clamp(center.y,
                                    m_bounds.y + halfHeight,
                                    m_bounds.y + m_bounds.height - halfHeight);
    } else {
        // Map is SHORTER than the view, so some of the screen must fall outside
        // it — the only choice is which edge. Anchor the view's bottom to the
        // map's bottom: empty space above the level reads as sky, whereas empty
        // space below cuts the ground off and looks broken.
        //
        // Centring here instead put 40px of void above AND below every sub-level
        // (640px map vs 720px view) — audit C-1.
        center.y = m_bounds.y + m_bounds.height - halfHeight;
    }

    return center;
}

void Camera::update(float dt) {
    // Clamp first, then add shake, then clamp the result again. Shake used to be
    // applied after the only clamp, so every shake pushed the view outside the
    // map by up to its intensity (audit C-2).
    sf::Vector2f center = clampToBounds(m_position);
    center += calculateShakeOffset(dt);
    center = clampToBounds(center);

    m_view.setCenter(center);
}

void Camera::snapTo(const sf::Vector2f& target) {
    // Jump the camera without interpolation, keeping m_position authoritative.
    // Writing getView().setCenter() directly instead leaves m_position stale, so
    // the next update() snaps straight back (audit C-4).
    m_position = target;
    m_view.setCenter(clampToBounds(m_position));
}

