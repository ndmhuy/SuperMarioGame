#pragma once

#include <SFML/Graphics.hpp>

class SpriteTransformAnim {
public:
    enum class EasingType {
        Linear,
        EaseInOut,
        ElasticPulse
    };

    SpriteTransformAnim() = default;

    // Start scale transition animation with floor anchoring origin
    void startScaleAnim(float startScale, float targetScale, float duration, EasingType easing = EasingType::EaseInOut);

    // Start continuous rotation spin animation (e.g. Star Kill spin)
    void startRotationSpin(float spinSpeedDegreesPerSec);

    // Stop rotation spin
    void stopRotationSpin() { m_spinSpeed = 0.0f; m_currentRotation = 0.0f; }

    // Update timer & calculate current transformations
    void update(float dt);

    // Apply scale & rotation to target sprite while preserving bottom-center origin
    void applyToSprite(sf::Sprite& sprite, bool facingRight = true);

    // Accessors
    float getCurrentScale() const { return m_currentScale; }
    float getCurrentRotation() const { return m_currentRotation; }
    bool isFinished() const { return !m_scaleActive; }
    bool isFormFlickering() const { return m_scaleActive; }

private:
    float m_startScale = 1.0f;
    float m_targetScale = 1.0f;
    float m_currentScale = 1.0f;
    float m_duration = 0.0f;
    float m_elapsedTime = 0.0f;
    EasingType m_easing = EasingType::EaseInOut;
    bool m_scaleActive = false;

    float m_spinSpeed = 0.0f;
    float m_currentRotation = 0.0f;
};
