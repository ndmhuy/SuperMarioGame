#include "Graphics/Hud.hpp"
#include "Core/ResourceManager.hpp"
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/Text.hpp>
#include <cmath>
#include <iostream>
#include <algorithm>

Hud::Hud(sf::Vector2i windowSize)
    : m_scoreText(ResourceManager::getInstance().getFont("PressStart2P")),
      m_coinsText(ResourceManager::getInstance().getFont("PressStart2P")),
      m_worldText(ResourceManager::getInstance().getFont("PressStart2P")),
      m_timeLeftText(ResourceManager::getInstance().getFont("PressStart2P")),
      m_livesText(ResourceManager::getInstance().getFont("PressStart2P")),
      m_comboCountText(ResourceManager::getInstance().getFont("PressStart2P")),
      m_pSwitchTimeText(ResourceManager::getInstance().getFont("PressStart2P")),
      m_bossNameText(ResourceManager::getInstance().getFont("PressStart2P")) {

    // Initialize text attributes
    m_scoreText.setCharacterSize(24);
    m_scoreText.setFillColor(sf::Color::White);

    m_coinsText.setCharacterSize(24);
    m_coinsText.setFillColor(sf::Color::White);

    m_worldText.setCharacterSize(24);
    m_worldText.setFillColor(sf::Color::Red);

    m_timeLeftText.setCharacterSize(24);
    m_timeLeftText.setFillColor(sf::Color::White);

    m_livesText.setCharacterSize(24);
    m_livesText.setFillColor(sf::Color::White);

    m_comboCountText.setCharacterSize(36);
    m_comboCountText.setFillColor(sf::Color::Green);

    m_pSwitchTimeText.setCharacterSize(24);
    m_pSwitchTimeText.setFillColor(sf::Color::Yellow);

    m_bossNameText.setCharacterSize(20);
    m_bossNameText.setFillColor(sf::Color::Red);

    // Configure health bar background
    m_healthBarOuter.setSize(sf::Vector2f(300.f, 20.f));
    m_healthBarOuter.setFillColor(sf::Color(50, 50, 50));
    m_healthBarOuter.setOutlineColor(sf::Color::White);
    m_healthBarOuter.setOutlineThickness(2.0f);
    m_healthBarOuter.setPosition(sf::Vector2f(490.f, 160.f));

    // Configure health bar fill
    m_healthBarInner.setSize(sf::Vector2f(300.f, 20.f));
    m_healthBarInner.setFillColor(sf::Color::Red);
    m_healthBarInner.setPosition(sf::Vector2f(490.f, 160.f));

    // Configure 10-point star shape for Star Coins
    m_starShape.setPointCount(10);
    float outerRadius = 10.0f;
    float innerRadius = 4.0f;
    for (int i = 0; i < 10; ++i) {
        float angle = i * 3.14159265f / 5.0f - 3.14159265f / 2.0f;
        float r = (i % 2 == 0) ? outerRadius : innerRadius;
        m_starShape.setPoint(i, sf::Vector2f(r * std::cos(angle), r * std::sin(angle)));
    }

    // Configure standard coin shape
    m_coinShape.setRadius(9.0f);
    m_coinShape.setFillColor(sf::Color(255, 215, 0)); // Gold
    m_coinShape.setOutlineColor(sf::Color(200, 150, 0)); // Dark Gold/Orange
    m_coinShape.setOutlineThickness(1.5f);
}

Hud::~Hud() = default;

