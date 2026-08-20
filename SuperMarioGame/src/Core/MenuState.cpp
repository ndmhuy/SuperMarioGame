#include "Core/MenuState.hpp"
#include "Core/AchievementManager.hpp"
#include "Core/CharacterSelectState.hpp"
#include "Core/OptionsState.hpp"
#include "Core/PlayingState.hpp"
#include "Core/Game.hpp"
#include "Core/InputManager.hpp"
#include "Core/SoundManager.hpp"
#include "Utils/Constants.hpp"
#include "Utils/MetaGame.hpp"

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Window/Event.hpp>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace {

// Main-menu rows, in display order.
enum MainRow { ROW_START = 0, ROW_VERSUS, ROW_DAILY, ROW_EDITOR, ROW_GENERATOR,
               ROW_RECORDS, ROW_OPTIONS, ROW_QUIT, ROW_COUNT };

const char* const kThemes[] = {"OVERWORLD", "UNDERGROUND", "CASTLE", "ICE"};
constexpr int kThemeCount = 4;
const char* const kDifficulties[] = {"EASY", "MEDIUM", "HARD"};
constexpr int kDifficultyCount = 3;


std::string percent(float value) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(0) << (value * 100.0f) << "%";
    return ss.str();
}

// The multiplayer modes, in the order the page cycles them. Split-screen is
// absent on purpose rather than shown greyed out: Camera holds a single sf::View
// and is deliberately non-movable, and every screen-space overlay in the game
// would need to learn about viewports before a second view could exist. Offering
// a row that cannot work is worse than not offering it.
const GameMode kMultiplayerModes[] = {
    GameMode::VersusHuman, GameMode::VersusCPU, GameMode::CoopHuman,
    GameMode::ShadowChase
};
constexpr int kMultiplayerModeCount = 4;

// One-line explanation per mode, shown under the list. A player choosing
// "SHADOW CHASE" from a label alone has no way to know what it does.
const char* modeBlurb(GameMode mode) {
    switch (mode) {
        case GameMode::VersusHuman:
            return "TWO HUMANS, ONE SCREEN. STOMP EACH OTHER.";
        case GameMode::VersusCPU:
            return "RACE AND FIGHT AN AI OPPONENT.";
        case GameMode::CoopHuman:
            return "SHARED LIVES. BOUNCE OFF YOUR PARTNER.";
        case GameMode::ShadowChase:
            return "YOUR OWN PATH, 3 SECONDS BEHIND YOU.";
        case GameMode::SinglePlayer:
            break;
    }
    return "";
}

int indexOfMode(GameMode mode) {
    for (int i = 0; i < kMultiplayerModeCount; ++i) {
        if (kMultiplayerModes[i] == mode) return i;
    }
    return 0;
}

} // namespace

void MenuState::enter() {
    std::cout << "Entering MenuState" << std::endl;
    // Pass the registered track key, not a raw path — SoundManager maps
    // "title_screen" to assets/bgm/main_menu.mp3 and resolves it per platform.
    SoundManager::getInstance().playMusic("title_screen");

    m_playerSheet  = SpriteSheet::loadAtlas("player");
    m_scenerySheet = SpriteSheet::loadAtlas("world_scenery_item");
    m_background.setTheme(BackgroundTheme::Overworld);
    m_background.setSpriteSheet(m_scenerySheet.get());
    // No tilemap behind the menu, so the backdrop has to close off its own
    // ground or the hills float over open sky.
    m_background.setDrawGroundBand(true);

    m_mainItems.clear();
    // The New Game+ cycle is shown on the start row, so a player can see the
    // campaign has reset rather than wondering why 1-2 is locked again.
    m_mainItems.emplace_back("START GAME", MetaGame::newGamePlusLabel());
    m_mainItems.emplace_back("MULTIPLAYER", "4 MODES");
    m_mainItems.emplace_back("DAILY CHALLENGE", MetaGame::todaysChallengeName());
    m_mainItems.emplace_back("MAP EDITOR");
    m_mainItems.emplace_back("PROCEDURAL LEVEL");
    // Achievement progress on the row itself, so the player can see there is
    // something to chase without opening the page first.
    {
        const auto& achievements = AchievementManager::getInstance().getAchievements();
        int unlocked = 0;
        for (const auto& a : achievements) if (a.unlocked) ++unlocked;
        m_mainItems.emplace_back("RECORDS", std::to_string(unlocked) + "/" +
                                            std::to_string(achievements.size()));
    }
    m_mainItems.emplace_back("OPTIONS");
    m_mainItems.emplace_back("QUIT");

    // Returning here from a finished run must not leave the screen unusable.
    m_dismissed = false;
    m_page = Page::Main;
}

