#include "Graphics/ParticleEmitter.hpp"
#include <cstdlib>

static float randomFloat(float minVal, float maxVal) {
    if (minVal >= maxVal) return minVal;
    float r = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
    return minVal + r * (maxVal - minVal);
}

ParticleEmitter::ParticleEmitter(const EmitterSettings& settings)
    : m_setting(settings), m_enable(true), m_accumulator(0.0f) {}

void ParticleEmitter::update(float dt, const sf::Vector2f& currentPosition) {
    if (!m_enable || m_setting.emissionRate <= 0.0f) return;

    m_accumulator += dt;
    float interval = 1.0f / m_setting.emissionRate;

    while (m_accumulator >= interval) {
        spawnParticle(currentPosition);
        m_accumulator -= interval;
    }
}

void ParticleEmitter::spawnParticle(const sf::Vector2f& position) {
    ParticleData data;
    data.position = position;
    data.velocity = sf::Vector2f(
        randomFloat(m_setting.minVelocity.x, m_setting.maxVelocity.x),
        randomFloat(m_setting.minVelocity.y, m_setting.maxVelocity.y)
    );
    data.acceleration = m_setting.acceleration;
    data.startColor = m_setting.startColor;
    data.endColor = m_setting.endColor;
    data.lifetime = randomFloat(m_setting.minLifetime, m_setting.maxLifetime);
    data.startScale = m_setting.startScale;
    data.endScale = m_setting.endScale;
    data.textureRect = m_setting.textureRect;

    ParticleSystem::getInstance().emit(data);
}

