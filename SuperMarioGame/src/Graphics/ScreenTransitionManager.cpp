#include "Graphics/ScreenTransitionManager.hpp"
#include "Utils/Constants.hpp"
#include "Utils/MathUtils.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

ScreenTransitionManager& ScreenTransitionManager::getInstance() {
    static ScreenTransitionManager instance;
    return instance;
}

void ScreenTransitionManager::startTransition(TransitionType type,
                                             float duration,
                                             Callback onMidpoint,
                                             Callback onComplete,
                                             sf::Vector2f focalPoint,
                                             sf::Color overlayColor) {
    m_type = type;
    m_duration = std::max(0.01f, duration);
    m_elapsedTime = 0.0f;
    m_focalPoint = focalPoint;
    m_color = overlayColor;
    m_onMidpoint = std::move(onMidpoint);
    m_onComplete = std::move(onComplete);
    m_midpointTriggered = false;

    switch (m_type) {
        case TransitionType::FadeOut:
        case TransitionType::CircleWipeOut:
            m_state = TransitionState::Out;
            break;
        case TransitionType::FadeIn:
        case TransitionType::CircleWipeIn:
            m_state = TransitionState::In;
            break;
        case TransitionType::PipeTransition:
            m_state = TransitionState::Out;
            break;
        case TransitionType::None:
        default:
            m_state = TransitionState::Idle;
            if (m_onComplete) m_onComplete();
            break;
    }
}

void ScreenTransitionManager::fadeOut(float duration, Callback onComplete, sf::Color color) {
    startTransition(TransitionType::FadeOut, duration, nullptr, std::move(onComplete), {640.0f, 360.0f}, color);
}

void ScreenTransitionManager::fadeIn(float duration, Callback onComplete, sf::Color color) {
    startTransition(TransitionType::FadeIn, duration, nullptr, std::move(onComplete), {640.0f, 360.0f}, color);
}

void ScreenTransitionManager::circleWipeOut(float duration, sf::Vector2f center, Callback onMidpoint, Callback onComplete) {
    startTransition(TransitionType::CircleWipeOut, duration, std::move(onMidpoint), std::move(onComplete), center, sf::Color::Black);
}

void ScreenTransitionManager::circleWipeIn(float duration, sf::Vector2f center, Callback onComplete) {
    startTransition(TransitionType::CircleWipeIn, duration, nullptr, std::move(onComplete), center, sf::Color::Black);
}

void ScreenTransitionManager::pipeWipe(float duration, sf::Vector2f pipePos, Callback onMidpoint, Callback onComplete) {
    startTransition(TransitionType::PipeTransition, duration, std::move(onMidpoint), std::move(onComplete), pipePos, sf::Color::Black);
}

void ScreenTransitionManager::reset() {
    m_type = TransitionType::None;
    m_state = TransitionState::Idle;
    m_elapsedTime = 0.0f;
    m_duration = 0.0f;
    m_midpointTriggered = false;
    m_onMidpoint = nullptr;
    m_onComplete = nullptr;
}

bool ScreenTransitionManager::isTransitioning() const {
    return m_state != TransitionState::Idle && m_state != TransitionState::Completed;
}

TransitionState ScreenTransitionManager::getState() const {
    return m_state;
}

float ScreenTransitionManager::getProgress() const {
    if (m_duration <= 0.0f) return 1.0f;

    if (m_type == TransitionType::PipeTransition) {
        float half = m_duration * 0.5f;
        if (m_state == TransitionState::Out || (m_state == TransitionState::Midpoint && !m_midpointTriggered)) {
            return std::min(1.0f, m_elapsedTime / half);
        } else {
            return std::min(1.0f, (m_elapsedTime - half) / half);
        }
    } else if (m_type == TransitionType::CircleWipeOut && m_onMidpoint && m_onComplete) {
        // Two-phase circle wipe out -> in
        float half = m_duration * 0.5f;
        if (m_state == TransitionState::Out) {
            return std::min(1.0f, m_elapsedTime / half);
        } else {
            return std::min(1.0f, (m_elapsedTime - half) / half);
        }
    }

    return std::min(1.0f, m_elapsedTime / m_duration);
}

