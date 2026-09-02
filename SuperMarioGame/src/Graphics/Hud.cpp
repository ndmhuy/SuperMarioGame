#include "Graphics/Hud.hpp"
#include "Core/ResourceManager.hpp"
#include "Graphics/SpriteSheet.hpp"
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Text.hpp>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <iostream>

Hud::Hud(sf::Vector2i windowSize, const SpriteSheet* itemSheet, const SpriteSheet* playerSheet)
    : m_scoreText(ResourceManager::getInstance().getFont("PressStart2P")),
      m_coinsText(ResourceManager::getInstance().getFont("PressStart2P")),
      m_worldText(ResourceManager::getInstance().getFont("PressStart2P")),
      m_timeLeftText(ResourceManager::getInstance().getFont("PressStart2P")),
      m_livesText(ResourceManager::getInstance().getFont("PressStart2P")),
      m_comboCountText(ResourceManager::getInstance().getFont("PressStart2P")),
      m_pSwitchTimeText(ResourceManager::getInstance().getFont("PressStart2P")),
      m_bossNameText(ResourceManager::getInstance().getFont("PressStart2P")),
      m_secondLivesText(ResourceManager::getInstance().getFont("PressStart2P")),
      m_secondLabelText(ResourceManager::getInstance().getFont("PressStart2P")),
      m_bossHintText(ResourceManager::getInstance().getFont("PressStart2P")),
      m_itemSheet(itemSheet),
      m_playerSheet(playerSheet) {

    // Initialize text attributes
    m_scoreText.setCharacterSize(24);
    m_scoreText.setFillColor(sf::Color::White);
    m_scoreText.setOutlineColor(sf::Color(0, 0, 0, 220));
    m_scoreText.setOutlineThickness(2.0f);

    m_coinsText.setCharacterSize(24);
    m_coinsText.setFillColor(sf::Color::White);
    m_coinsText.setOutlineColor(sf::Color(0, 0, 0, 220));
    m_coinsText.setOutlineThickness(2.0f);

    m_worldText.setCharacterSize(24);
    m_worldText.setFillColor(sf::Color::Red);
    m_worldText.setOutlineColor(sf::Color(0, 0, 0, 220));
    m_worldText.setOutlineThickness(2.0f);

    m_timeLeftText.setCharacterSize(24);
    m_timeLeftText.setFillColor(sf::Color::White);
    m_timeLeftText.setOutlineColor(sf::Color(0, 0, 0, 220));
    m_timeLeftText.setOutlineThickness(2.0f);

    m_livesText.setCharacterSize(24);
    m_livesText.setFillColor(sf::Color::White);
    m_livesText.setOutlineColor(sf::Color(0, 0, 0, 220));
    m_livesText.setOutlineThickness(2.0f);

    m_comboCountText.setCharacterSize(36);
    m_comboCountText.setFillColor(sf::Color::Green);
    m_comboCountText.setOutlineColor(sf::Color(0, 0, 0, 220));
    m_comboCountText.setOutlineThickness(2.0f);

    m_pSwitchTimeText.setCharacterSize(24);
    m_pSwitchTimeText.setFillColor(sf::Color::Yellow);
    m_pSwitchTimeText.setOutlineColor(sf::Color(0, 0, 0, 220));
    m_pSwitchTimeText.setOutlineThickness(2.0f);

    m_bossNameText.setCharacterSize(20);
    m_bossNameText.setFillColor(sf::Color::Red);
    m_bossNameText.setOutlineColor(sf::Color(0, 0, 0, 220));
    m_bossNameText.setOutlineThickness(2.0f);

    // Player 2's block mirrors Player 1's: same size, same outline, same shape.
    m_secondLivesText.setCharacterSize(24);
    m_secondLivesText.setFillColor(sf::Color::White);
    m_secondLivesText.setOutlineColor(sf::Color(0, 0, 0, 220));
    m_secondLivesText.setOutlineThickness(2.0f);

    m_secondLabelText.setCharacterSize(24);
    m_secondLabelText.setFillColor(sf::Color(120, 220, 120));   // green: Luigi's colour
    m_secondLabelText.setOutlineColor(sf::Color(0, 0, 0, 220));
    m_secondLabelText.setOutlineThickness(2.0f);

    m_bossHintText.setCharacterSize(12);
    m_bossHintText.setFillColor(sf::Color(255, 200, 80));
    m_bossHintText.setOutlineColor(sf::Color(0, 0, 0, 220));
    m_bossHintText.setOutlineThickness(2.0f);

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

float Hud::coinIconX() const {
    // 25px is the icon's own 24px width plus a hair of separation.
    return m_coinsText.getPosition().x - m_coinsText.getOrigin().x - 25.0f;
}

void Hud::sync(const HudData& data) {
    m_curData = data;

    // The right-hand HUD column grows leftwards from this edge instead of
    // rightwards off the screen. "%06d" is a minimum width, not a maximum: a
    // seventh score digit reached x=1265 and an eighth left the 1280px window
    // entirely, and four-digit coin counts did the same one row up. Anchoring
    // the right edge makes the field length irrelevant.
    constexpr float RIGHT_EDGE = 1244.f;
    const auto pinRight = [](sf::Text& text, float y) {
        const sf::FloatRect bounds = text.getLocalBounds();
        text.setOrigin(sf::Vector2f(bounds.position.x + bounds.size.x, 0.0f));
        text.setPosition(sf::Vector2f(RIGHT_EDGE, y));
    };

    // 1. Score: at least 6 digits, padded with zero (Y=60, right-aligned)
    char scoreBuf[32];
    std::snprintf(scoreBuf, sizeof(scoreBuf), "%06d", m_curData.score);
    m_scoreText.setString(scoreBuf);
    pinRight(m_scoreText, 60.f);

    // 2. Coins: x 57 (Y=25, right-aligned)
    char coinsBuf[32];
    std::snprintf(coinsBuf, sizeof(coinsBuf), "x %02d", m_curData.coins);
    m_coinsText.setString(coinsBuf);
    pinRight(m_coinsText, 25.f);

    // 3. World Level (Y=25, Center). Centred on its own width rather than
    // pinned to x=540: the label is no longer a fixed-width "WORLD 1-1", and a
    // longer one such as "WORLD 1-1 SUB" would have run into the TIME field.
    m_worldText.setString(m_curData.worldLabel);
    {
        const sf::FloatRect bounds = m_worldText.getLocalBounds();
        m_worldText.setOrigin(sf::Vector2f(bounds.size.x * 0.5f, 0.0f));
        m_worldText.setPosition(sf::Vector2f(620.f, 25.f));
    }

    // 4. Time left countdown (Y=60, Center-Right)
    char timeBuf[32];
    std::snprintf(timeBuf, sizeof(timeBuf), "%03d", m_curData.timeLeft);
    m_timeLeftText.setString(timeBuf);
    m_timeLeftText.setPosition(sf::Vector2f(820.f, 60.f));

    // 5. Lives count: x  9 (Y=60, Left)
    char livesBuf[32];
    std::snprintf(livesBuf, sizeof(livesBuf), "x  %d", m_curData.lives);
    m_livesText.setString(livesBuf);
    if (m_playerSheet) {
        m_livesText.setPosition(sf::Vector2f(90.f, 60.f)); // Shifted right for character icon
    } else {
        m_livesText.setPosition(sf::Vector2f(60.f, 60.f));
    }

    // 6. Combo text: x2! (Center, fades with the chain)
    if (m_curData.comboCount > 1 && m_curData.comboTimer > 0.0f) {
        char comboBuf[32];
        std::snprintf(comboBuf, sizeof(comboBuf), "x%d!", m_curData.comboCount);
        m_comboCountText.setString(comboBuf);
        sf::FloatRect bounds = m_comboCountText.getLocalBounds();
        m_comboCountText.setOrigin(sf::Vector2f(bounds.size.x / 2.f, bounds.size.y / 2.f));
        // Drawn above centre rather than dead centre: at (640, 360) it sat
        // directly on top of the player it was describing.
        m_comboCountText.setPosition(sf::Vector2f(640.f, 260.f));

        // Fade over the last second, so it leaves rather than blinking out.
        const float fade = std::min(1.0f, m_curData.comboTimer);
        const auto alpha = static_cast<std::uint8_t>(255.0f * fade);
        m_comboCountText.setFillColor(sf::Color(0, 255, 0, alpha));
        m_comboCountText.setOutlineColor(sf::Color(0, 0, 0, static_cast<std::uint8_t>(220.0f * fade)));
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

    // 7b. Player 2's life count, in the same "x N" form as Player 1's.
    if (m_curData.hasSecondPlayer) {
        char p2Buf[32];
        std::snprintf(p2Buf, sizeof(p2Buf), "x  %d", m_curData.secondLives);
        m_secondLivesText.setString(p2Buf);
        m_secondLivesText.setPosition(sf::Vector2f(310.f, 60.f));

        std::string label = m_curData.secondPlayerLabel;
        std::transform(label.begin(), label.end(), label.begin(), ::toupper);
        m_secondLabelText.setString(label);
        m_secondLabelText.setPosition(sf::Vector2f(280.f, 25.f));
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

        // How to actually hurt it. Bowser takes no damage from fire, so a player
        // with a fire flower has no way to learn that fire is nonetheless the
        // route to an opening unless the HUD says so.
        if (m_curData.bossStaggered) {
            m_bossHintText.setString("STUNNED - STOMP NOW");
        } else if (m_curData.bossFireHitsToStagger > 0) {
            char hintBuf[64];
            std::snprintf(hintBuf, sizeof(hintBuf), "%d FIREBALLS TO STUN",
                          m_curData.bossFireHitsToStagger);
            m_bossHintText.setString(hintBuf);
        } else {
            m_bossHintText.setString("");
        }
        const sf::FloatRect hintBounds = m_bossHintText.getLocalBounds();
        m_bossHintText.setOrigin(sf::Vector2f(hintBounds.size.x * 0.5f, 0.0f));
        m_bossHintText.setPosition(sf::Vector2f(640.f, 186.f));
    }
}

void Hud::update(float dt) {
    m_animTimer += dt;
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
    bool coinSpriteDrawn = false;
    if (m_itemSheet) {
        int coinFrame = static_cast<int>(m_animTimer / 0.15f) % 4;
        sf::Sprite coinSprite = m_itemSheet->getSprite("coin_" + std::to_string(coinFrame));
        sf::FloatRect bounds = coinSprite.getLocalBounds();
        if (bounds.size.x > 0 && bounds.size.y > 0) {
            float targetHeight = 24.0f;
            float scale = targetHeight / bounds.size.y;
            coinSprite.setScale(sf::Vector2f(scale, scale));
            coinSprite.setPosition(sf::Vector2f(coinIconX(), 28.f));
            target.draw(coinSprite, state);
            coinSpriteDrawn = true;
        }
    }
    if (!coinSpriteDrawn) {
        m_coinShape.setPosition(sf::Vector2f(coinIconX(), 32.f));
        target.draw(m_coinShape, state);
    }

    // 4. Draw Star Coins (Y=95, below Time Left)
    for (int i = 0; i < 3; ++i) {
        bool starSpriteDrawn = false;
        if (m_itemSheet) {
            sf::Sprite starSprite = m_curData.starCoinsCollected[i]
                ? m_itemSheet->getSprite("big_coin")
                : m_itemSheet->getSprite("big_coin_outline");
            sf::FloatRect bounds = starSprite.getLocalBounds();
            if (bounds.size.x > 0 && bounds.size.y > 0) {
                float scale = 22.0f / bounds.size.y;
                starSprite.setScale(sf::Vector2f(scale, scale));
                starSprite.setOrigin(sf::Vector2f(bounds.size.x * 0.5f, bounds.size.y * 0.5f));
                starSprite.setPosition(sf::Vector2f(820.f + i * 30.f + 10.f, 95.f + 10.f));
                target.draw(starSprite, state);
                starSpriteDrawn = true;
            }
        }
        if (!starSpriteDrawn) {
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
    }

    // 5. Draw Dynamic Text Indicators
    target.draw(m_scoreText, state);
    target.draw(m_coinsText, state);
    target.draw(m_worldText, state);
    target.draw(m_timeLeftText, state);

    drawPlayerBadge(target, state, m_curData.characterName, {72.f, 72.f});
    target.draw(m_livesText, state);

    // 5b. Player 2, given the same icon-and-lives badge as Player 1 rather than
    // a line of 12px text at the bottom of the screen.
    if (m_curData.hasSecondPlayer) {
        drawPlayerBadge(target, state, m_curData.secondCharacterName, {292.f, 72.f});
        target.draw(m_secondLabelText, state);
        target.draw(m_secondLivesText, state);
    }

    // 6. Draw Combo Counter (only while the chain is actually running)
    if (m_curData.comboCount > 1 && m_curData.comboTimer > 0.0f) {
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
        if (!m_bossHintText.getString().isEmpty()) {
            target.draw(m_bossHintText, state);
        }
    }
}

void Hud::drawPlayerBadge(sf::RenderTarget& target, sf::RenderStates state,
                          const std::string& characterName, sf::Vector2f iconCentre) const {
    if (!m_playerSheet) return;

    std::string charName = characterName;
    std::transform(charName.begin(), charName.end(), charName.begin(), ::tolower);
    sf::Sprite playerSprite = m_playerSheet->getSprite(charName + "_small_idle");

    const sf::FloatRect bounds = playerSprite.getLocalBounds();
    if (bounds.size.x <= 0.0f || bounds.size.y <= 0.0f) return;

    const float scale = 24.0f / bounds.size.y;
    playerSprite.setScale(sf::Vector2f(scale, scale));
    playerSprite.setOrigin(sf::Vector2f(bounds.size.x * 0.5f, bounds.size.y * 0.5f));
    playerSprite.setPosition(iconCentre);
    target.draw(playerSprite, state);
}