void ParticleEmitter::burst(sf::Vector2f position, ParticleType type) {
    EmitterSettings preset;
    int count = 4;

    switch (type) {
        case ParticleType::BrickBreak:
            count = 4;
            preset.minVelocity = sf::Vector2f(-140.f, -320.f);
            preset.maxVelocity = sf::Vector2f(140.f, -120.f);
            preset.acceleration = sf::Vector2f(0.f, 650.f); // gravity
            preset.startColor = sf::Color::White; // Preserves true 3D brick texture colors
            preset.endColor = sf::Color(255, 255, 255, 0);
            preset.minLifetime = 0.7f;
            preset.maxLifetime = 1.1f;
            preset.startScale = 0.6f;
            preset.endScale = 0.3f;
            preset.textureRect = sf::IntRect(sf::Vector2i(32, 0), sf::Vector2i(16, 16));
            break;

        case ParticleType::CoinSparkle:
            count = 8;
            preset.minVelocity = sf::Vector2f(-80.f, -80.f);
            preset.maxVelocity = sf::Vector2f(80.f, 80.f);
            preset.acceleration = sf::Vector2f(0.f, 0.f);
            preset.startColor = sf::Color(255, 240, 160); // Bright gold-white
            preset.endColor = sf::Color(255, 215, 0, 0);
            preset.minLifetime = 0.4f;
            preset.maxLifetime = 0.7f;
            preset.startScale = 0.6f;
            preset.endScale = 0.2f;
            preset.textureRect = sf::IntRect(sf::Vector2i(16, 0), sf::Vector2i(16, 16));
            break;

        case ParticleType::DeathPoof:
            count = 12;
            preset.minVelocity = sf::Vector2f(-110.f, -110.f);
            preset.maxVelocity = sf::Vector2f(110.f, 110.f);
            preset.acceleration = sf::Vector2f(0.f, 0.f);
            preset.startColor = sf::Color(255, 255, 255); // White smoke
            preset.endColor = sf::Color(240, 240, 240, 0);
            preset.minLifetime = 0.5f;
            preset.maxLifetime = 0.8f;
            preset.startScale = 0.75f;
            preset.endScale = 0.2f;
            preset.textureRect = sf::IntRect(sf::Vector2i(0, 16), sf::Vector2i(16, 16));
            break;

        case ParticleType::Stomp:
            count = 6;
            preset.minVelocity = sf::Vector2f(-150.f, -40.f);
            preset.maxVelocity = sf::Vector2f(150.f, 0.f);
            preset.acceleration = sf::Vector2f(0.f, -30.f);
            preset.startColor = sf::Color(240, 240, 240, 230);
            preset.endColor = sf::Color(220, 220, 220, 0);
            preset.minLifetime = 0.3f;
            preset.maxLifetime = 0.5f;
            preset.startScale = 0.6f;
            preset.endScale = 0.2f;
            preset.textureRect = sf::IntRect(sf::Vector2i(0, 0), sf::Vector2i(16, 16));
            break;

        case ParticleType::Combo:
            count = 6;
            preset.minVelocity = sf::Vector2f(-90.f, -180.f);
            preset.maxVelocity = sf::Vector2f(90.f, -70.f);
            preset.acceleration = sf::Vector2f(0.f, 100.f);
            preset.startColor = sf::Color(255, 255, 180); // Bright star
            preset.endColor = sf::Color(255, 100, 255, 0); // Magenta fade
            preset.minLifetime = 0.6f;
            preset.maxLifetime = 1.0f;
            preset.startScale = 0.7f;
            preset.endScale = 0.25f;
            preset.textureRect = sf::IntRect(sf::Vector2i(16, 0), sf::Vector2i(16, 16));
            break;

        case ParticleType::WallDust:
            count = 4;
            preset.minVelocity = sf::Vector2f(-40.f, -40.f);
            preset.maxVelocity = sf::Vector2f(40.f, 20.f);
            preset.acceleration = sf::Vector2f(0.f, 0.f);
            preset.startColor = sf::Color(240, 240, 240, 220);
            preset.endColor = sf::Color(200, 200, 200, 0);
            preset.minLifetime = 0.35f;
            preset.maxLifetime = 0.6f;
            preset.startScale = 0.45f;
            preset.endScale = 0.15f;
            preset.textureRect = sf::IntRect(sf::Vector2i(0, 0), sf::Vector2i(16, 16));
            break;

        case ParticleType::WaterBubble:
            count = 4;
            preset.minVelocity = sf::Vector2f(-20.f, -60.f);
            preset.maxVelocity = sf::Vector2f(20.f, -25.f);
            preset.acceleration = sf::Vector2f(0.f, -20.f); // buoyancy
            preset.startColor = sf::Color(220, 245, 255, 240);
            preset.endColor = sf::Color(180, 220, 255, 0);
            preset.minLifetime = 1.0f;
            preset.maxLifetime = 1.6f;
            preset.startScale = 0.4f;
            preset.endScale = 0.7f;
            preset.textureRect = sf::IntRect(sf::Vector2i(48, 0), sf::Vector2i(16, 16));
            break;

        case ParticleType::LavaEmber:
            count = 4;
            preset.minVelocity = sf::Vector2f(-30.f, -100.f);
            preset.maxVelocity = sf::Vector2f(30.f, -40.f);
            preset.acceleration = sf::Vector2f(0.f, -10.f);
            preset.startColor = sf::Color(255, 160, 20); // Bright lava orange
            preset.endColor = sf::Color(255, 0, 0, 0);   // Red transparent
            preset.minLifetime = 0.8f;
            preset.maxLifetime = 1.4f;
            preset.startScale = 0.5f;
            preset.endScale = 0.15f;
            preset.textureRect = sf::IntRect(sf::Vector2i(16, 16), sf::Vector2i(16, 16));
            break;
    }

    EmitterSettings oldSetting = m_setting;
    m_setting = preset;
    for (int i = 0; i < count; ++i) {
        spawnParticle(position);
    }
    m_setting = oldSetting;
}

void ParticleEmitter::setSetting(const EmitterSettings& settings) {
    m_setting = settings;
}

void ParticleEmitter::setEnable(bool enabled) {
    m_enable = enabled;
}
