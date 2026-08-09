#pragma once

#include <SFML/Graphics.hpp>
#include <functional>

enum class TransitionType {
    None,
    FadeIn,           // Screen transitions from dark overlay to full transparency
    FadeOut,          // Screen transitions from full transparency to dark overlay
    CircleWipeIn,     // Circular iris expands from focal point to reveal screen
    CircleWipeOut,    // Circular iris shrinks to focal point to cover screen
    PipeTransition    // Mario pipe transition (iris shrink + expand at pipe location)
};

enum class TransitionState {
    Idle,
    Out,              // Obscuring screen (e.g. fading to dark/covering)
    Midpoint,         // Screen 100% obscured, midpoint callback executed
    In,               // Revealing screen (e.g. fading to clear/uncovering)
    Completed         // Transition completed, state reset to Idle
};

class ScreenTransitionManager {
public:
    using Callback = std::function<void()>;

    static ScreenTransitionManager& getInstance();

    // Prevent copying / moving for Singleton
    ScreenTransitionManager(const ScreenTransitionManager&) = delete;
    ScreenTransitionManager& operator=(const ScreenTransitionManager&) = delete;
    ScreenTransitionManager(ScreenTransitionManager&&) = delete;
    ScreenTransitionManager& operator=(ScreenTransitionManager&&) = delete;

    // Start transition with callbacks
    void startTransition(TransitionType type,
                         float duration,
                         Callback onMidpoint = nullptr,
                         Callback onComplete = nullptr,
                         sf::Vector2f focalPoint = sf::Vector2f(640.0f, 360.0f),
                         sf::Color overlayColor = sf::Color::Black);

    // Convenience API helpers
    void fadeOut(float duration, Callback onComplete = nullptr, sf::Color color = sf::Color::Black);
    void fadeIn(float duration, Callback onComplete = nullptr, sf::Color color = sf::Color::Black);
    void circleWipeOut(float duration, sf::Vector2f center, Callback onMidpoint = nullptr, Callback onComplete = nullptr);
    void circleWipeIn(float duration, sf::Vector2f center, Callback onComplete = nullptr);
    void pipeWipe(float duration, sf::Vector2f pipePos, Callback onMidpoint = nullptr, Callback onComplete = nullptr);

    // Game loop updates
    void update(float dt);
    void render(sf::RenderTarget& target);

    // Query state
    bool isTransitioning() const;
    TransitionState getState() const;
    float getProgress() const;

    // Reset transition system state
    void reset();

private:
    ScreenTransitionManager() = default;
    ~ScreenTransitionManager() = default;

    TransitionType m_type = TransitionType::None;
    TransitionState m_state = TransitionState::Idle;

    float m_duration = 0.0f;
    float m_elapsedTime = 0.0f;
    sf::Vector2f m_focalPoint = {640.0f, 360.0f};
    sf::Color m_color = sf::Color::Black;

    Callback m_onMidpoint = nullptr;
    Callback m_onComplete = nullptr;

    bool m_midpointTriggered = false;

    // Direct Vertex Geometry Rendering (No Shader dependency)
    void renderFade(sf::RenderTarget& target, float alphaProgress);
    void renderCircleWipe(sf::RenderTarget& target, float radiusProgress, bool invert);
};
