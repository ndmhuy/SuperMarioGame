#include "Entities/Flagpole.hpp"
#include "Entities/Player.hpp"
#include "Core/SoundManager.hpp"
#include "Core/EventBus.hpp"
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/ConvexShape.hpp>
#include <algorithm>
#include <cmath>

Flagpole::Flagpole(sf::Vector2f position, float poleHeight)
    : Block(position), m_poleHeight(poleHeight), m_triggered(false) {
    m_breakable = false;
    boundingBox.width = 16.0f; // Thin flagpole physics trigger
    boundingBox.height = m_poleHeight;

    m_flagY = position.y + 8.0f;
    m_targetFlagY = position.y + m_poleHeight - 32.0f;
}

void Flagpole::update(float dt) {
    if (m_triggered) {
        m_animTimer += 6.0f * dt;
        if (m_flagY < m_targetFlagY) {
            m_flagY += 220.0f * dt; // Flag smoothly slides down pole
            if (m_flagY > m_targetFlagY) {
                m_flagY = m_targetFlagY;
            }
        }
    }
}

void Flagpole::onHitFromBelow(Player& player) {
    // Flagpole is not hit from below
}

void Flagpole::onPlayerCollision(Player& player, float collisionY) {
    if (m_triggered) return;
    m_triggered = true;

    // Calculate height caught relative to flagpole top
    float distanceSelfFromTop = collisionY - position.y;
    float heightFromBottom = m_poleHeight - distanceSelfFromTop;

    // Clamp between 0 and m_poleHeight
    heightFromBottom = std::max(0.0f, std::min(heightFromBottom, m_poleHeight));

    float percentage = heightFromBottom / m_poleHeight;
    int points = 100;
    if (percentage >= 0.8f) {
        points = 5000;
    } else if (percentage >= 0.6f) {
        points = 2000;
    } else if (percentage >= 0.4f) {
        points = 800;
    } else if (percentage >= 0.2f) {
        points = 400;
    }

    player.addScore(points);
    SoundManager::getInstance().playSound("flagpole");
    EventBus::getInstance().publish({EventType::LevelComplete, points});
}

void Flagpole::render(sf::RenderTarget& target) {
    if (!active) return;

    // 1. Solid Base Block
    sf::RectangleShape baseBlock({32.0f, 16.0f});
    baseBlock.setPosition({position.x - 8.0f, position.y + m_poleHeight - 16.0f});
    baseBlock.setFillColor(sf::Color(160, 82, 45)); // Sienna brown base block
    baseBlock.setOutlineColor(sf::Color(80, 40, 20));
    baseBlock.setOutlineThickness(1.0f);
    target.draw(baseBlock);

    // 2. Metallic Flagpole Shaft
    sf::RectangleShape poleShaft({6.0f, m_poleHeight - 16.0f});
    poleShaft.setPosition({position.x + 5.0f, position.y});
    poleShaft.setFillColor(sf::Color(220, 220, 220)); // Silver metallic pole
    poleShaft.setOutlineColor(sf::Color(100, 100, 100));
    poleShaft.setOutlineThickness(1.0f);
    target.draw(poleShaft);

    // 3. Golden Top Orb Cap
    sf::CircleShape topOrb(6.0f);
    topOrb.setOrigin({6.0f, 6.0f});
    topOrb.setPosition({position.x + 8.0f, position.y - 2.0f});
    topOrb.setFillColor(sf::Color(255, 215, 0)); // Bright golden cap
    topOrb.setOutlineColor(sf::Color(180, 140, 0));
    topOrb.setOutlineThickness(1.0f);
    target.draw(topOrb);

    // 4. Waving Victory Flag attached to pole
    sf::ConvexShape flag;
    flag.setPointCount(3);

    float waveOffset = m_triggered ? std::sin(m_animTimer) * 2.5f : 0.0f;
    float currentY = m_flagY + waveOffset;

    flag.setPoint(0, {position.x - 24.0f, currentY});
    flag.setPoint(1, {position.x + 5.0f, currentY - 12.0f});
    flag.setPoint(2, {position.x + 5.0f, currentY + 12.0f});

    flag.setFillColor(sf::Color(230, 40, 40)); // Vibrant Mario Red Flag
    flag.setOutlineColor(sf::Color::White);
    flag.setOutlineThickness(1.0f);
    target.draw(flag);
}
