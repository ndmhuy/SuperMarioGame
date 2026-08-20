#include "Core/OptionsState.hpp"
#include <utility>
#include <cstdio>
#include "Core/StatisticsTracker.hpp"
#include "Core/AchievementManager.hpp"
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
// Achievement rows that fit inside the panel; the rest scroll.
constexpr int kAchievementRows = 9;

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

    if (m_page == Page::Controls) {
        // The action ids are exactly InputManager's bindable actions — anything
        // else is silently ignored by applyBindings().
        //
        // Both pads are listed. Player 2's block is what makes the arrows/M/N
        // layout discoverable and changeable; before this, the only way to find
        // out what Player 2's keys were was to read InputManager.cpp.
        for (int pad = 0; pad < 2; ++pad) {
            Row header{RowKind::Action, pad == 0 ? "-- PLAYER 1 --" : "-- PLAYER 2 --", "", false};
            header.selectable = false;
            m_rows.push_back(header);
            for (const char* action : {"left", "right", "jump", "run", "crouch", "fire", "groundpound"}) {
                Row row{RowKind::Binding, prettyAction(action), action, false};
                row.playerIndex = pad;
                m_rows.push_back(row);
            }
        }
        // A player who binds a control to a key they cannot find again needs a
        // way back that is not "edit config.json by hand".
        m_rows.push_back({RowKind::Action, "RESET CONTROLS", "", false});
        m_rows.push_back({RowKind::Action, "BACK", "", false});
    } else {
        m_rows.push_back({RowKind::Volume,     "MUSIC VOLUME", "", true});
        m_rows.push_back({RowKind::Volume,     "SFX VOLUME",   "", false});
        m_rows.push_back({RowKind::Difficulty, "DIFFICULTY",   "", false});
        m_rows.push_back({RowKind::Toggle,     "COLORBLIND",   "", false});
        m_rows.push_back({RowKind::Action,     "BACK",         "", false});
    }

    m_selected = std::clamp(m_selected, 0, static_cast<int>(m_rows.size()) - 1);
    // The clamp can land on a caption; step off it.
    if (!m_rows[static_cast<std::size_t>(m_selected)].selectable) moveRow(1);
}