void ScreenTransitionManager::update(float dt) {
    if (!isTransitioning()) return;

    m_elapsedTime += dt;

    if (m_type == TransitionType::PipeTransition || (m_type == TransitionType::CircleWipeOut && m_onMidpoint && m_onComplete)) {
        float halfDuration = m_duration * 0.5f;

        if (m_state == TransitionState::Out && m_elapsedTime >= halfDuration) {
            m_state = TransitionState::Midpoint;
            if (!m_midpointTriggered) {
                m_midpointTriggered = true;
                if (m_onMidpoint) {
                    m_onMidpoint();
                }
            }
            m_state = TransitionState::In;
        } else if (m_state == TransitionState::In && m_elapsedTime >= m_duration) {
            m_state = TransitionState::Completed;
            Callback cb = std::move(m_onComplete);
            m_state = TransitionState::Idle;
            m_type = TransitionType::None;
            if (cb) {
                cb();
            }
        }
    } else {
        // Single-phase transition
        if (m_elapsedTime >= m_duration) {
            m_state = TransitionState::Midpoint;
            if (!m_midpointTriggered) {
                m_midpointTriggered = true;
                if (m_onMidpoint) {
                    m_onMidpoint();
                }
            }

            m_state = TransitionState::Completed;
            Callback cb = std::move(m_onComplete);
            m_state = TransitionState::Idle;
            m_type = TransitionType::None;
            if (cb) {
                cb();
            }
        }
    }
}

void ScreenTransitionManager::render(sf::RenderTarget& target) {
    if (m_state == TransitionState::Idle && m_type == TransitionType::None) return;

    // Direct render in screen space (default view)
    sf::View origView = target.getView();
    target.setView(target.getDefaultView());

    float progress = getProgress();
    // Smooth cubic easing: t_eased = 3*t^2 - 2*t^3
    float easedProgress = 3.0f * progress * progress - 2.0f * progress * progress * progress;

    switch (m_type) {
        case TransitionType::FadeOut:
            renderFade(target, easedProgress);
            break;
        case TransitionType::FadeIn:
            renderFade(target, 1.0f - easedProgress);
            break;
        case TransitionType::CircleWipeOut:
            if (m_onMidpoint && m_onComplete) {
                if (m_state == TransitionState::Out) {
                    renderCircleWipe(target, 1.0f - easedProgress, false);
                } else {
                    renderCircleWipe(target, easedProgress, false);
                }
            } else {
                renderCircleWipe(target, 1.0f - easedProgress, false);
            }
            break;
        case TransitionType::CircleWipeIn:
            renderCircleWipe(target, easedProgress, false);
            break;
        case TransitionType::PipeTransition:
            if (m_state == TransitionState::Out) {
                renderCircleWipe(target, 1.0f - easedProgress, false);
            } else {
                renderCircleWipe(target, easedProgress, false);
            }
            break;
        default:
            break;
    }

    target.setView(origView);
}

void ScreenTransitionManager::renderFade(sf::RenderTarget& target, float alphaProgress) {
    sf::Vector2f viewSize = target.getView().getSize();
    sf::RectangleShape overlay(viewSize);
    overlay.setPosition({0.0f, 0.0f});

    std::uint8_t alpha = static_cast<std::uint8_t>(MathUtils::clamp(alphaProgress, 0.0f, 1.0f) * 255.0f);
    sf::Color color = m_color;
    color.a = alpha;
    overlay.setFillColor(color);

    target.draw(overlay);
}

void ScreenTransitionManager::renderCircleWipe(sf::RenderTarget& target, float radiusProgress, bool invert) {
    sf::Vector2f viewSize = target.getView().getSize();
    float maxRadius = std::sqrt(viewSize.x * viewSize.x + viewSize.y * viewSize.y);
    float currentRadius = maxRadius * MathUtils::clamp(radiusProgress, 0.0f, 1.0f);

    if (currentRadius <= 0.5f) {
        // 100% covered screen
        sf::RectangleShape fullRect(viewSize);
        fullRect.setPosition({0.0f, 0.0f});
        fullRect.setFillColor(m_color);
        target.draw(fullRect);
        return;
    }

    if (currentRadius >= maxRadius) {
        // 100% open, draw nothing
        return;
    }

    // Build geometry ring mask around m_focalPoint
    const int segments = 32;
    sf::VertexArray ring(sf::PrimitiveType::TriangleStrip);

    float outerRadius = maxRadius * 2.0f;

    for (int i = 0; i <= segments; ++i) {
        float angle = (2.0f * M_PI * i) / segments;
        float cosA = std::cos(angle);
        float sinA = std::sin(angle);

        sf::Vector2f outerPos = m_focalPoint + sf::Vector2f(outerRadius * cosA, outerRadius * sinA);
        sf::Vector2f innerPos = m_focalPoint + sf::Vector2f(currentRadius * cosA, currentRadius * sinA);

        sf::Vertex vOuter;
        vOuter.position = outerPos;
        vOuter.color = m_color;

        sf::Vertex vInner;
        vInner.position = innerPos;
        vInner.color = m_color;

        ring.append(vOuter);
        ring.append(vInner);
    }

    target.draw(ring);
}
