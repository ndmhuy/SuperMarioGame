#include "Core/CharacterSelectState.hpp"
#include "Core/Game.hpp"
#include "Core/MenuState.hpp"
#include "Core/PlayingState.hpp"
#include "Core/WorldMapState.hpp"
#include "Core/AchievementManager.hpp"
#include "Core/SoundManager.hpp"
#include "Utils/Constants.hpp"

#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Window/Event.hpp>
#include <cmath>
#include <filesystem>
#include <iostream>

namespace {
constexpr float CARD_W = 240.0f;
constexpr float CARD_H = 300.0f;
constexpr float CARD_GAP = 24.0f;
}

CharacterSelectState::CharacterSelectState(bool startInEditor, bool isProcedural,
                                           const MapGeneratorConfig& genConfig)
    : m_startInEditor(startInEditor), m_isProcedural(isProcedural), m_genConfig(genConfig) {
    // Order matches PlayingState's m_selectedCharIndex: 0 Mario, 1 Luigi, 2 Toad, 3 Peach.
    m_slots.push_back({"mario", "MARIO", "BALANCED",           "", ""});
    m_slots.push_back({"luigi", "LUIGI", "HIGH JUMP / DOUBLE", "", ""});
    m_slots.push_back({"toad",  "TOAD",  "FAST, LOW JUMP",     "toad",  "CLEAR ALL 3 LEVELS"});
    m_slots.push_back({"peach", "PEACH", "FLOATY GLIDE",       "peach", "CLEAR ALL 3 NO DEATHS"});
}

void CharacterSelectState::enter() {
    std::cout << "Entering CharacterSelectState" << std::endl;

    m_playerSheet = SpriteSheet::loadAtlas("player");
    if (!m_playerSheet) {
        std::cerr << "[CharacterSelectState] Player atlas not found; drawing name cards only."
                  << std::endl;
    }

    // Start on the first unlocked slot so the caret never opens on a locked card.
    for (std::size_t i = 0; i < m_slots.size(); ++i) {
        if (isUnlocked(m_slots[i])) {
            m_selected = static_cast<int>(i);
            break;
        }
    }
}

void CharacterSelectState::exit() {
    std::cout << "Exiting CharacterSelectState" << std::endl;
}

bool CharacterSelectState::isUnlocked(const CharacterSlot& slot) const {
    if (slot.achievementId.empty()) return true;
    return AchievementManager::getInstance().isUnlocked(slot.achievementId);
}

void CharacterSelectState::moveSelection(int delta) {
    const int n = static_cast<int>(m_slots.size());
    if (n == 0) return;

    // Skip over locked cards rather than letting the caret rest on one.
    for (int step = 1; step <= n; ++step) {
        const int candidate = ((m_selected + delta * step) % n + n) % n;
        if (isUnlocked(m_slots[static_cast<std::size_t>(candidate)])) {
            m_selected = candidate;
            return;
        }
    }
}

void CharacterSelectState::confirmSelection() {
    if (m_dismissed) return;
    if (!isUnlocked(m_slots[static_cast<std::size_t>(m_selected)])) {
        SoundManager::getInstance().playSound("bump");
        return;
    }

    m_dismissed = true;
    SoundManager::getInstance().playSound("enter_level");

    // The plain campaign goes through the world map, so the player picks where
    // to start and can see what they have cleared. The editor and the generator
    // have no campaign to map, so they drop straight into play.
    if (!m_startInEditor && !m_isProcedural) {
        Game::getInstance().changeState(std::make_unique<WorldMapState>(m_selected));
        return;
    }
    Game::getInstance().changeState(std::make_unique<PlayingState>(
        m_startInEditor, m_isProcedural, m_genConfig, m_selected, 0));
}

void CharacterSelectState::handleInput(const sf::Event& event) {
    const auto* keyPressed = event.getIf<sf::Event::KeyPressed>();
    if (!keyPressed) return;
    using Key = sf::Keyboard::Key;

    switch (keyPressed->code) {
        case Key::Left:
        case Key::A:
            moveSelection(-1);
            break;
        case Key::Right:
        case Key::D:
            moveSelection(1);
            break;
        case Key::Enter:
        case Key::Space:
            confirmSelection();
            break;
        case Key::Escape:
        case Key::Backspace:
            if (!m_dismissed) {
                m_dismissed = true;
                Game::getInstance().changeState(std::make_unique<MenuState>());
            }
            break;
        default:
            break;
    }
}