void MenuState::exit() {
    std::cout << "Exiting MenuState" << std::endl;
}

void MenuState::applyDifficultyPreset(int index) {
    m_selectedDifficultyIdx = index;
    m_generatorConfig.difficulty = static_cast<MapDifficulty>(index);
    switch (index) {
        case 0:  m_generatorConfig.pitProbability = 0.05f; m_generatorConfig.enemySpawnRate = 0.10f; break;
        case 1:  m_generatorConfig.pitProbability = 0.12f; m_generatorConfig.enemySpawnRate = 0.20f; break;
        default: m_generatorConfig.pitProbability = 0.22f; m_generatorConfig.enemySpawnRate = 0.35f; break;
    }
}

bool MenuState::isMultiplayerRowEnabled(MpRow row) const {
    switch (row) {
        case MpRow::Opponent:
            // Only the two versus modes have an opponent to choose. Co-op is
            // human-only by definition, and a shadow is not an opponent that can
            // be configured.
            return m_match.isVersus();
        case MpRow::Difficulty:
        case MpRow::Archetype:
            return m_match.isCpuOpponent();
        default:
            return true;
    }
}

std::vector<UiMenuItem> MenuState::buildMultiplayerItems() const {
    std::vector<UiMenuItem> rows;
    rows.emplace_back("MODE", toString(m_match.mode));
    rows.emplace_back("OPPONENT",
                      m_match.isCpuOpponent() ? "CPU" : "HUMAN",
                      isMultiplayerRowEnabled(MpRow::Opponent));
    rows.emplace_back("AI SKILL", toString(m_match.aiDifficulty),
                      isMultiplayerRowEnabled(MpRow::Difficulty));
    rows.emplace_back("AI STYLE", toString(m_match.aiArchetype),
                      isMultiplayerRowEnabled(MpRow::Archetype));
    rows.emplace_back("START");
    rows.emplace_back("BACK");
    return rows;
}

void MenuState::moveSelection(int delta) {
    if (m_page == Page::Main) {
        m_mainSelected = (m_mainSelected + delta + ROW_COUNT) % ROW_COUNT;
        return;
    }
    if (m_page == Page::Generator) {
        const int n = static_cast<int>(GenRow::COUNT);
        m_genSelected = (m_genSelected + delta + n) % n;
        return;
    }

    // Skip over rows this mode does not offer, so the cursor never parks on a
    // greyed-out line where Left/Right silently does nothing.
    const int n = static_cast<int>(MpRow::COUNT);
    const int step = (delta >= 0) ? 1 : -1;
    for (int i = 0; i < n; ++i) {
        m_mpSelected = (m_mpSelected + step + n) % n;
        if (isMultiplayerRowEnabled(static_cast<MpRow>(m_mpSelected))) return;
    }
}

