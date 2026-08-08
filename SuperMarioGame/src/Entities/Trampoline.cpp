#include "Entities/Trampoline.hpp"
#include "Entities/Player.hpp"
#include "Core/SoundManager.hpp"
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/CircleShape.hpp>
#include <cmath>

Trampoline::Trampoline(sf::Vector2f pos) : Item(pos) {
    velocity = sf::Vector2f{0.0f, 0.0f};
    boundingBox.width = 32.0f;
    boundingBox.height = 32.0f;
}

void Trampoline::update(float dt) {
    if (m_isCompressed) {
        m_compressTimer -= dt;
        if (m_compressTimer <= 0.0f) {
            m_compressTimer = 0.0f;
            m_isCompressed = false;
        }
    }
}

void Trampoline::render(sf::RenderTarget& target) {
    if (!active) return;

    float renderHeight = m_isCompressed ? 16.0f : 32.0f;
    float renderY = position.y + (32.0f - renderHeight);

    // 1. Steel base foot plate
    sf::RectangleShape basePlate({32.0f, 4.0f});
    basePlate.setPosition({position.x, position.y + 28.0f});
    basePlate.setFillColor(sf::Color(100, 100, 100));
    basePlate.setOutlineColor(sf::Color::Black);
    basePlate.setOutlineThickness(1.0f);
    target.draw(basePlate);

    // 2. Coiled metallic spring coils
    int numCoils = m_isCompressed ? 2 : 4;
    float coilStep = (renderHeight - 12.0f) / numCoils;

    for (int i = 0; i < numCoils; ++i) {
        sf::RectangleShape coil({24.0f, 3.0f});
        float coilY = renderY + 8.0f + i * coilStep;
        float offsetX = (i % 2 == 0) ? 2.0f : 6.0f;

        coil.setPosition({position.x + offsetX, coilY});
        coil.setFillColor(sf::Color(200, 200, 200));
        coil.setOutlineColor(sf::Color(80, 80, 80));
        coil.setOutlineThickness(1.0f);
        target.draw(coil);
    }

    // 3. Green padded top spring cushion
    sf::RectangleShape topCushion({32.0f, 8.0f});
    topCushion.setPosition({position.x, renderY});
    topCushion.setFillColor(sf::Color(46, 204, 113)); // Vibrant Emerald Green Top Pad
    topCushion.setOutlineColor(sf::Color(255, 235, 100)); // Golden yellow rim
    topCushion.setOutlineThickness(1.0f);
    target.draw(topCushion);
}

void Trampoline::activate(Player& player) {
    // Compression spring trigger
    m_isCompressed = true;
    m_compressTimer = 0.25f;

    // Apply high-bounce impulse ~6 tiles height (-831.4 px/s)
    sf::Vector2f vel = player.getVelocity();
    vel.y = -831.4f;
    player.setVelocity(vel);

    SoundManager::getInstance().playSound("powerup");
}

void Trampoline::collect() {
    // Trampoline is a reusable block, it is not consumed/collected.
}