void CharacterSelectState::update(float dt) {
    m_elapsed += dt;
}

void CharacterSelectState::render(sf::RenderTarget& target) {
    target.setView(target.getDefaultView());
    UiRenderer::drawDimmer(target, 255, sf::Color(24, 32, 68));

    const float centerX = Constants::WINDOW_WIDTH * 0.5f;
    UiRenderer::drawShadowedText(target, "CHOOSE YOUR CHARACTER", {centerX, 70.0f}, 22,
                                 sf::Color(255, 216, 0), true);

    const float totalW = CARD_W * static_cast<float>(m_slots.size())
                       + CARD_GAP * static_cast<float>(m_slots.size() - 1);
    const float startX = (Constants::WINDOW_WIDTH - totalW) * 0.5f;
    const float cardY = 170.0f;

    for (std::size_t i = 0; i < m_slots.size(); ++i) {
        const CharacterSlot& slot = m_slots[i];
        const bool selected = (static_cast<int>(i) == m_selected);
        const bool unlocked = isUnlocked(slot);
        const float x = startX + static_cast<float>(i) * (CARD_W + CARD_GAP);

        sf::Color outline = sf::Color(90, 90, 110);
        if (!unlocked)      outline = sf::Color(70, 60, 60);
        else if (selected)  outline = sf::Color(255, 216, 0);

        // Selected card lifts a few pixels; enough motion to read as a selection
        // without any extra art.
        const float lift = selected ? std::abs(std::sin(m_elapsed * 3.0f)) * 6.0f : 0.0f;
        UiRenderer::drawPanel(target, {x, cardY - lift}, {CARD_W, CARD_H},
                              sf::Color(0, 0, 0, unlocked ? 200 : 160), outline);

        // Portrait: the character's idle frame, scaled up from the 16x26 source.
        if (m_playerSheet) {
            const std::string frame = slot.id + "_small_idle";
            if (m_playerSheet->hasFrame(frame)) {
                sf::Sprite portrait = m_playerSheet->getSprite(frame);
                portrait.setScale({4.0f, 4.0f});
                const auto bounds = portrait.getLocalBounds();
                portrait.setPosition({x + (CARD_W - bounds.size.x * 4.0f) * 0.5f,
                                      cardY - lift + 60.0f});
                if (!unlocked) {
                    portrait.setColor(sf::Color(0, 0, 0, 220));   // silhouette
                }
                target.draw(portrait);
            }
        }

        const sf::Color nameColor = !unlocked ? sf::Color(120, 120, 120)
                                  : selected  ? sf::Color(255, 216, 0)
                                              : sf::Color(230, 230, 230);
        UiRenderer::drawText(target, slot.displayName, {x + CARD_W * 0.5f, cardY - lift + 200.0f},
                             16, nameColor, true);

        // Blurbs and unlock hints are content, not layout: they are authored per
        // character and nothing stops the next one being longer than the card.
        // Fitting them to CARD_W means adding a character cannot push text out
        // over its neighbour's portrait.
        constexpr float TEXT_BUDGET = CARD_W - 16.0f;
        if (unlocked) {
            UiRenderer::drawTextFitted(target, slot.blurb, {x + CARD_W * 0.5f, cardY - lift + 236.0f},
                                       9, sf::Color(170, 170, 170), TEXT_BUDGET, true);
        } else {
            UiRenderer::drawText(target, "LOCKED", {x + CARD_W * 0.5f, cardY - lift + 236.0f},
                                 10, sf::Color(200, 80, 80), true);
            UiRenderer::drawTextFitted(target, slot.unlockHint,
                                       {x + CARD_W * 0.5f, cardY - lift + 262.0f}, 8,
                                       sf::Color(140, 110, 110), TEXT_BUDGET, true);
        }
    }

    UiRenderer::drawText(target, "LEFT/RIGHT  SELECT     ENTER  START     ESC  BACK",
                         {centerX, 560.0f}, 11, sf::Color(180, 180, 180), true);
}