void MenuState::adjustSelection(int direction) {
    if (m_page == Page::Multiplayer) {
        switch (static_cast<MpRow>(m_mpSelected)) {
            case MpRow::Mode: {
                const int next = (indexOfMode(m_match.mode) + direction +
                                  kMultiplayerModeCount) % kMultiplayerModeCount;
                m_match.mode = kMultiplayerModes[next];
                // The cursor may now be sitting on a row this mode does not
                // offer — step it back to somewhere meaningful.
                if (!isMultiplayerRowEnabled(static_cast<MpRow>(m_mpSelected))) {
                    m_mpSelected = static_cast<int>(MpRow::Mode);
                }
                break;
            }
            case MpRow::Opponent:
                if (!m_match.isVersus()) break;
                // Two choices, so direction only has to flip it.
                m_match.mode = m_match.isCpuOpponent() ? GameMode::VersusHuman
                                                       : GameMode::VersusCPU;
                break;
            case MpRow::Difficulty: {
                if (!m_match.isCpuOpponent()) break;
                constexpr int kCount = 3;
                const int next = (static_cast<int>(m_match.aiDifficulty) + direction +
                                  kCount) % kCount;
                m_match.aiDifficulty = static_cast<AIDifficulty>(next);
                break;
            }
            case MpRow::Archetype: {
                if (!m_match.isCpuOpponent()) break;
                constexpr int kCount = 3;
                const int next = (static_cast<int>(m_match.aiArchetype) + direction +
                                  kCount) % kCount;
                m_match.aiArchetype = static_cast<AIArchetype>(next);
                break;
            }
            default:
                break;
        }
        return;
    }

    if (m_page != Page::Generator) return;

    // Sliders move in 1% steps; the ranges match what the old ImGui sliders used.
    switch (static_cast<GenRow>(m_genSelected)) {
        case GenRow::Theme:
            m_selectedThemeIdx = (m_selectedThemeIdx + direction + kThemeCount) % kThemeCount;
            m_generatorConfig.theme = static_cast<MapTheme>(m_selectedThemeIdx);
            break;
        case GenRow::Difficulty:
            applyDifficultyPreset((m_selectedDifficultyIdx + direction + kDifficultyCount) % kDifficultyCount);
            break;
        case GenRow::PitProbability:
            m_generatorConfig.pitProbability =
                std::clamp(m_generatorConfig.pitProbability + 0.01f * direction, 0.0f, 0.40f);
            break;
        case GenRow::PipeFrequency:
            m_generatorConfig.pipeFrequency =
                std::clamp(m_generatorConfig.pipeFrequency + 0.01f * direction, 0.0f, 0.20f);
            break;
        case GenRow::EnemyRate:
            m_generatorConfig.enemySpawnRate =
                std::clamp(m_generatorConfig.enemySpawnRate + 0.01f * direction, 0.0f, 0.50f);
            break;
        case GenRow::CoinRate:
            m_generatorConfig.coinClusterRate =
                std::clamp(m_generatorConfig.coinClusterRate + 0.01f * direction, 0.0f, 0.50f);
            break;
        default:
            break;
    }
}

