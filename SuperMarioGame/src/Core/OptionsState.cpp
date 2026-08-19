#include "Core/OptionsState.hpp"
#include "Core/Game.hpp"
#include "Core/InputManager.hpp"
#include "Core/SoundManager.hpp"
#include "Utils/Constants.hpp"

#include <SFML/Window/Event.hpp>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <iostream>

namespace {

constexpr float PANEL_W = 760.0f;
constexpr float PANEL_H = 560.0f;

const char* const kDifficulties[] = {"easy", "normal", "hard"};
constexpr int kDifficultyCount = 3;

int difficultyIndex(const std::string& name) {
    for (int i = 0; i < kDifficultyCount; ++i) {
        if (name == kDifficulties[i]) return i;
    }
    return 1; // normal
}

// "jump" -> "JUMP", "groundpound" -> "GROUND POUND"
std::string prettyAction(const std::string& action) {
    if (action == "groundpound") return "GROUND POUND";
    std::string out = action;
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    return out;
}

} // namespace

void OptionsState::enter() {
    std::cout << "Entering OptionsState" << std::endl;
    buildRows();
    m_highScores = Serializer::loadHighScores();
}

void OptionsState::exit() {
    std::cout << "Exiting OptionsState" << std::endl;
    // Settings are written to saves/config.json on shutdown; nothing to flush
    // here beyond what the setters already applied live.
}

void OptionsState::buildRows() {
    m_rows.clear();
    m_rows.push_back({RowKind::Volume,     "MUSIC VOLUME", "", true});
    m_rows.push_back({RowKind::Volume,     "SFX VOLUME",   "", false});
    m_rows.push_back({RowKind::Difficulty, "DIFFICULTY",   "", false});
    m_rows.push_back({RowKind::Toggle,     "COLORBLIND",   "", false});

    // Controls. The action ids are exactly InputManager's bindable actions —
    // anything else is silently ignored by applyBindings().
    for (const char* action : {"left", "right", "jump", "run", "crouch", "fire", "groundpound"}) {
        m_rows.push_back({RowKind::Binding, prettyAction(action), action, false});
    }

    m_rows.push_back({RowKind::Action, "BACK", "", false});
    m_selected = std::min(m_selected, static_cast<int>(m_rows.size()) - 1);
}

