#include "Core/VictoryState.hpp"
#include "Core/Game.hpp"
#include "Core/SoundManager.hpp"
#include "Utils/Constants.hpp"
#include "Utils/Serializer.hpp"

#include <SFML/Window/Event.hpp>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <iostream>

namespace {
constexpr float PANEL_W = 560.0f;
constexpr float PANEL_H = 400.0f;
// Seconds the time bonus takes to roll into the score readout.
constexpr float TALLY_DURATION = 1.4f;

std::string upper(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    return s;
}
}

VictoryState::VictoryState(LevelSummary summary, std::function<void()> onContinue)
    : m_summary(std::move(summary)), m_onContinue(std::move(onContinue)) {}

void VictoryState::enter() {
    std::cout << "Entering VictoryState (" << m_summary.levelName << ")" << std::endl;

    SoundManager::getInstance().stopMusic();
    SoundManager::getInstance().playSound(m_summary.isFinalLevel ? "world_clear" : "stage_clear");

    // Finishing the campaign ends the run, so that is a final score worth
    // recording. Clearing a single level is not — the run continues.
    if (m_summary.isFinalLevel) {
        HighScoreEntry entry;
        entry.score     = m_summary.finalScore;
        entry.coins     = m_summary.coins;
        entry.starCoins = static_cast<int>(std::count(m_summary.starCoins.begin(),
                                                      m_summary.starCoins.end(), true));
        entry.character = m_summary.characterName;
        entry.levelName = m_summary.levelName;
        Serializer::recordHighScore(entry);
    }
}

void VictoryState::exit() {
    std::cout << "Exiting VictoryState" << std::endl;
}

void VictoryState::dismiss() {
    if (m_dismissed) return;
    m_dismissed = true;

    // Pop first, then run the continuation: advanceToNextLevel() reloads the
    // level underneath us, and it must not do that while this state is still on
    // top of the stack. The pop is deferred, but so is anything the callback
    // queues, and the queue preserves order.
    Game::getInstance().popState();
    if (m_onContinue) m_onContinue();
}

void VictoryState::handleInput(const sf::Event& event) {
    const auto* keyPressed = event.getIf<sf::Event::KeyPressed>();
    if (!keyPressed) return;

    using Key = sf::Keyboard::Key;
    if (keyPressed->code == Key::Enter || keyPressed->code == Key::Space ||
        keyPressed->code == Key::Escape) {
        // A press during the tally skips it rather than dismissing, so the
        // player cannot accidentally miss the summary with a held key.
        if (m_tallied < static_cast<float>(m_summary.timeBonus)) {
            m_tallied = static_cast<float>(m_summary.timeBonus);
            return;
        }
        dismiss();
    }
}

void VictoryState::update(float dt) {
    m_elapsed += dt;

    const float target = static_cast<float>(m_summary.timeBonus);
    if (m_tallied < target) {
        m_tallied = std::min(target, m_tallied + (target / TALLY_DURATION) * dt);
        if (m_tallied >= target) {
            SoundManager::getInstance().playSound("coin");
        }
    }
}

void VictoryState::render(sf::RenderTarget& target) {
    target.setView(target.getDefaultView());

    UiRenderer::drawDimmer(target, 170);

    const float px = (Constants::WINDOW_WIDTH - PANEL_W) * 0.5f;
    const float py = (Constants::WINDOW_HEIGHT - PANEL_H) * 0.5f;
    UiRenderer::drawPanel(target, {px, py}, {PANEL_W, PANEL_H},
                          sf::Color(0, 0, 0, 230), sf::Color(120, 255, 140));

    const float centerX = Constants::WINDOW_WIDTH * 0.5f;

    UiRenderer::drawShadowedText(target,
                                 m_summary.isFinalLevel ? "CAMPAIGN CLEAR!" : "LEVEL CLEAR!",
                                 {centerX, py + 34.0f}, 24, sf::Color(120, 255, 140), true);
    UiRenderer::drawText(target, upper(m_summary.characterName) + "   WORLD " + upper(m_summary.levelName),
                         {centerX, py + 76.0f}, 12, sf::Color(180, 180, 180), true);

    const int shownBonus = static_cast<int>(std::lround(m_tallied));
    const int shownScore = m_summary.scoreBeforeBonus + shownBonus;

    float y = py + 126.0f;
    UiRenderer::drawText(target, "TIME LEFT", {px + 70.0f, y}, 13, sf::Color(200, 200, 200));
    UiRenderer::drawText(target, std::to_string(m_summary.timeRemaining), {px + 380.0f, y}, 13,
                         sf::Color(255, 255, 255));
    y += 36.0f;
    UiRenderer::drawText(target, "TIME BONUS", {px + 70.0f, y}, 13, sf::Color(200, 200, 200));
    UiRenderer::drawText(target, "+" + std::to_string(shownBonus), {px + 380.0f, y}, 13,
                         sf::Color(255, 216, 0));
    y += 36.0f;
    UiRenderer::drawText(target, "COINS", {px + 70.0f, y}, 13, sf::Color(200, 200, 200));
    UiRenderer::drawText(target, std::to_string(m_summary.coins), {px + 380.0f, y}, 13,
                         sf::Color(255, 255, 255));
    y += 44.0f;
    UiRenderer::drawText(target, "SCORE", {px + 70.0f, y}, 16, sf::Color(255, 255, 255));
    UiRenderer::drawText(target, std::to_string(shownScore), {px + 380.0f, y}, 16,
                         sf::Color(255, 255, 255));

    // Star coins: filled for collected, hollow for missed.
    y += 48.0f;
    UiRenderer::drawText(target, "STAR COINS", {px + 70.0f, y}, 13, sf::Color(200, 200, 200));
    for (std::size_t i = 0; i < m_summary.starCoins.size(); ++i) {
        const bool got = m_summary.starCoins[i];
        UiRenderer::drawText(target, got ? "*" : "-",
                             {px + 380.0f + static_cast<float>(i) * 34.0f, y}, 16,
                             got ? sf::Color(255, 216, 0) : sf::Color(90, 90, 90));
    }

    const bool tallyDone = m_tallied >= static_cast<float>(m_summary.timeBonus);
    if (tallyDone && std::fmod(m_elapsed, 1.0f) < 0.65f) {
        const char* prompt = m_summary.isFinalLevel ? "PRESS ENTER FOR THE MENU"
                                                    : "PRESS ENTER TO CONTINUE";
        UiRenderer::drawText(target, prompt, {centerX, py + PANEL_H - 40.0f}, 11,
                             sf::Color(235, 235, 235), true);
    }
}