void MenuState::activateSelection() {
    if (m_dismissed) return;
    Game& game = Game::getInstance();

    if (m_page == Page::Main) {
        switch (m_mainSelected) {
            case ROW_START:
                m_dismissed = true;
                game.changeState(std::make_unique<CharacterSelectState>(false, false));
                break;
            case ROW_VERSUS:
                // Opens the multiplayer page rather than starting a match. This
                // row used to drop straight into a hardcoded human-vs-human
                // shared-screen game on 1-1, which was the only match that
                // existed; there are four modes now and a CPU opponent to
                // configure.
                m_page = Page::Multiplayer;
                m_mpSelected = static_cast<int>(MpRow::Mode);
                break;
            case ROW_DAILY: {
                // Date-seeded, so everyone playing today gets the same level —
                // which is the only thing that makes it a challenge.
                m_dismissed = true;
                const MapGeneratorConfig daily =
                    MetaGame::dailyChallengeConfig(MetaGame::todaysSeed());
                game.changeState(std::make_unique<CharacterSelectState>(false, true, daily));
                break;
            }
            case ROW_EDITOR:
                m_dismissed = true;
                game.changeState(std::make_unique<PlayingState>(true, false));
                break;
            case ROW_GENERATOR:
                m_page = Page::Generator;
                m_genSelected = 0;
                break;
            case ROW_RECORDS:
                // Same screen, opened on the statistics page. Everything it
                // shows was already being tracked and persisted with nowhere in
                // the game to see it.
                game.pushState(std::make_unique<OptionsState>(OptionsState::Page::Statistics));
                break;
            case ROW_OPTIONS:
                // Overlay: it pops straight back to this menu.
                game.pushState(std::make_unique<OptionsState>());
                break;
            case ROW_QUIT:
                m_dismissed = true;
                game.quit();
                break;
            default:
                break;
        }
        return;
    }

    if (m_page == Page::Multiplayer) {
        switch (static_cast<MpRow>(m_mpSelected)) {
            case MpRow::Start:
                // Straight into 1-1: a multiplayer match is one level, and
                // routing it through the world map would imply a shared
                // campaign that these modes do not have.
                m_dismissed = true;
                game.changeState(std::make_unique<PlayingState>(
                    false, false, MapGeneratorConfig(), 0, 0, m_match));
                break;
            case MpRow::Back:
                m_page = Page::Main;
                break;
            default:
                // Value rows confirm as a nudge, so Enter is never a dead key.
                adjustSelection(1);
                break;
        }
        return;
    }

    switch (static_cast<GenRow>(m_genSelected)) {
        case GenRow::GeneratePlay:
            m_dismissed = true;
            game.changeState(std::make_unique<CharacterSelectState>(false, true, m_generatorConfig));
            break;
        case GenRow::GenerateEdit:
            m_dismissed = true;
            game.changeState(std::make_unique<PlayingState>(true, true, m_generatorConfig));
            break;
        case GenRow::Back:
            m_page = Page::Main;
            break;
        default:
            // Value rows confirm as a nudge, so Enter is never a dead key.
            adjustSelection(1);
            break;
    }
}

void MenuState::handleInput(const sf::Event& event) {
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
        case Key::Left:
        case Key::A:
            adjustSelection(-1);
            break;
        case Key::Right:
        case Key::D:
            adjustSelection(1);
            break;
        case Key::Enter:
        case Key::Space:
            activateSelection();
            break;
        case Key::Escape:
        case Key::Backspace:
            // From the submenu, back out. From the top level, quitting is the
            // only thing left — but make the player pick it deliberately.
            if (m_page != Page::Main) m_page = Page::Main;
            else                      m_mainSelected = ROW_QUIT;
            break;
        default:
            break;
    }
}

void MenuState::update(float dt) {
    m_elapsed += dt;

    // Clouds drift left; the walker patrols the ground line and wraps around.
    m_cloudScroll = std::fmod(m_cloudScroll + dt * 14.0f, static_cast<float>(Constants::WINDOW_WIDTH) + 200.0f);
    m_walkerX += dt * 70.0f;
    if (m_walkerX > static_cast<float>(Constants::WINDOW_WIDTH) + 64.0f) {
        m_walkerX = -64.0f;
    }
}

void MenuState::drawBackground(sf::RenderTarget& target) const {
    // Was a hand-rolled backdrop of sf::CircleShape clouds and hills, written
    // before BackgroundRenderer existed. It read as coloured blobs next to the
    // pixel art everywhere else, which is exactly what it looked like.
    m_background.render(target, AABB{m_cloudScroll * 3.0f, 0.0f,
                                     static_cast<float>(Constants::WINDOW_WIDTH),
                                     static_cast<float>(Constants::WINDOW_HEIGHT)});

    // The walking character stays: it is the one animated thing on the screen.
    if (m_playerSheet) {
        const std::string frame = "mario_small_walk_" + std::to_string(
            static_cast<int>(m_elapsed / 0.12f) % 2);
        if (m_playerSheet->hasFrame(frame)) {
            sf::Sprite walker = m_playerSheet->getSprite(frame);
            walker.setScale({2.0f, 2.0f});
            const auto bounds = walker.getLocalBounds();
            // 640 is BackgroundRenderer's ground line: the walker has to stand on the
            // same line the backdrop layers sit on, or it floats.
            walker.setPosition({m_walkerX, 640.0f - bounds.size.y * 2.0f});
            target.draw(walker);
        }
    }
}