void OptionsState::moveRow(int delta) {
    if (m_rows.empty()) return;
    const int n = static_cast<int>(m_rows.size());
    const int step = (delta >= 0) ? 1 : -1;
    // Step over the "-- PLAYER 1 --" / "-- PLAYER 2 --" captions. A caption the
    // cursor can land on looks like a row whose Enter key is broken.
    for (int i = 0; i < n; ++i) {
        m_selected = (m_selected + step + n) % n;
        if (m_rows[static_cast<std::size_t>(m_selected)].selectable) return;
    }
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
            const std::string key =
                InputManager::getInstance().getBoundKeyName(row.actionId, row.playerIndex);
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
            if (row.label == "RESET CONTROLS") {
                // Applied and persisted through the same path a manual rebind
                // takes, so the defaults cannot drift from what the game does.
                // Both pads: resetting one and leaving the other half-rebound is
                // not what "reset controls" means to anyone reading the button.
                for (int pad = 0; pad < 2; ++pad) {
                    const auto defaults =
                        InputManager::getInstance().resetBindingsToDefaults(pad);
                    for (const auto& [action, key] : defaults) {
                        Game::getInstance().setKeyBinding(action, key, pad);
                    }
                }
                m_notice = "BOTH PADS RESET TO DEFAULTS";
                m_noticeTimer = 2.5f;
            } else {
                close();
            }
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

            // Say so when the key was already taken. applyBindings swaps the two
            // rather than orphaning the other action, but silently moving a
            // control the player did not mention is worse than telling them.
            // Conflicts are resolved per pad. The two players are at the same
            // keyboard, but their tables are independent by design: both may
            // legitimately bind "left" to the same key in a single-player run,
            // and only the pad being edited should be rearranged.
            const int pad = row.playerIndex;
            const std::string previousOwner =
                InputManager::getInstance().getActionForKey(keyText, pad);
            // setKeyBinding both applies the binding live and stores it for
            // saveSettings() to persist at shutdown.
            Game::getInstance().setKeyBinding(row.actionId, keyText, pad);
            if (!previousOwner.empty() && previousOwner != row.actionId) {
                m_notice = keyText + " SWAPPED WITH " + prettyAction(previousOwner);
                m_noticeTimer = 2.5f;
                // The displaced action moved to the key this one vacated, and
                // Game must persist that too or it reverts on the next launch.
                Game::getInstance().setKeyBinding(
                    previousOwner,
                    InputManager::getInstance().getBoundKeyName(previousOwner, pad), pad);
            }
        } else {
            std::cerr << "[OptionsState] That key has no name in the binding table; ignored."
                      << std::endl;
        }
        m_awaitingBindingRow = -1;
        return;
    }

    switch (keyPressed->code) {
        case Key::Tab: {
            // Settings -> Controls -> High Scores -> Statistics -> Achievements.
            constexpr int kPageCount = 5;
            m_page = static_cast<Page>((static_cast<int>(m_page) + 1) % kPageCount);
            m_achievementScroll = 0;
            // Settings and Controls own different row lists, so the list has to
            // be rebuilt when the page changes rather than only on enter().
            m_selected = 0;
            buildRows();
            break;
        }
        case Key::Escape:
        case Key::Backspace:
            close();
            break;
        case Key::Up:
        case Key::W:
            if (m_page == Page::Settings && !m_rows.empty()) {
                moveRow(-1);
            } else if (m_page == Page::Achievements) {
                m_achievementScroll = std::max(0, m_achievementScroll - 1);
            }
            break;
        case Key::Down:
        case Key::S:
            if (m_page == Page::Settings && !m_rows.empty()) {
                moveRow(1);
            } else if (m_page == Page::Achievements) {
                const int total = static_cast<int>(
                    AchievementManager::getInstance().getAchievements().size());
                m_achievementScroll = std::min(std::max(0, total - kAchievementRows),
                                               m_achievementScroll + 1);
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
    if (m_noticeTimer > 0.0f) {
        m_noticeTimer -= dt;
        if (m_noticeTimer <= 0.0f) m_notice.clear();
    }
}

void OptionsState::render(sf::RenderTarget& target) {
    target.setView(target.getDefaultView());

    UiRenderer::drawDimmer(target, 190);

    const float px = (Constants::WINDOW_WIDTH - PANEL_W) * 0.5f;
    const float py = (Constants::WINDOW_HEIGHT - PANEL_H) * 0.5f;
    UiRenderer::drawPanel(target, {px, py}, {PANEL_W, PANEL_H});

    const float centerX = Constants::WINDOW_WIDTH * 0.5f;
    // Both list pages draw through the same block below; they differ only in
    // which rows buildRows() produced and how tightly they have to be packed.
    const bool settings = (m_page == Page::Settings || m_page == Page::Controls);

    const char* title = "OPTIONS";
    switch (m_page) {
        case Page::Controls:     title = "CONTROLS";     break;
        case Page::HighScores:   title = "HIGH SCORES";  break;
        case Page::Statistics:   title = "STATISTICS";   break;
        case Page::Achievements: title = "ACHIEVEMENTS"; break;
        case Page::Settings:     break;
    }
    UiRenderer::drawShadowedText(target, title, {centerX, py + 28.0f}, 24,
                                 sf::Color(255, 216, 0), true);

    // Which of the four pages is showing, so Tab is discoverable rather than
    // something you have to already know about.
    const char* const kPageNames[] = {"OPTIONS", "KEYS", "SCORES", "STATS", "AWARDS"};
    std::string tabs;
    for (int i = 0; i < 5; ++i) {
        if (i) tabs += "   ";
        tabs += (static_cast<int>(m_page) == i) ? std::string("[") + kPageNames[i] + "]"
                                                : std::string(" ") + kPageNames[i] + " ";
    }
    UiRenderer::drawText(target, tabs, {centerX, py + 62.0f}, 10,
                         sf::Color(150, 150, 150), true);
    UiRenderer::drawText(target, "TAB  SWITCH PAGE", {centerX, py + 78.0f}, 9,
                         sf::Color(110, 110, 110), true);

    if (m_page == Page::Statistics)   { renderStatisticsPage(target);   return; }
    if (m_page == Page::Achievements) { renderAchievementsPage(target); return; }

    if (settings) {
        std::vector<UiMenuItem> items;
        items.reserve(m_rows.size());
        for (std::size_t i = 0; i < m_rows.size(); ++i) {
            const Row& row = m_rows[i];
            std::string value = valueTextFor(row);
            if (m_awaitingBindingRow == static_cast<int>(i)) {
                value = "PRESS KEY";
            }
            // Captions render as disabled rows, which is what greys them out.
            items.emplace_back(row.label, value, row.selectable);
        }

        // Eighteen rows on the controls page against five on settings, in the
        // same 560px panel: the row height has to come from the row count or the
        // list runs out through the bottom of the frame.
        const float rowHeight = (m_page == Page::Controls) ? 24.0f : 34.0f;
        UiRenderer::drawMenuItems(target, items, m_selected,
                                  {px + 60.0f, py + 100.0f}, rowHeight, 13,
                                  px + PANEL_W - 200.0f, m_elapsed);

        if (!m_notice.empty()) {
            UiRenderer::drawText(target, m_notice, {centerX, py + PANEL_H - 60.0f}, 10,
                                 sf::Color(120, 255, 140), true);
        }

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

namespace {

// "3m 04s", or "1h 12m" once a run gets long. Raw seconds read as noise.
std::string formatDuration(float seconds) {
    const int total = std::max(0, static_cast<int>(seconds));
    const int hours = total / 3600;
    const int mins  = (total % 3600) / 60;
    const int secs  = total % 60;
    char buffer[32];
    if (hours > 0) std::snprintf(buffer, sizeof(buffer), "%dh %02dm", hours, mins);
    else           std::snprintf(buffer, sizeof(buffer), "%dm %02ds", mins, secs);
    return buffer;
}

} // namespace

void OptionsState::renderStatisticsPage(sf::RenderTarget& target) const {
    // The tracker has been counting all along — every one of these numbers was
    // already being maintained and persisted, with nowhere in the game to see it
    // except a collapsed ImGui panel in the dev overlay.
    const GameStatistics& stats = StatisticsTracker::getInstance().getStats();

    const float px = (Constants::WINDOW_WIDTH - PANEL_W) * 0.5f;
    const float py = (Constants::WINDOW_HEIGHT - PANEL_H) * 0.5f;
    const float centerX = Constants::WINDOW_WIDTH * 0.5f;

    const std::vector<std::pair<std::string, std::string>> lines = {
        {"ENEMIES DEFEATED", std::to_string(stats.totalEnemiesDefeated)},
        {"COINS COLLECTED",  std::to_string(stats.totalCoinsCollected)},
        {"DEATHS",           std::to_string(stats.totalDeaths)},
        {"BEST COMBO",       std::to_string(stats.highestCombo)},
        {"TIME PLAYED",      formatDuration(stats.totalTimePlayed)},
    };

    float y = py + 130.0f;
    for (const auto& [label, value] : lines) {
        UiRenderer::drawText(target, label, {px + 70.0f, y}, 13, sf::Color(220, 220, 220));
        UiRenderer::drawText(target, value, {px + PANEL_W - 70.0f -
                                             UiRenderer::measureTextWidth(value, 13), y},
                             13, sf::Color(255, 216, 0));
        y += 44.0f;
    }

    // A derived line is worth more than another counter: it says something the
    // raw numbers do not.
    const int runs = std::max(1, stats.totalDeaths);
    const std::string perLife = std::to_string(stats.totalEnemiesDefeated / runs);
    UiRenderer::drawText(target, "ENEMIES PER LIFE LOST", {px + 70.0f, y}, 13,
                         sf::Color(150, 200, 255));
    UiRenderer::drawText(target, perLife, {px + PANEL_W - 70.0f -
                                           UiRenderer::measureTextWidth(perLife, 13), y},
                         13, sf::Color(150, 200, 255));

    UiRenderer::drawText(target, "ESC  BACK", {centerX, py + PANEL_H - 36.0f}, 10,
                         sf::Color(150, 150, 150), true);
}

void OptionsState::renderAchievementsPage(sf::RenderTarget& target) const {
    const auto& achievements = AchievementManager::getInstance().getAchievements();

    const float px = (Constants::WINDOW_WIDTH - PANEL_W) * 0.5f;
    const float py = (Constants::WINDOW_HEIGHT - PANEL_H) * 0.5f;
    const float centerX = Constants::WINDOW_WIDTH * 0.5f;

    int unlocked = 0;
    for (const auto& a : achievements) if (a.unlocked) ++unlocked;

    const std::string progress = std::to_string(unlocked) + " / " +
                                 std::to_string(achievements.size()) + " UNLOCKED";
    UiRenderer::drawText(target, progress, {centerX, py + 104.0f}, 12,
                         sf::Color(120, 255, 140), true);

    // Progress bar: the count alone does not show how close you are.
    const float barW = PANEL_W - 140.0f;
    const float filled = achievements.empty()
        ? 0.0f : barW * (static_cast<float>(unlocked) / static_cast<float>(achievements.size()));
    UiRenderer::drawPanel(target, {px + 70.0f, py + 126.0f}, {barW, 10.0f},
                          sf::Color(40, 40, 40, 255), sf::Color(90, 90, 90, 255));
    if (filled > 0.0f) {
        UiRenderer::drawPanel(target, {px + 70.0f, py + 126.0f}, {filled, 10.0f},
                              sf::Color(120, 255, 140, 220), sf::Color(120, 255, 140, 240));
    }

    const int total = static_cast<int>(achievements.size());
    const int first = std::clamp(m_achievementScroll, 0, std::max(0, total - kAchievementRows));
    const int last  = std::min(total, first + kAchievementRows);

    float y = py + 156.0f;
    for (int i = first; i < last; ++i) {
        const Achievement& a = achievements[static_cast<std::size_t>(i)];
        // A locked achievement still shows its condition: hiding it would make
        // the list a wall of question marks with nothing to aim at.
        const sf::Color colour = a.unlocked ? sf::Color(255, 216, 0) : sf::Color(120, 120, 120);
        UiRenderer::drawText(target, a.unlocked ? "*" : "-", {px + 60.0f, y}, 12, colour);
        UiRenderer::drawText(target, a.name, {px + 84.0f, y}, 12, colour);
        UiRenderer::drawText(target, a.condition, {px + 84.0f, y + 16.0f}, 9,
                             a.unlocked ? sf::Color(170, 170, 170) : sf::Color(90, 90, 90));
        y += 40.0f;
    }

    if (total > kAchievementRows) {
        UiRenderer::drawText(target, "UP/DOWN  SCROLL      ESC  BACK",
                             {centerX, py + PANEL_H - 36.0f}, 10, sf::Color(150, 150, 150), true);
    } else {
        UiRenderer::drawText(target, "ESC  BACK", {centerX, py + PANEL_H - 36.0f}, 10,
                             sf::Color(150, 150, 150), true);
    }
}
