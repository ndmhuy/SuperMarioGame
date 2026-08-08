#include "Graphics/SpriteTransformAnim.hpp"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

void SpriteTransformAnim::startScaleAnim(float startScale, float targetScale, float duration, EasingType easing) {
    m_startScale = startScale;
    m_targetScale = targetScale;
    m_currentScale = startScale;
    m_duration = duration;
    m_elapsedTime = 0.0f;
    m_easing = easing;
    m_scaleActive = true;
}

void SpriteTransformAnim::startRotationSpin(float spinSpeedDegreesPerSec) {
    m_spinSpeed = spinSpeedDegreesPerSec;
}

void SpriteTransformAnim::update(float dt) {
    // Rotation update
    if (m_spinSpeed != 0.0f) {
        m_currentRotation += m_spinSpeed * dt;
        m_currentRotation = std::fmod(m_currentRotation, 360.0f);
        if (m_currentRotation < 0.0f) m_currentRotation += 360.0f;
    }

    // Scale lerp update
    if (m_scaleActive) {
        m_elapsedTime += dt;
        if (m_elapsedTime >= m_duration) {
            m_elapsedTime = m_duration;
            m_currentScale = m_targetScale;
            m_scaleActive = false;
        } else {
            float t = m_elapsedTime / m_duration;
            float factor = t;

            switch (m_easing) {
                case EasingType::Linear:
                    factor = t;
                    break;
                case EasingType::EaseInOut:
                    factor = t < 0.5f ? 2.0f * t * t : 1.0f - std::pow(-2.0f * t + 2.0f, 2.0f) / 2.0f;
                    break;
                case EasingType::ElasticPulse:
                    factor = std::sin(t * M_PI * 4.0f) * (1.0f - t) * 0.2f + t;
                    break;
            }

            m_currentScale = m_startScale + (m_targetScale - m_startScale) * factor;
        }
    }
}

void SpriteTransformAnim::applyToSprite(sf::Sprite& sprite, bool facingRight) {
    sf::FloatRect bounds = sprite.getLocalBounds();
    if (bounds.size.x == 0.0f || bounds.size.y == 0.0f) return;

    // Floor anchoring origin: feet bottom-center (width / 2, height)
    sprite.setOrigin(sf::Vector2f(bounds.size.x / 2.0f, bounds.size.y));

    float dirScale = facingRight ? m_currentScale : -m_currentScale;
    sprite.setScale(sf::Vector2f(dirScale, m_currentScale));
    sprite.setRotation(sf::degrees(m_currentRotation));
}
