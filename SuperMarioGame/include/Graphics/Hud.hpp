#pragma once

#include <array>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/Drawable.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/ConvexShape.hpp>
#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include "Core/EventBus.hpp"

struct HudData {
    int score = 0;
    int coins = 0;
    int lives = 3;
    int timeLeft = 300;
    int worldMajor = 1;
    int worldMinor = 1;
    int comboCount = 1;
    std::string characterName = "mario";
    bool pSwitchActive = false;
    float pSwitchTimer = 0.0f;
    std::array<bool,3> starCoinsCollected;

    // Boss HUD Elements
    bool bossActive = false;
    std::string bossName = "BOWSER";
    int bossHealth = 100;
    int bossMaxHealth = 100;
};

class SpriteSheet;

class Hud : public sf::Drawable {
public:
    Hud(sf::Vector2i windowSize, const SpriteSheet* itemSheet = nullptr, const SpriteSheet* playerSheet = nullptr);
    ~Hud();

    void sync(const HudData& data);

    void update(float dt);

protected:
    void draw(sf::RenderTarget& target, sf::RenderStates state) const override;

private:
    HudData m_curData;
    std::vector<EventBus::SubscriptionId> m_subscribedIds;

    sf::Text m_scoreText, m_coinsText, m_worldText,
             m_timeLeftText, m_livesText, m_comboCountText, m_pSwitchTimeText,
             m_bossNameText;
    std::vector<std::unique_ptr<sf::Drawable>> m_uiElements;

    mutable sf::ConvexShape m_starShape;
    mutable sf::CircleShape m_coinShape;
    mutable sf::RectangleShape m_healthBarOuter;
    mutable sf::RectangleShape m_healthBarInner;

    const SpriteSheet* m_itemSheet = nullptr;
    const SpriteSheet* m_playerSheet = nullptr;
    float m_animTimer = 0.0f;
};
