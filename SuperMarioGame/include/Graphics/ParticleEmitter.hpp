#pragma once

#include "ParticleSystem.hpp"

enum class ParticleType {
    BrickBreak, 
    CoinSparkle, 
    DeathPoof, 
    Stomp, 
    Combo, 
    WallDust, 
    WaterBubble, 
    LavaEmber
};

struct EmitterSettings {
    sf::Vector2f minVelocity;
    sf::Vector2f maxVelocity;
    sf::Vector2f acceleration;
    sf::Color startColor = sf::Color::White;
    sf::Color endColor = sf::Color::Transparent;
    float minLifetime = 0.5f;
    float maxLifetime = 1.0f;
    float startScale = 1.0f;
    float endScale = 1.0f;
    float emissionRate = 0.0f; // Particles per second (0 = burst only)
    sf::IntRect textureRect;
};

class ParticleEmitter {
public:
    ParticleEmitter() = default;
    explicit ParticleEmitter(const EmitterSettings& settings);

    // For continueous emission
    void update(float dt, const sf::Vector2f& currentPosition);

    void burst(sf::Vector2f position, ParticleType type);

    void setSetting(const EmitterSettings& settings);
    void setEnable(bool enabled);

private:
    void spawnParticle(const sf::Vector2f& position);

    EmitterSettings m_setting;
    bool m_enable = true;
    float m_accumulator = 0.0f;
};
