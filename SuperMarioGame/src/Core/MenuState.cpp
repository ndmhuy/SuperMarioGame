#include "Core/MenuState.hpp"
#include "Core/CharacterSelectState.hpp"
#include "Core/OptionsState.hpp"
#include "Core/PlayingState.hpp"
#include "Core/Game.hpp"
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
enum MainRow { ROW_START = 0, ROW_DAILY, ROW_EDITOR, ROW_GENERATOR, ROW_OPTIONS, ROW_QUIT, ROW_COUNT };

const char* const kThemes[] = {"OVERWORLD", "UNDERGROUND", "CASTLE", "ICE"};
constexpr int kThemeCount = 4;
const char* const kDifficulties[] = {"EASY", "MEDIUM", "HARD"};
constexpr int kDifficultyCount = 3;

constexpr float GROUND_Y = 600.0f;

std::string percent(float value) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(0) << (value * 100.0f) << "%";
    return ss.str();
}

std::unique_ptr<SpriteSheet> tryLoadSheet(const std::string& folderName) {
    for (const std::string& root : {"assets/spriteSheet/", "SuperMarioGame/assets/spriteSheet/",
                                    "../assets/spriteSheet/"}) {
        const std::string path = root + folderName;
        if (std::filesystem::exists(path)) {
            try {
                return std::make_unique<SpriteSheet>(path);
            } catch (...) {}
        }
    }
    return nullptr;
}

} // namespace

void MenuState::enter() {
    std::cout << "Entering MenuState" << std::endl;
    // Pass the registered track key, not a raw path — SoundManager maps
    // "title_screen" to assets/bgm/main_menu.mp3 and resolves it per platform.
    SoundManager::getInstance().playMusic("title_screen");

    m_playerSheet  = tryLoadSheet("player");
    m_scenerySheet = tryLoadSheet("world_scenery_item");

    m_mainItems.clear();
    // The New Game+ cycle is shown on the start row, so a player can see the
    // campaign has reset rather than wondering why 1-2 is locked again.
    m_mainItems.emplace_back("START GAME", MetaGame::newGamePlusLabel());
    m_mainItems.emplace_back("DAILY CHALLENGE", MetaGame::todaysChallengeName());
    m_mainItems.emplace_back("MAP EDITOR");
    m_mainItems.emplace_back("PROCEDURAL LEVEL");
    m_mainItems.emplace_back("OPTIONS & SCORES");
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

void MenuState::moveSelection(int delta) {
    if (m_page == Page::Main) {
        m_mainSelected = (m_mainSelected + delta + ROW_COUNT) % ROW_COUNT;
    } else {
        const int n = static_cast<int>(GenRow::COUNT);
        m_genSelected = (m_genSelected + delta + n) % n;
    }
}

void MenuState::adjustSelection(int direction) {
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
            if (m_page == Page::Generator) m_page = Page::Main;
            else                           m_mainSelected = ROW_QUIT;
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
    // Sky.
    UiRenderer::drawDimmer(target, 255, sf::Color(92, 148, 252));

    // Two cloud banks at different speeds — cheap parallax, no assets needed.
    auto drawCloud = [&target](float x, float y, float scale) {
        for (int i = 0; i < 3; ++i) {
            sf::CircleShape puff(18.0f * scale);
            puff.setFillColor(sf::Color(255, 255, 255, 210));
            puff.setPosition({x + static_cast<float>(i) * 22.0f * scale, y - (i == 1 ? 10.0f * scale : 0.0f)});
            target.draw(puff);
        }
    };
    const float w = static_cast<float>(Constants::WINDOW_WIDTH) + 200.0f;
    for (int i = 0; i < 4; ++i) {
        const float base = static_cast<float>(i) * 340.0f;
        drawCloud(std::fmod(base - m_cloudScroll + w, w) - 100.0f, 90.0f, 1.0f);
        drawCloud(std::fmod(base + 170.0f - m_cloudScroll * 0.55f + w, w) - 100.0f, 190.0f, 0.7f);
    }

    // Rolling hills behind the ground line.
    for (int i = 0; i < 5; ++i) {
        sf::CircleShape hill(110.0f);
        hill.setFillColor(sf::Color(64, 168, 72));
        hill.setPosition({static_cast<float>(i) * 300.0f - 120.0f, GROUND_Y - 120.0f});
        target.draw(hill);
    }

    // Ground strip, tiled from the same atlas the levels use when available.
    const float tile = Constants::TILE_SIZE;
    bool drewTiles = false;
    if (m_scenerySheet && m_scenerySheet->hasFrame("solid_block_brown")) {
        for (float x = 0.0f; x < static_cast<float>(Constants::WINDOW_WIDTH); x += tile) {
            sf::Sprite ground = m_scenerySheet->getSprite("solid_block_brown");
            const auto bounds = ground.getLocalBounds();
            if (bounds.size.x <= 0.0f || bounds.size.y <= 0.0f) break;
            ground.setScale({tile / bounds.size.x, tile / bounds.size.y});
            ground.setPosition({x, GROUND_Y});
            target.draw(ground);
            drewTiles = true;
        }
    }
    if (!drewTiles) {
        sf::RectangleShape ground({static_cast<float>(Constants::WINDOW_WIDTH), tile});
        ground.setFillColor(sf::Color(150, 90, 40));
        ground.setPosition({0.0f, GROUND_Y});
        target.draw(ground);
    }
    sf::RectangleShape subsoil({static_cast<float>(Constants::WINDOW_WIDTH),
                                static_cast<float>(Constants::WINDOW_HEIGHT) - GROUND_Y - tile});
    subsoil.setFillColor(sf::Color(92, 56, 24));
    subsoil.setPosition({0.0f, GROUND_Y + tile});
    target.draw(subsoil);

    // A Mario walking the ground line, using the real two-frame walk cycle.
    if (m_playerSheet) {
        const std::string frame = "mario_small_walk_" + std::to_string(
            static_cast<int>(m_elapsed / 0.12f) % 2);
        if (m_playerSheet->hasFrame(frame)) {
            sf::Sprite walker = m_playerSheet->getSprite(frame);
            walker.setScale({2.0f, 2.0f});
            const auto bounds = walker.getLocalBounds();
            walker.setPosition({m_walkerX, GROUND_Y - bounds.size.y * 2.0f});
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
        UiRenderer::drawPanel(target, {centerX - 220.0f, 220.0f}, {440.0f, 250.0f},
                              sf::Color(0, 0, 0, 170));
        UiRenderer::drawMenuItems(target, m_mainItems, m_mainSelected,
                                  {centerX - 150.0f, 250.0f}, 44.0f, 16, 0.0f, m_elapsed);
        UiRenderer::drawText(target, "UP/DOWN  SELECT      ENTER  CONFIRM",
                             {centerX, 500.0f}, 11, sf::Color(255, 255, 255), true);
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
    UiRenderer::drawText(target, "LEFT/RIGHT  ADJUST      ESC  BACK",
                         {centerX, 570.0f}, 11, sf::Color(200, 200, 200), true);
}
