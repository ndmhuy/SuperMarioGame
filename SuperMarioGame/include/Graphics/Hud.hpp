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
    // This level has no countdown, so the TIME field shows dashes rather than a
    // number that never changes. Endless Mode is scored on distance, not time.
    bool timeUnlimited = false;

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
    // Bridge axes already reached, out of how many this fight has. A total of 0
    // means this fight has no axe route and the field is not drawn.
    //
    // Same reasoning as bossFireHitsToStagger above, and it bites harder here:
    // the axe count now scales with difficulty and EVERY axe has to be reached
    // before the bridge drops, so without this the player touches an axe on
    // Hard, watches the bridge stay exactly where it was, and concludes the axe
    // is broken rather than that two more are waiting.
    int bossAxesTotal = 0;
    int bossAxesReached = 0;

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

    // --- Elimination ---------------------------------------------------------
    //
    // This player has spent their last life and left the level. In a two-player
    // match the survivor plays on, so the badge still has something to say —
    // and what it said before was wrong in two different ways: Player 1's badge
    // stayed on screen frozen on its last values (Hud::draw draws it
    // unconditionally, and PlayingState stopped feeding it once
    // Game::getPlayer() went null), while Player 2's disappeared entirely
    // (its whole HUD block was gated on the live m_player2 pointer).
    //
    // Carried as an explicit fact rather than inferred from a pointer because
    // by the time the HUD syncs the Player object has been destroy()ed and
    // forgotten — there is nothing left to ask who it was.
    bool eliminated = false;
    bool secondEliminated = false;
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
    // tests/verify_r21_versus_and_axes.cpp reads back what the running game's
    // HUD was actually handed after an elimination. Asserting against a HudData
    // the harness composed itself would pass while the game still drew a stale
    // badge — which is the defect — so the harness has to see m_curData.
    friend class VersusAndAxesTestHooks;

    HudData m_curData;

    sf::Text m_scoreText, m_coinsText, m_worldText,
             m_timeLeftText, m_livesText, m_comboCountText, m_pSwitchTimeText,
             m_bossNameText, m_secondLivesText, m_secondLabelText,
             m_bossHintText, m_bossAxeText;

    // Draws one player's character icon and life count. Both players get the
    // same treatment; only the anchor differs, which is what makes the second
    // player look like a participant rather than an afterthought.
    //
    // `eliminated` dims the icon and stamps "OUT" under it. The user asked for
    // a dead player to be SHOWN as dead rather than silently removed, and a
    // badge that simply vanishes reads as a rendering bug from the survivor's
    // seat.
    void drawPlayerBadge(sf::RenderTarget& target, sf::RenderStates state,
                         const std::string& characterName, sf::Vector2f iconCentre,
                         bool eliminated) const;

    // Left edge for the coin icon. Derived from where the coin count actually
    // ended up, because that field is right-aligned and therefore moves with
    // the digit count: a fixed icon x would drift away from a two-digit total
    // and be overlapped by a five-digit one.
    float coinIconX() const;
    std::vector<std::unique_ptr<sf::Drawable>> m_uiElements;

    mutable sf::ConvexShape m_starShape;
    mutable sf::CircleShape m_coinShape;
    mutable sf::RectangleShape m_healthBarOuter;
    mutable sf::RectangleShape m_healthBarInner;

    const SpriteSheet* m_itemSheet = nullptr;
    const SpriteSheet* m_playerSheet = nullptr;
    float m_animTimer = 0.0f;
};