std::string OptionsState::valueTextFor(const Row& row) const {
    Game& game = Game::getInstance();
    switch (row.kind) {
        case RowKind::Volume: {
            const float v = row.isMusic ? game.getMusicVolume() : game.getSfxVolume();
            return std::to_string(static_cast<int>(std::lround(v))) + "%";
        }
        case RowKind::Difficulty: {
            std::string d = game.getDifficulty();
            std::transform(d.begin(), d.end(), d.begin(),
                           [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
            return d;
        }
        case RowKind::Toggle:
            return game.getColorblindMode() ? "ON" : "OFF";
        case RowKind::Binding: {
            const std::string key = InputManager::getInstance().getBoundKeyName(row.actionId, 0);
            return key.empty() ? "-" : key;
        }
        case RowKind::Action:
        default:
            return "";
    }
}

void OptionsState::adjustSelected(int direction) {
    if (m_rows.empty()) return;
    const Row& row = m_rows[static_cast<std::size_t>(m_selected)];
    Game& game = Game::getInstance();

    switch (row.kind) {
        case RowKind::Volume: {
            const float current = row.isMusic ? game.getMusicVolume() : game.getSfxVolume();
            const float next = std::clamp(current + static_cast<float>(direction) * 5.0f, 0.0f, 100.0f);
            if (row.isMusic) game.setMusicVolume(next);
            else             game.setSfxVolume(next);
            // Play a blip on SFX changes so the new level is audible immediately.
            if (!row.isMusic) SoundManager::getInstance().playSound("coin");
            break;
        }
        case RowKind::Difficulty: {
            const int idx = (difficultyIndex(game.getDifficulty()) + direction + kDifficultyCount) % kDifficultyCount;
            game.setDifficulty(kDifficulties[idx]);
            break;
        }
        case RowKind::Toggle:
            game.setColorblindMode(!game.getColorblindMode());
            break;
        default:
            break;
    }
}

void OptionsState::activateSelected() {
    if (m_rows.empty()) return;
    const Row& row = m_rows[static_cast<std::size_t>(m_selected)];

    switch (row.kind) {
        case RowKind::Binding:
            // Swallow the next key press and bind it to this action.
            m_awaitingBindingRow = m_selected;
            break;
        case RowKind::Toggle:
        case RowKind::Difficulty:
            adjustSelected(1);
            break;
        case RowKind::Action:
            close();
            break;
        default:
            break;
    }
}

void OptionsState::close() {
    if (m_closing) return;
    m_closing = true;
    Game::getInstance().popState();
}

void OptionsState::handleInput(const sf::Event& event) {
    const auto* keyPressed = event.getIf<sf::Event::KeyPressed>();
    if (!keyPressed) return;
    using Key = sf::Keyboard::Key;

    // Rebinding mode consumes everything: the very next key becomes the binding.
    if (m_awaitingBindingRow >= 0) {
        if (keyPressed->code == Key::Escape) {
            m_awaitingBindingRow = -1;   // cancelled, binding unchanged
            return;
        }
        const std::string keyText = InputManager::keyName(keyPressed->code);
        if (!keyText.empty()) {
            const Row& row = m_rows[static_cast<std::size_t>(m_awaitingBindingRow)];
            // setKeyBinding both applies the binding live and stores it for
            // saveSettings() to persist at shutdown.
            Game::getInstance().setKeyBinding(row.actionId, keyText);
        } else {
            std::cerr << "[OptionsState] That key has no name in the binding table; ignored."
                      << std::endl;
        }
        m_awaitingBindingRow = -1;
        return;
    }

    switch (keyPressed->code) {
        case Key::Tab:
            m_page = (m_page == Page::Settings) ? Page::HighScores : Page::Settings;
            break;
        case Key::Escape:
        case Key::Backspace:
            close();
            break;
        case Key::Up:
        case Key::W:
            if (m_page == Page::Settings && !m_rows.empty()) {
                const int n = static_cast<int>(m_rows.size());
                m_selected = (m_selected - 1 + n) % n;
            }
            break;
        case Key::Down:
        case Key::S:
            if (m_page == Page::Settings && !m_rows.empty()) {
                const int n = static_cast<int>(m_rows.size());
                m_selected = (m_selected + 1) % n;
            }
            break;
        case Key::Left:
        case Key::A:
            if (m_page == Page::Settings) adjustSelected(-1);
            break;
        case Key::Right:
        case Key::D:
            if (m_page == Page::Settings) adjustSelected(1);
            break;
        case Key::Enter:
        case Key::Space:
            if (m_page == Page::Settings) activateSelected();
            else                          close();
            break;
        default:
            break;
    }
}

void OptionsState::update(float dt) {
    m_elapsed += dt;
}

void OptionsState::render(sf::RenderTarget& target) {
    target.setView(target.getDefaultView());

    UiRenderer::drawDimmer(target, 190);

    const float px = (Constants::WINDOW_WIDTH - PANEL_W) * 0.5f;
    const float py = (Constants::WINDOW_HEIGHT - PANEL_H) * 0.5f;
    UiRenderer::drawPanel(target, {px, py}, {PANEL_W, PANEL_H});

    const float centerX = Constants::WINDOW_WIDTH * 0.5f;
    const bool settings = (m_page == Page::Settings);

    UiRenderer::drawShadowedText(target, settings ? "OPTIONS" : "HIGH SCORES",
                                 {centerX, py + 28.0f}, 24, sf::Color(255, 216, 0), true);
    UiRenderer::drawText(target, "TAB  SWITCH PAGE", {centerX, py + 64.0f}, 10,
                         sf::Color(150, 150, 150), true);

    if (settings) {
        std::vector<UiMenuItem> items;
        items.reserve(m_rows.size());
        for (std::size_t i = 0; i < m_rows.size(); ++i) {
            const Row& row = m_rows[i];
            std::string value = valueTextFor(row);
            if (m_awaitingBindingRow == static_cast<int>(i)) {
                value = "PRESS KEY";
            }
            items.emplace_back(row.label, value);
        }

        UiRenderer::drawMenuItems(target, items, m_selected,
                                  {px + 60.0f, py + 100.0f}, 34.0f, 13,
                                  px + PANEL_W - 200.0f, m_elapsed);

        const char* hint = (m_awaitingBindingRow >= 0)
            ? "PRESS A KEY TO BIND   ESC CANCEL"
            : "LEFT/RIGHT ADJUST   ENTER REBIND   ESC BACK";
        UiRenderer::drawText(target, hint, {centerX, py + PANEL_H - 36.0f}, 10,
                             sf::Color(160, 160, 160), true);
        return;
    }

    // --- High scores page ---
    if (m_highScores.empty()) {
        UiRenderer::drawText(target, "NO SCORES RECORDED YET", {centerX, py + 220.0f}, 14,
                             sf::Color(180, 180, 180), true);
    } else {
        UiRenderer::drawText(target, "#   SCORE      LEVEL     CHAR    STARS",
                             {px + 60.0f, py + 110.0f}, 12, sf::Color(120, 200, 255));

        float y = py + 150.0f;
        for (std::size_t i = 0; i < m_highScores.size(); ++i) {
            const HighScoreEntry& e = m_highScores[i];
            std::string charName = e.character;
            std::transform(charName.begin(), charName.end(), charName.begin(),
                           [](unsigned char c) { return static_cast<char>(std::toupper(c)); });

            const sf::Color rowColor = (i == 0) ? sf::Color(255, 216, 0) : sf::Color(225, 225, 225);
            UiRenderer::drawText(target, std::to_string(i + 1), {px + 60.0f, y}, 12, rowColor);
            UiRenderer::drawText(target, std::to_string(e.score), {px + 110.0f, y}, 12, rowColor);
            UiRenderer::drawText(target, e.levelName, {px + 290.0f, y}, 12, rowColor);
            UiRenderer::drawText(target, charName, {px + 420.0f, y}, 12, rowColor);
            UiRenderer::drawText(target, std::to_string(e.starCoins) + "/3", {px + 560.0f, y}, 12, rowColor);
            y += 32.0f;
        }
    }

    UiRenderer::drawText(target, "ESC BACK", {centerX, py + PANEL_H - 36.0f}, 10,
                         sf::Color(160, 160, 160), true);
}