void Hud::sync(const HudData& data) {
    m_curData = data;

    // 1. Score: 6 digits padded with zero (Y=60, Right)
    char scoreBuf[32];
    std::snprintf(scoreBuf, sizeof(scoreBuf), "%06d", m_curData.score);
    m_scoreText.setString(scoreBuf);
    m_scoreText.setPosition(sf::Vector2f(1100.f, 60.f));

    // 2. Coins: x 57 (Y=25, Right)
    char coinsBuf[32];
    std::snprintf(coinsBuf, sizeof(coinsBuf), "x %02d", m_curData.coins);
    m_coinsText.setString(coinsBuf);
    m_coinsText.setPosition(sf::Vector2f(1125.f, 25.f));

    // 3. World Level: WORLD 1-1 (Y=25, Center)
    char worldBuf[32];
    std::snprintf(worldBuf, sizeof(worldBuf), "WORLD %d-%d", m_curData.worldMajor, m_curData.worldMinor);
    m_worldText.setString(worldBuf);
    m_worldText.setPosition(sf::Vector2f(540.f, 25.f));

    // 4. Time left countdown (Y=60, Center-Right)
    char timeBuf[32];
    std::snprintf(timeBuf, sizeof(timeBuf), "%03d", m_curData.timeLeft);
    m_timeLeftText.setString(timeBuf);
    m_timeLeftText.setPosition(sf::Vector2f(820.f, 60.f));

    // 5. Lives count: x  9 (Y=60, Left)
    char livesBuf[32];
    std::snprintf(livesBuf, sizeof(livesBuf), "x  %d", m_curData.lives);
    m_livesText.setString(livesBuf);
    m_livesText.setPosition(sf::Vector2f(60.f, 60.f));

    // 6. Combo text: x2! (temporary, Center)
    if (m_curData.comboCount > 1) {
        char comboBuf[32];
        std::snprintf(comboBuf, sizeof(comboBuf), "x%d!", m_curData.comboCount);
        m_comboCountText.setString(comboBuf);
        sf::FloatRect bounds = m_comboCountText.getLocalBounds();
        m_comboCountText.setOrigin(sf::Vector2f(bounds.size.x / 2.f, bounds.size.y / 2.f));
        m_comboCountText.setPosition(sf::Vector2f(640.f, 360.f));
    }

    // 7. P-Switch Alert: P-SWITCH 15 (when active, Top-Center)
    if (m_curData.pSwitchActive) {
        char pSwitchBuf[64];
        std::snprintf(pSwitchBuf, sizeof(pSwitchBuf), "P-SWITCH %02d", static_cast<int>(m_curData.pSwitchTimer));
        m_pSwitchTimeText.setString(pSwitchBuf);
        sf::FloatRect bounds = m_pSwitchTimeText.getLocalBounds();
        m_pSwitchTimeText.setOrigin(sf::Vector2f(bounds.size.x / 2.f, bounds.size.y / 2.f));
        m_pSwitchTimeText.setPosition(sf::Vector2f(640.f, 100.f));
    }

    // 8. Boss HUD Sync:
    if (m_curData.bossActive) {
        std::string bossUpper = m_curData.bossName;
        std::transform(bossUpper.begin(), bossUpper.end(), bossUpper.begin(), ::toupper);
        m_bossNameText.setString(bossUpper);

        // Center boss name horizontally at X = 640, Y = 130
        sf::FloatRect bounds = m_bossNameText.getLocalBounds();
        m_bossNameText.setOrigin(sf::Vector2f(bounds.size.x / 2.f, bounds.size.y / 2.f));
        m_bossNameText.setPosition(sf::Vector2f(640.f, 130.f));

        // Adjust the size of the inner health bar based on health fraction
        float healthPct = 0.0f;
        if (m_curData.bossMaxHealth > 0) {
            healthPct = std::clamp(static_cast<float>(m_curData.bossHealth) / m_curData.bossMaxHealth, 0.0f, 1.0f);
        }
        m_healthBarInner.setSize(sf::Vector2f(300.f * healthPct, 20.f));
    }
}

void Hud::update(float dt) {
    // Static fields are kept in sync via dynamic sync() calls from game loop
}

void Hud::draw(sf::RenderTarget& target, sf::RenderStates state) const {
    sf::Font& font = ResourceManager::getInstance().getFont("PressStart2P");

    // Helper text for drawing static labels
    sf::Text labelText(font);
    labelText.setCharacterSize(24);

    // 1. Draw Player Name (Y=25, Left)
    std::string nameUpper = m_curData.characterName;
    std::transform(nameUpper.begin(), nameUpper.end(), nameUpper.begin(), ::toupper);
    labelText.setString(nameUpper);
    labelText.setFillColor(sf::Color::Red);
    labelText.setPosition(sf::Vector2f(60.f, 25.f));
    target.draw(labelText, state);

    // 2. Draw TIME Label (Y=25, Center-Right)
    labelText.setString("TIME");
    labelText.setFillColor(sf::Color::Red);
    labelText.setPosition(sf::Vector2f(820.f, 25.f));
    target.draw(labelText, state);

    // 3. Draw Coin Icon (Y=32, Right)
    m_coinShape.setPosition(sf::Vector2f(1100.f, 32.f));
    target.draw(m_coinShape, state);

    // 4. Draw Star Coins (Y=95, below Time Left)
    for (int i = 0; i < 3; ++i) {
        m_starShape.setPosition(sf::Vector2f(820.f + i * 30.f + 10.f, 95.f + 10.f));
        if (m_curData.starCoinsCollected[i]) {
            m_starShape.setFillColor(sf::Color::Red);
            m_starShape.setOutlineColor(sf::Color::Red);
            m_starShape.setOutlineThickness(1.5f);
        } else {
            m_starShape.setFillColor(sf::Color::Transparent);
            m_starShape.setOutlineColor(sf::Color::Red);
            m_starShape.setOutlineThickness(1.5f);
        }
        target.draw(m_starShape, state);
    }

    // 5. Draw Dynamic Text Indicators
    target.draw(m_scoreText, state);
    target.draw(m_coinsText, state);
    target.draw(m_worldText, state);
    target.draw(m_timeLeftText, state);
    target.draw(m_livesText, state);

    // 6. Draw Combo Counter (if active)
    if (m_curData.comboCount > 1) {
        target.draw(m_comboCountText, state);
    }

    // 7. Draw P-Switch Timer (if active)
    if (m_curData.pSwitchActive) {
        target.draw(m_pSwitchTimeText, state);
    }

    // 8. Draw Boss HUD (if active)
    if (m_curData.bossActive) {
        target.draw(m_bossNameText, state);
        target.draw(m_healthBarOuter, state);
        target.draw(m_healthBarInner, state);
    }
}
