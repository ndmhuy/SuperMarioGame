#include "Core/GameOverState.hpp"
#include "Core/Game.hpp"
#include "Core/InputManager.hpp"
#include "Core/MenuState.hpp"
#include "Core/PlayingState.hpp"
#include "Core/SoundManager.hpp"
#include "Core/StatisticsTracker.hpp"
#include "Utils/Constants.hpp"
#include "Utils/LevelCatalog.hpp"
#include "Utils/Serializer.hpp"

#include <SFML/Window/Event.hpp>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <iostream>

namespace {
constexpr float PANEL_W = 560.0f;
constexpr float PANEL_H = 420.0f;

std::string upper(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    return s;
}
}

GameOverState::GameOverState(RunSummary summary)
    : m_summary(std::move(summary)) {
    m_items.emplace_back("RETRY LEVEL");
    m_items.emplace_back("QUIT TO MENU");
}

void GameOverState::enter() {
    std::cout << "Entering GameOverState (score " << m_summary.score << ")" << std::endl;

    SoundManager::getInstance().stopMusic();
    SoundManager::getInstance().playSound("game_over");

    // Drop whatever was held during gameplay. Held-key state is only cleared on
    // focus loss, so without this a key held through the death carries into this
    // screen and, with key repeat on, keeps arriving as fresh presses.
    InputManager::getInstance().clearHeldKeys();

    // A finished run is the only point at which a score is final, so this is
    // where the high-score table is written — unless Debug > Cheats was used
    // during it. A demo take with infinite lives is not a score, and letting one
    // into saves/highscores.json would push a real run off the bottom of the
    // table permanently. PlayingState::exit() keeps the taint flag alive
    // precisely so this check can still see it from here.
    if (Game::getInstance().debugCheats().tainted()) {
        std::cout << "[GameOverState] Cheats were used this run; not recording a high score."
                  << std::endl;
        return;
    }

    HighScoreEntry entry;
    entry.score      = m_summary.score;
    entry.coins      = m_summary.coins;
    entry.starCoins  = m_summary.starCoins;
    entry.character  = m_summary.characterName;
    entry.levelName  = m_summary.isEndless ? "Endless"
                     : m_summary.isProcedural ? "Procedural"
                                              : LevelCatalog::nameFor(m_summary.levelIndex);
    m_madeHighScore = Serializer::recordHighScore(entry);
}

void GameOverState::exit() {
    std::cout << "Exiting GameOverState" << std::endl;
}

void GameOverState::activateSelection() {
    if (m_dismissed) return;
    m_dismissed = true;

    if (m_selected == 0) {
        // Rebuild the same level with the same character. The lost run's score
        // is gone deliberately — a retry is a fresh attempt.
        // Same mode, too: retrying a Shadow Chase used to drop the player into
        // an ordinary single-player level, because the mode was not part of
        // what a run summary remembered.
        Game::getInstance().changeState(std::make_unique<PlayingState>(
            false, m_summary.isProcedural, m_summary.generatorConfig,
            m_summary.characterIndex, m_summary.levelIndex, m_summary.match,
            m_summary.isEndless));
    } else {
        Game::getInstance().changeState(std::make_unique<MenuState>());
    }
}

void GameOverState::handleInput(const sf::Event& event) {
    const auto* keyPressed = event.getIf<sf::Event::KeyPressed>();
    if (!keyPressed) return;

    // Ignore everything for the first fraction of a second.
    //
    // Nothing disables SFML's key repeat and nothing clears held keys on a state
    // change, so a player still holding jump as they died — Space for Player 1 —
    // got activateSelection() on the very first frame this screen existed. The
    // panel was dismissed before it was ever drawn, which is what "death screen
    // too short / input makes it skip" describes: the screen was not short, it
    // was skipped.
    if (m_elapsed < kInputLockout) return;

    using Key = sf::Keyboard::Key;

    switch (keyPressed->code) {
        case Key::Up:
        case Key::W:
            // Up used to fall through into Down and move the cursor *forwards*.
            m_selected = (m_selected - 1 + static_cast<int>(m_items.size())) %
                         static_cast<int>(m_items.size());
            break;
        case Key::Down:
        case Key::S:
            m_selected = (m_selected + 1) % static_cast<int>(m_items.size());
            break;
        case Key::Enter:
        case Key::Space:
            activateSelection();
            break;
        case Key::Escape:
            if (!m_dismissed) {
                m_dismissed = true;
                Game::getInstance().changeState(std::make_unique<MenuState>());
            }
            break;
        default:
            break;
    }
}

