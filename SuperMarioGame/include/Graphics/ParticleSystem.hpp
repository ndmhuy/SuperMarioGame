#pragma once

#include <SFML/Graphics.hpp>
#include <vector>
#include "Core/EventBus.hpp"

struct ParticleData {
    sf::Vector2f position;          // Initial position
    sf::Vector2f velocity;          // Initial velocity
    sf::Vector2f acceleration;      // Acceleration of the particle
    sf::Color startColor;
    sf::Color endColor;
    float lifetime;                 // Lifetime of the particle
    float startScale = 1.0f;
    float endScale = 1.0f;          // Start and end scale
    sf::IntRect textureRect;        // Exact texture rectangle for particle 
};

class ParticleSystem : public sf::Drawable {
public:

    // Delete copy/move semantics for Singleton
    ParticleSystem(const ParticleSystem &) = delete;
    ParticleSystem& operator=(const ParticleSystem &) = delete;
    ParticleSystem(ParticleSystem &&) = delete;
    ParticleSystem& operator=(ParticleSystem &&) = delete;

    // Singleton Instance
    static ParticleSystem& getInstance();

    void update(float dt);
    void emit(ParticleData data);

private:
    struct Particle {
        sf::Vector2f position;
        sf::Vector2f velocity;
        sf::Vector2f acceleration;
        sf::Color startColor;
        sf::Color endColor;
        float maxLifetime = 0.0f;
        float startScale = 1.0f;
        float endScale = 1.0f;
        sf::IntRect textureRect;
        float scale = 1.0f;        // Size multiplier
        float lifetime = 0.0f;     // Seconds since birth
        bool active = false;       // Object pool availability flag
    };

    ParticleSystem();
    ~ParticleSystem() = default;

    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

    std::vector<Particle> m_particlePool;
    sf::VertexArray m_vertexArray;  // Quad rendering
    const sf::Texture* m_particleTexture = nullptr; // Shared texture for particles

    std::vector<EventBus::SubscriptionId> m_eventIds;
};
