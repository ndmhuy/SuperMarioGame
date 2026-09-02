#include "Core/PauseState.hpp"
#include "Core/Game.hpp"
#include "Core/OptionsState.hpp"
#include "Core/EventBus.hpp"
#include "Core/SoundManager.hpp"
#include "Utils/Constants.hpp"

#include <SFML/Window/Event.hpp>
#include <iostream>

namespace {
constexpr float PANEL_W = 460.0f;
constexpr float PANEL_H = 360.0f;
}

PauseState::PauseState(std::function<void()> onRestartLevel,
                       std::function<void()> onSaveGame,
                       std::function<void()> onQuitToMenu)
    : m_onRestartLevel(std::move(onRestartLevel)),
      m_onSaveGame(std::move(onSaveGame)),
      m_onQuitToMenu(std::move(onQuitToMenu)) {
    m_items.emplace_back("RESUME");
    m_items.emplace_back("SAVE GAME");
    m_items.emplace_back("OPTIONS");
    m_items.emplace_back("RESTART LEVEL");
    m_items.emplace_back("QUIT TO MENU");
}

void PauseState::enter() {
    std::cout << "Entering PauseState" << std::endl;
    // Subscribers (HUD, sound) learn about the pause from the bus rather than
    // from a direct call, same as every other cross-system notification.
    EventBus::getInstance().publish({EventType::PauseToggled, 1});
    SoundManager::getInstance().playSound("pause");
}

void PauseState::exit() {
    std::cout << "Exiting PauseState" << std::endl;
    EventBus::getInstance().publish({EventType::PauseToggled, 0});
}

void PauseState::moveSelection(int delta) {
    const int n = static_cast<int>(m_items.size());
    if (n == 0) return;
    m_selected = (m_selected + delta % n + n) % n;
}

void PauseState::activateSelection() {
    if (m_dismissed) return;

    switch (m_selected) {
        case 0: // Resume
            m_dismissed = true;
            Game::getInstance().popState();
            break;
        case 1: // Save game — stays on the pause menu and reports back
            if (m_onSaveGame) {
                m_onSaveGame();
                m_notice = "SAVED TO SLOT " + std::to_string(Game::getInstance().getActiveSlot());
                m_noticeTimer = 2.5f;
            }
            break;
        case 2: // Options — pushed over the pause menu, pops back to it
            Game::getInstance().pushState(std::make_unique<OptionsState>());
            break;
        case 3: // Restart level
            m_dismissed = true;
            Game::getInstance().popState();
            if (m_onRestartLevel) m_onRestartLevel();
            break;
        case 4: // Quit to menu
            m_dismissed = true;
            Game::getInstance().popState();
            if (m_onQuitToMenu) m_onQuitToMenu();
            break;
        default:
            break;
    }
}

void PauseState::handleInput(const sf::Event& event) {
    const auto* keyPressed = event.getIf<sf::Event::KeyPressed>();
    if (!keyPressed) return;

    using Key = sf::Keyboard::Key;
    switch (keyPressed->code) {
        case Key::Up:
        case Key::W:
            moveSelection(-1);
            break;
        case Key::Down:
        case Key::S:
            moveSelection(1);
            break;
        case Key::Enter:
        case Key::Space:
            activateSelection();
            break;
        case Key::Escape:
        case Key::P:
            // Escape is the pause key, so pressing it again resumes.
            if (!m_dismissed) {
                m_dismissed = true;
                Game::getInstance().popState();
            }
            break;
        default:
            break;
    }
}

void PauseState::update(float dt) {
    m_elapsed += dt;
    if (m_noticeTimer > 0.0f) {
        m_noticeTimer -= dt;
        if (m_noticeTimer <= 0.0f) m_notice.clear();
    }
}

void PauseState::render(sf::RenderTarget& target) {
    // PlayingState has already drawn the world in its own camera view.
    target.setView(target.getDefaultView());

    UiRenderer::drawDimmer(target, 150);

    const float px = (Constants::WINDOW_WIDTH - PANEL_W) * 0.5f;
    const float py = (Constants::WINDOW_HEIGHT - PANEL_H) * 0.5f;
    UiRenderer::drawPanel(target, {px, py}, {PANEL_W, PANEL_H});

    constexpr float TITLE_Y = 36.0f;
    constexpr float LIST_TOP = 118.0f;

    UiRenderer::drawShadowedText(target, "PAUSED",
                                 {Constants::WINDOW_WIDTH * 0.5f, py + TITLE_Y},
                                 28, sf::Color(255, 216, 0), true);

    // The notice goes in the empty band between the title and the first row,
    // not at the bottom of the panel. Down there it landed on top of QUIT TO
    // MENU — the last of five 42px rows ends at py+482 and the notice was drawn
    // at py+290+... i.e. y 470, overlapping in both axes every single time the
    // player saved. This band belongs to nothing else, so a sixth row can be
    // added without recreating the collision.
    if (!m_notice.empty()) {
        UiRenderer::drawTextFitted(target, m_notice,
                                   {Constants::WINDOW_WIDTH * 0.5f,
                                    py + (TITLE_Y + 34.0f + LIST_TOP) * 0.5f},
                                   10, sf::Color(120, 255, 140), PANEL_W - 32.0f, true);
    }

    UiRenderer::drawMenuItems(target, m_items, m_selected,
                              {px + 78.0f, py + LIST_TOP}, 42.0f, 16, 0.0f, m_elapsed,
                              px + PANEL_W);

    UiRenderer::drawText(target, "UP/DOWN  SELECT   ENTER  CONFIRM",
                         {Constants::WINDOW_WIDTH * 0.5f, py + PANEL_H - 44.0f},
                         10, sf::Color(160, 160, 160), true);
}