void MenuState::render(sf::RenderTarget& target) {
    target.setView(target.getDefaultView());
    drawBackground(target);

    const float centerX = Constants::WINDOW_WIDTH * 0.5f;

    // Title bobs on a sine so the screen is never completely static.
    const float titleY = 74.0f + std::sin(m_elapsed * 2.0f) * 6.0f;
    UiRenderer::drawShadowedText(target, "SUPER MARIO", {centerX, titleY}, 44,
                                 sf::Color(255, 216, 0), true);
    UiRenderer::drawShadowedText(target, "CS202 FINAL PROJECT", {centerX, titleY + 58.0f}, 14,
                                 sf::Color(255, 255, 255), true);

    if (m_page == Page::Main) {
        // Height comes from the row count. It was a hardcoded 250px, which fit
        // the five rows it was written for; adding 2P Versus and Daily Challenge
        // pushed the last two rows straight out through the bottom of the panel
        // and on top of the hint line.
        constexpr float ROW_HEIGHT = 40.0f;
        constexpr float PANEL_TOP = 214.0f;
        constexpr float PADDING = 26.0f;
        const float panelHeight = PADDING * 2.0f + ROW_HEIGHT * static_cast<float>(m_mainItems.size());

        // Wide enough that the longest label ("DAILY CHALLENGE", 15 characters
        // at 15px) clears the value column instead of printing through it.
        UiRenderer::drawPanel(target, {centerX - 320.0f, PANEL_TOP}, {640.0f, panelHeight},
                              sf::Color(0, 0, 0, 170));
        UiRenderer::drawMenuItems(target, m_mainItems, m_mainSelected,
                                  {centerX - 270.0f, PANEL_TOP + PADDING}, ROW_HEIGHT, 15,
                                  centerX + 40.0f, m_elapsed);
        // Shadowed: this line sits over the parallax bushes, and plain white
        // text on light green foliage is unreadable.
        UiRenderer::drawShadowedText(target, "UP/DOWN  SELECT      ENTER  CONFIRM",
                                     {centerX, PANEL_TOP + panelHeight + 20.0f}, 11,
                                     sf::Color(255, 255, 255), true);
        return;
    }

    if (m_page == Page::Multiplayer) {
        const std::vector<UiMenuItem> rows = buildMultiplayerItems();

        // Which keys each participant gets — the one thing a second player at
        // the same keyboard has to be told before the level starts.
        //
        // Read from InputManager rather than written out, because these are
        // rebindable and a hint that names the defaults is worse than none: this
        // line said "P1 WASD" while a config.json in the repo had Player 1 on the
        // arrow keys, which also means both players were sharing them.
        const InputManager& input = InputManager::getInstance();
        const bool twoHumans = (m_match.mode == GameMode::VersusHuman || m_match.isCoop());
        bool padsCollide = false;
        if (twoHumans) {
            for (const char* action : {"left", "right", "jump"}) {
                if (!input.getBoundKeyName(action, 0).empty() &&
                    input.getBoundKeyName(action, 0) == input.getBoundKeyName(action, 1)) {
                    padsCollide = true;
                    break;
                }
            }
        }

        // The panel is sized from what is actually going in it. It was a fixed
        // 330px, so the key summary and the shared-keys warning — both added
        // later, and both conditional — were clipped straight through the bottom
        // edge of the frame.
        constexpr float kTop = 196.0f;
        constexpr float kRowHeight = 34.0f;
        const float rowsTop = kTop + 58.0f;
        const float rowsBottom = rowsTop + kRowHeight * static_cast<float>(rows.size());
        const float blurbY = rowsBottom + 16.0f;
        const float keysY = blurbY + 22.0f;
        const float warnY = keysY + 18.0f;
        const float contentBottom = padsCollide ? warnY : (twoHumans ? keysY : blurbY);
        const float panelHeight = (contentBottom + 22.0f) - kTop;

        UiRenderer::drawPanel(target, {centerX - 280.0f, kTop}, {560.0f, panelHeight},
                              sf::Color(0, 0, 0, 200));
        UiRenderer::drawText(target, "MULTIPLAYER", {centerX, kTop + 20.0f}, 14,
                             sf::Color(255, 170, 220), true);
        UiRenderer::drawMenuItems(target, rows, m_mpSelected,
                                  {centerX - 210.0f, rowsTop}, kRowHeight, 13,
                                  centerX + 90.0f, m_elapsed);

        // What the highlighted mode actually does. The labels alone do not say.
        UiRenderer::drawText(target, modeBlurb(m_match.mode), {centerX, blurbY}, 11,
                             sf::Color(200, 200, 200), true);

        if (twoHumans) {
            auto padSummary = [&input](int pad) {
                const std::string left  = input.getBoundKeyName("left", pad);
                const std::string right = input.getBoundKeyName("right", pad);
                const std::string jump  = input.getBoundKeyName("jump", pad);
                return left + "/" + right + " + " + jump;
            };
            UiRenderer::drawText(target, "P1  " + padSummary(0) + "      P2  " + padSummary(1),
                                 {centerX, keysY}, 11, sf::Color(150, 220, 150), true);

            // Both pads on the same key is unplayable, and the only place it can
            // be noticed before the level starts is here.
            if (padsCollide) {
                UiRenderer::drawText(target, "BOTH PLAYERS SHARE KEYS - SEE OPTIONS/KEYS",
                                     {centerX, warnY}, 10, sf::Color(255, 140, 140), true);
            }
        }

        UiRenderer::drawShadowedText(target, "LEFT/RIGHT  ADJUST      ESC  BACK",
                                     {centerX, kTop + panelHeight + 18.0f}, 11,
                                     sf::Color(220, 220, 220), true);
        return;
    }

    // --- Procedural generator submenu ---
    std::vector<UiMenuItem> rows;
    rows.emplace_back("THEME",       kThemes[m_selectedThemeIdx]);
    rows.emplace_back("DIFFICULTY",  kDifficulties[m_selectedDifficultyIdx]);
    rows.emplace_back("PITS",        percent(m_generatorConfig.pitProbability));
    rows.emplace_back("PIPES",       percent(m_generatorConfig.pipeFrequency));
    rows.emplace_back("ENEMIES",     percent(m_generatorConfig.enemySpawnRate));
    rows.emplace_back("COINS",       percent(m_generatorConfig.coinClusterRate));
    rows.emplace_back("GENERATE & PLAY");
    rows.emplace_back("GENERATE & EDIT");
    rows.emplace_back("BACK");

    UiRenderer::drawPanel(target, {centerX - 280.0f, 200.0f}, {560.0f, 400.0f},
                          sf::Color(0, 0, 0, 200));
    UiRenderer::drawText(target, "PROCEDURAL GENERATOR", {centerX, 220.0f}, 14,
                         sf::Color(120, 200, 255), true);
    UiRenderer::drawMenuItems(target, rows, m_genSelected,
                              {centerX - 210.0f, 262.0f}, 36.0f, 13,
                              centerX + 110.0f, m_elapsed);
    UiRenderer::drawShadowedText(target, "LEFT/RIGHT  ADJUST      ESC  BACK",
                                 {centerX, 570.0f}, 11, sf::Color(220, 220, 220), true);
}