void GameOverState::update(float dt) {
    m_elapsed += dt;
}

void GameOverState::render(sf::RenderTarget& target) {
    target.setView(target.getDefaultView());

    // This state owns the screen, so it paints its own background rather than
    // relying on whatever the previous state left in the frame buffer.
    UiRenderer::drawDimmer(target, 255, sf::Color(18, 8, 24));

    const float px = (Constants::WINDOW_WIDTH - PANEL_W) * 0.5f;
    const float py = (Constants::WINDOW_HEIGHT - PANEL_H) * 0.5f;
    UiRenderer::drawPanel(target, {px, py}, {PANEL_W, PANEL_H},
                          sf::Color(0, 0, 0, 235), sf::Color(220, 60, 60));

    const float centerX = Constants::WINDOW_WIDTH * 0.5f;

    // The headline names the mode's own ending. "GAME OVER" is right for a solo
    // run and wrong for every other case: losing a versus match is a result, not
    // a failure, and being caught by your own shadow is the entire point of the
    // mode rather than an anonymous death.
    std::string headline = "GAME OVER";
    sf::Color headlineColor(230, 60, 60);
    if (m_summary.caughtByShadow) {
        headline = "CAUGHT";
        headlineColor = sf::Color(190, 120, 255);
    } else if (m_summary.match.isVersus()) {
        const bool cpu = m_summary.match.isCpuOpponent();
        headline = (m_summary.score > m_summary.opponentScore)
                       ? "P1 WINS"
                       : (m_summary.score < m_summary.opponentScore
                              ? (cpu ? "CPU WINS" : "P2 WINS")
                              : "DRAW");
        headlineColor = sf::Color(255, 216, 0);
    } else if (m_summary.match.isCoop()) {
        headline = "TEAM DOWN";
        headlineColor = sf::Color(120, 200, 255);
    }
    UiRenderer::drawShadowedText(target, headline, {centerX, py + 40.0f}, 32,
                                 headlineColor, true);

    const std::string levelName = m_summary.isEndless
        ? ("ENDLESS - " + std::to_string(m_summary.endlessDistanceTiles) + "M")
        : m_summary.isProcedural
            ? "PROCEDURAL"
            : upper(LevelCatalog::nameFor(m_summary.levelIndex));

    float y = py + 82.0f;
    // How the run ended, in the words PlayingState used when it ended it.
    if (!m_summary.cause.empty()) {
        UiRenderer::drawText(target, upper(m_summary.cause), {centerX, y}, 12,
                             sf::Color(200, 160, 160), true);
    }

    y = py + 120.0f;
    UiRenderer::drawText(target, upper(m_summary.characterName) + "   WORLD " + levelName,
                         {centerX, y}, 12, sf::Color(180, 180, 180), true);
    y += 40.0f;
    UiRenderer::drawText(target, "SCORE  " + std::to_string(m_summary.score),
                         {centerX, y}, 16, sf::Color(255, 255, 255), true);
    y += 32.0f;
    UiRenderer::drawText(target, "COINS  " + std::to_string(m_summary.coins) +
                                 "    STARS  " + std::to_string(m_summary.starCoins) + "/3",
                         {centerX, y}, 12, sf::Color(255, 216, 0), true);

    // Death counter, read from the tracker that has always counted PlayerDied.
    y += 28.0f;
    UiRenderer::drawText(target, "DEATHS  " +
                         std::to_string(StatisticsTracker::getInstance().getStats().totalDeaths),
                         {centerX, y}, 12, sf::Color(200, 120, 120), true);

    if (m_madeHighScore) {
        y += 34.0f;
        // Blink so it is noticeable without any extra art.
        if (std::fmod(m_elapsed, 1.0f) < 0.6f) {
            UiRenderer::drawText(target, "NEW HIGH SCORE!", {centerX, y}, 12,
                                 sf::Color(120, 255, 140), true);
        }
    }

    UiRenderer::drawMenuItems(target, m_items, m_selected,
                              {px + 140.0f, py + PANEL_H - 130.0f}, 40.0f, 14, 0.0f, m_elapsed);

    UiRenderer::drawText(target, "UP/DOWN  SELECT   ENTER  CONFIRM",
                         {centerX, py + PANEL_H - 34.0f}, 10, sf::Color(150, 150, 150), true);
}
