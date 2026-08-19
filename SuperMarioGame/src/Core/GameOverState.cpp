#include "Core/GameOverState.hpp"
#include "Core/Game.hpp"
#include "Core/MenuState.hpp"
#include "Core/PlayingState.hpp"
#include "Core/SoundManager.hpp"
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

    // A finished run is the only point at which a score is final, so this is
    // where the high-score table is written.
    HighScoreEntry entry;
    entry.score      = m_summary.score;
    entry.coins      = m_summary.coins;
    entry.starCoins  = m_summary.starCoins;
    entry.character  = m_summary.characterName;
    entry.levelName  = m_summary.isProcedural ? "Procedural"
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
        Game::getInstance().changeState(std::make_unique<PlayingState>(
            false, m_summary.isProcedural, m_summary.generatorConfig,
            m_summary.characterIndex, m_summary.levelIndex));
    } else {
        Game::getInstance().changeState(std::make_unique<MenuState>());
    }
}

void GameOverState::handleInput(const sf::Event& event) {
    const auto* keyPressed = event.getIf<sf::Event::KeyPressed>();
    if (!keyPressed) return;
    using Key = sf::Keyboard::Key;

    switch (keyPressed->code) {
        case Key::Up:
        case Key::W:
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
    UiRenderer::drawShadowedText(target, "GAME OVER", {centerX, py + 40.0f}, 32,
                                 sf::Color(230, 60, 60), true);

    const std::string levelName = m_summary.isProcedural
        ? "PROCEDURAL"
        : upper(LevelCatalog::nameFor(m_summary.levelIndex));

    float y = py + 120.0f;
    UiRenderer::drawText(target, upper(m_summary.characterName) + "   WORLD " + levelName,
                         {centerX, y}, 12, sf::Color(180, 180, 180), true);
    y += 40.0f;
    UiRenderer::drawText(target, "SCORE  " + std::to_string(m_summary.score),
                         {centerX, y}, 16, sf::Color(255, 255, 255), true);
    y += 32.0f;
    UiRenderer::drawText(target, "COINS  " + std::to_string(m_summary.coins) +
                                 "    STARS  " + std::to_string(m_summary.starCoins) + "/3",
                         {centerX, y}, 12, sf::Color(255, 216, 0), true);

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
