#pragma once

#include <array>
#include <memory>
#include <string>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/Drawable.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/ConvexShape.hpp>
#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/RectangleShape.hpp>

struct HudData {
    int score = 0;
    int coins = 0;
    int lives = 3;
    int timeLeft = 300;

    // What the WORLD field reads.
    //
    // This used to be two ints that PlayingState never assigned, so the HUD
    // printed "WORLD 1-1" from HudData's own defaults for every level in the
    // game — 1-2, 1-3, the bonus stage and every procedural level included.
    // A string rather than a pair of numbers because not every level in this
    // game HAS a major-minor number: "BONUS 1" and "RANDOM" are not 1-4 and 1-5,
    // and forcing them through a "%d-%d" was what made this wrong to begin with.
    std::string worldLabel = "WORLD 1-1";
    int comboCount = 1;
    // Seconds left on the chain. The combo text used to be drawn whenever the
    // count exceeded 1 and the count never came down, so a 36px "x7!" stayed
    // nailed to the middle of the screen for the rest of the level. It now fades
    // out as this runs down.
    float comboTimer = 0.0f;
    std::string characterName = "mario";
    bool pSwitchActive = false;
    float pSwitchTimer = 0.0f;
    std::array<bool,3> starCoinsCollected;

    // Boss HUD Elements
    bool bossActive = false;
    std::string bossName = "BOWSER";
    int bossHealth = 100;
    int bossMaxHealth = 100;
    // Fireballs still needed to stagger the boss, or -1 when the boss has no
    // such mechanic. A route through a fight the player cannot see the state of
    // is a route they will not find.
    int bossFireHitsToStagger = -1;
    bool bossStaggered = false;

    // --- Player 2 -----------------------------------------------------------
    //
    // Multiplayer had no presence in the HUD at all: Player 1 got a character
    // icon and a life count at the top left, and Player 2 got a line of small
    // text at the very bottom of the screen. Both players are playing the same
    // game and both need the same two facts about themselves.
    bool hasSecondPlayer = false;
    std::string secondCharacterName = "luigi";
    int secondLives = 3;
    int secondCoins = 0;
    // Shown instead of "P2" when the second player is a bot, because which
    // archetype is playing changes how to play against it.
    std::string secondPlayerLabel = "P2";
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

    sf::Text m_scoreText, m_coinsText, m_worldText,
             m_timeLeftText, m_livesText, m_comboCountText, m_pSwitchTimeText,
             m_bossNameText, m_secondLivesText, m_secondLabelText,
             m_bossHintText;

    // Draws one player's character icon and life count. Both players get the
    // same treatment; only the anchor differs, which is what makes the second
    // player look like a participant rather than an afterthought.
    void drawPlayerBadge(sf::RenderTarget& target, sf::RenderStates state,
                         const std::string& characterName, sf::Vector2f iconCentre) const;
    std::vector<std::unique_ptr<sf::Drawable>> m_uiElements;

    mutable sf::ConvexShape m_starShape;
    mutable sf::CircleShape m_coinShape;
    mutable sf::RectangleShape m_healthBarOuter;
    mutable sf::RectangleShape m_healthBarInner;

    const SpriteSheet* m_itemSheet = nullptr;
    const SpriteSheet* m_playerSheet = nullptr;
    float m_animTimer = 0.0f;
};
