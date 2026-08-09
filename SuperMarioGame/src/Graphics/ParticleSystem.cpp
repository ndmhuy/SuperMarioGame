#include "Graphics/ParticleSystem.hpp"
#include "Core/ResourceManager.hpp"
#include <filesystem>
#include <iostream>
#include <cstdint>

ParticleSystem& ParticleSystem::getInstance() {
    static ParticleSystem instance;
    return instance;
}

ParticleSystem::ParticleSystem() {
    // Pre-allocate object pool to fixed size of 200 particles (SPEC §9)
    m_particlePool.resize(200);

    // Triangle rendering primitive (2 triangles per particle quad for SFML 3.0)
    m_vertexArray.setPrimitiveType(sf::PrimitiveType::Triangles);

    // Resolve texture path for particles
    ResourceManager& rm = ResourceManager::getInstance();
    std::string texPath = "assets/spriteSheet/particles/particles.png";
    if (!std::filesystem::exists(texPath)) texPath = "../assets/spriteSheet/particles/particles.png";
    if (!std::filesystem::exists(texPath)) texPath = "../../assets/spriteSheet/particles/particles.png";
    if (!std::filesystem::exists(texPath)) texPath = "SuperMarioGame/assets/spriteSheet/particles/particles.png";
    if (!std::filesystem::exists(texPath)) texPath = "../SuperMarioGame/assets/spriteSheet/particles/particles.png";
    if (!std::filesystem::exists(texPath)) texPath = "assets/spriteSheet/test/test.png";
    if (!std::filesystem::exists(texPath)) texPath = "../assets/spriteSheet/test/test.png";

    if (std::filesystem::exists(texPath)) {
        if (rm.loadTexture("particleTexture", texPath)) {
            m_particleTexture = &rm.getTexture("particleTexture");
        }
    }
}

void ParticleSystem::emit(ParticleData data) {
    for (auto& p : m_particlePool) {
        if (!p.active) {
            p.position = data.position;
            p.velocity = data.velocity;
            p.acceleration = data.acceleration;
            p.startColor = data.startColor;
            p.endColor = data.endColor;
            p.maxLifetime = (data.lifetime > 0.0f) ? data.lifetime : 0.5f;
            p.startScale = data.startScale;
            p.endScale = data.endScale;
            p.textureRect = data.textureRect;
            p.scale = data.startScale;
            p.lifetime = 0.0f;
            p.active = true;
            break;
        }
    }
}

void ParticleSystem::update(float dt) {
    std::vector<size_t> activeIndices;
    activeIndices.reserve(m_particlePool.size());

    for (size_t i = 0; i < m_particlePool.size(); ++i) {
        auto& p = m_particlePool[i];
        if (!p.active) continue;

        p.lifetime += dt;
        if (p.lifetime >= p.maxLifetime) {
            p.active = false;
            continue;
        }

        // Update physics
        p.velocity += p.acceleration * dt;
        p.position += p.velocity * dt;

        // Progress fraction [0, 1]
        float t = p.lifetime / p.maxLifetime;
        if (t < 0.0f) t = 0.0f;
        if (t > 1.0f) t = 1.0f;

        // Scale interpolation
        p.scale = p.startScale + t * (p.endScale - p.startScale);

        activeIndices.push_back(i);
    }

    // Rebuild VertexArray for fast single draw call (using 2 triangles per particle quad for SFML 3.0)
    m_vertexArray.resize(activeIndices.size() * 6);

    for (size_t idx = 0; idx < activeIndices.size(); ++idx) {
        const auto& p = m_particlePool[activeIndices[idx]];
        float t = p.lifetime / p.maxLifetime;
        if (t < 0.0f) t = 0.0f;
        if (t > 1.0f) t = 1.0f;

        // Color (RGBA) linear interpolation
        std::uint8_t r = static_cast<std::uint8_t>(p.startColor.r + t * (p.endColor.r - p.startColor.r));
        std::uint8_t g = static_cast<std::uint8_t>(p.startColor.g + t * (p.endColor.g - p.startColor.g));
        std::uint8_t b = static_cast<std::uint8_t>(p.startColor.b + t * (p.endColor.b - p.startColor.b));
        std::uint8_t a = static_cast<std::uint8_t>(p.startColor.a + t * (p.endColor.a - p.startColor.a));
        sf::Color currentColor(r, g, b, a);

        // Quad dimensions based on textureRect size (or fallback 16x16 if empty)
        float width = (p.textureRect.size.x > 0) ? static_cast<float>(p.textureRect.size.x) : 16.0f;
        float height = (p.textureRect.size.y > 0) ? static_cast<float>(p.textureRect.size.y) : 16.0f;

        float halfW = (width * p.scale) * 0.5f;
        float halfH = (height * p.scale) * 0.5f;

        // 4 Corner Positions
        sf::Vector2f tl = p.position + sf::Vector2f(-halfW, -halfH);
        sf::Vector2f tr = p.position + sf::Vector2f(halfW, -halfH);
        sf::Vector2f br = p.position + sf::Vector2f(halfW, halfH);
        sf::Vector2f bl = p.position + sf::Vector2f(-halfW, halfH);

        // Texture UV coordinates
        float texLeft = static_cast<float>(p.textureRect.position.x);
        float texTop = static_cast<float>(p.textureRect.position.y);
        float texRight = texLeft + width;
        float texBottom = texTop + height;

        sf::Vector2f uvTL(texLeft, texTop);
        sf::Vector2f uvTR(texRight, texTop);
        sf::Vector2f uvBR(texRight, texBottom);
        sf::Vector2f uvBL(texLeft, texBottom);

        size_t vIdx = idx * 6;

        // Triangle 1 (TL -> TR -> BR)
        m_vertexArray[vIdx + 0].position = tl;
        m_vertexArray[vIdx + 0].color = currentColor;
        m_vertexArray[vIdx + 0].texCoords = uvTL;

        m_vertexArray[vIdx + 1].position = tr;
        m_vertexArray[vIdx + 1].color = currentColor;
        m_vertexArray[vIdx + 1].texCoords = uvTR;

        m_vertexArray[vIdx + 2].position = br;
        m_vertexArray[vIdx + 2].color = currentColor;
        m_vertexArray[vIdx + 2].texCoords = uvBR;

        // Triangle 2 (TL -> BR -> BL)
        m_vertexArray[vIdx + 3].position = tl;
        m_vertexArray[vIdx + 3].color = currentColor;
        m_vertexArray[vIdx + 3].texCoords = uvTL;

        m_vertexArray[vIdx + 4].position = br;
        m_vertexArray[vIdx + 4].color = currentColor;
        m_vertexArray[vIdx + 4].texCoords = uvBR;

        m_vertexArray[vIdx + 5].position = bl;
        m_vertexArray[vIdx + 5].color = currentColor;
        m_vertexArray[vIdx + 5].texCoords = uvBL;
    }
}

void ParticleSystem::draw(sf::RenderTarget& target, sf::RenderStates states) const {
    if (m_vertexArray.getVertexCount() == 0) return;
    if (m_particleTexture) {
        states.texture = m_particleTexture;
    }
    target.draw(m_vertexArray, states);
}
