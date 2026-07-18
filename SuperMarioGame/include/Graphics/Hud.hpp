#pragma once

#include <array>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/Drawable.hpp>
#include <SFML/Graphics/Text.hpp>
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
};

class Hud : public sf::Drawable {
public:
    Hud(sf::Vector2i windowSize);
    ~Hud();

    void sync(const HudData& data);

    void update(float dt);

protected:
    void draw(sf::RenderTarget& target, sf::RenderStates state) const override;

private:
    HudData m_curData;
    std::vector<EventBus::SubscriptionId> m_subscribedIds;

    sf::Text m_scoreText, m_coinsText, m_worldText,
             m_timeLeftText, m_livesText, m_comboCountText, m_pSwitchTimeText;
    std::vector<std::unique_ptr<sf::Drawable>> m_uiElements;
};
