#pragma once

#include <SFML/Graphics/View.hpp>
#include <vector>
#include <functional>
#include "Physics/AABB.hpp"
#include "Core/EventBus.hpp"

enum class ShakePreset {
    Light,   // 2.0px offset, 0.10s duration
    Medium,  // 4.0px offset, 0.20s duration
    Heavy,   // 6.0px offset, 0.30s duration
    Custom   // Custom params
};

struct ShakeParams {
    float intensity = 0.0f;                // Peak amplitude in pixels
    float duration = 0.0f;                 // Total duration in seconds
    sf::Vector2f direction = {0.0f, 0.0f};  // Direction vector (0,0 = omnidirectional)
    bool useDecay = true;                  // Smooth linear/exponential decay
    float frequency = 30.0f;               // Oscillation frequency in Hz
};

class Camera {
public:
    Camera();
    ~Camera();

    // Prevent copying, allow moving
    Camera(const Camera&) = delete;
    Camera& operator=(const Camera&) = delete;
    Camera(Camera&&) noexcept = default;
    Camera& operator=(Camera&&) noexcept = default;

    // Follow target & set bounds
    void follow(const sf::Vector2f& target, float dt);
    void setBounds(const AABB& bounds);

    // View & bounds accessors
    sf::View& getView();
    AABB getVisibleBounds() const;

    // Position & Bounds Control
    void setPosition(const sf::Vector2f& pos);
    void move(const sf::Vector2f& offset);
    sf::Vector2f getPosition() const;
    void setBoundsEnabled(bool enabled);
    bool isBoundsEnabled() const;

    // Jump straight to a target, respecting bounds. Use this for respawns, level
    // loads and rewind restores — writing getView().setCenter() directly leaves
    // m_position stale and the next update() undoes it (audit C-4).
    void snapTo(const sf::Vector2f& target);

    // Where the view centre would sit for a given target, after clamping.
    // Exposed so tests can assert the clamp without driving a frame.
    sf::Vector2f clampToBounds(sf::Vector2f center) const;

    // Screen Shake API
    void triggerScreenShake(ShakePreset preset);
    void triggerScreenShake(float intensity, float duration, sf::Vector2f direction = {0.0f, 0.0f}, bool useDecay = true);
    void triggerScreenShake(const ShakeParams& params);

    // Shake status accessors
    bool isShaking() const;
    const ShakeParams& getActiveShakeParams() const;
    float getShakeRemainingTime() const;
    float getShakeElapsedTime() const;

    // EventBus Integration
    void subscribeToEvents();
    void unsubscribeFromEvents();

    // Engine loop update
    void update(float dt);

private:
    sf::View m_view;
    AABB m_bounds;
    sf::Vector2f m_position;
    bool m_boundsEnabled = true;

    // Active shake runtime state
    ShakeParams m_activeShake;
    float m_shakeTimer = 0.0f;
    float m_shakeElapsedTime = 0.0f;

    // EventBus subscriptions tracking
    std::vector<EventBus::SubscriptionId> m_subscriptionIds;
    bool m_subscribedToEvents = false;

    // Internal offset generator
    sf::Vector2f calculateShakeOffset(float dt);
};

