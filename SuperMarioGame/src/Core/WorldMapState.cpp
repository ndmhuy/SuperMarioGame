#include "Core/WorldMapState.hpp"
#include "Core/CharacterSelectState.hpp"
#include "Core/Game.hpp"
#include "Core/MenuState.hpp"
#include "Core/PlayingState.hpp"
#include "Core/SoundManager.hpp"
#include "Utils/Constants.hpp"
#include "Utils/LevelCatalog.hpp"

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Window/Event.hpp>

#include <cmath>
#include <filesystem>
#include <iostream>

namespace {

constexpr float NODE_RADIUS = 26.0f;
constexpr float MAP_LEFT = 130.0f;
constexpr float MAP_RIGHT = 1150.0f;
constexpr float MAP_BASELINE = 400.0f;
// How far the path rises and falls between nodes. Purely decorative, but it is
// what makes the row read as a route rather than a list.
constexpr float MAP_WAVE = 90.0f;

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

WorldMapState::WorldMapState(int characterIndex)
    : m_characterIndex(characterIndex) {}

void WorldMapState::buildNodes() {
    m_nodes.clear();
    const int count = LevelCatalog::count();
    if (count <= 0) return;

    const float span = MAP_RIGHT - MAP_LEFT;
    for (int i = 0; i < count; ++i) {
        const float t = (count == 1) ? 0.5f : static_cast<float>(i) / static_cast<float>(count - 1);
        Node node;
        node.levelIndex = i;
        node.position = {
            MAP_LEFT + span * t,
            MAP_BASELINE + std::sin(t * 3.14159f * 2.0f) * MAP_WAVE
        };
        m_nodes.push_back(node);
    }
}

void WorldMapState::enter() {
    std::cout << "Entering WorldMapState" << std::endl;
    SoundManager::getInstance().playMusic("world_map");

    m_progress = CampaignProgress::load();
    buildNodes();

    // Open on the furthest level the player may enter, which is where they
    // actually want to be — not back at 1-1 every time.
    m_selected = CampaignProgress::highestUnlockedIndex();

    m_scenerySheet = tryLoadSheet("world_scenery_item");
    m_playerSheet = tryLoadSheet("player");
    m_dismissed = false;
}

void WorldMapState::exit() {
    std::cout << "Exiting WorldMapState" << std::endl;
}

void WorldMapState::moveSelection(int delta) {
    const int count = static_cast<int>(m_nodes.size());
    if (count == 0) return;

    // Walk along the path and stop at the last unlocked node rather than
    // wrapping: the map is a route, and a locked level is the end of it.
    const int candidate = m_selected + delta;
    if (candidate < 0 || candidate >= count) return;
    if (!CampaignProgress::isUnlocked(candidate)) {
        SoundManager::getInstance().playSound("bump");
        return;
    }
    m_selected = candidate;
}

void WorldMapState::confirmSelection() {
    if (m_dismissed) return;
    if (!CampaignProgress::isUnlocked(m_selected)) {
        SoundManager::getInstance().playSound("bump");
        return;
    }

    m_dismissed = true;
    SoundManager::getInstance().playSound("enter_level");
    Game::getInstance().changeState(std::make_unique<PlayingState>(
        false, false, MapGeneratorConfig(), m_characterIndex, m_selected));
}

void WorldMapState::handleInput(const sf::Event& event) {
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
                Game::getInstance().changeState(std::make_unique<CharacterSelectState>());
            }
            break;
        default:
            break;
    }
}

void WorldMapState::update(float dt) {
    m_elapsed += dt;
}

void WorldMapState::drawPath(sf::RenderTarget& target) const {
    // Dashed segments between consecutive nodes, with the dashes crawling along
    // the route. A cleared stretch is drawn solid gold.
    for (std::size_t i = 0; i + 1 < m_nodes.size(); ++i) {
        const sf::Vector2f a = m_nodes[i].position;
        const sf::Vector2f b = m_nodes[i + 1].position;
        const sf::Vector2f delta = b - a;
        const float length = std::sqrt(delta.x * delta.x + delta.y * delta.y);
        if (length <= 1.0f) continue;

        const bool cleared = m_progress[i].completed;
        const sf::Color color = cleared ? sf::Color(255, 216, 0, 220) : sf::Color(180, 180, 180, 120);

        const int dots = static_cast<int>(length / 18.0f);
        // Crawl only along an uncleared stretch, so movement reads as "go here".
        const float crawl = cleared ? 0.0f : std::fmod(m_elapsed * 0.5f, 1.0f);
        for (int d = 0; d <= dots; ++d) {
            const float t = std::fmod(static_cast<float>(d) / static_cast<float>(dots + 1) + crawl, 1.0f);
            sf::CircleShape dot(cleared ? 4.0f : 3.0f);
            dot.setOrigin({dot.getRadius(), dot.getRadius()});
            dot.setPosition(a + delta * t);
            dot.setFillColor(color);
            target.draw(dot);
        }
    }
}

void WorldMapState::drawNode(sf::RenderTarget& target, const Node& node) const {
    const bool unlocked = CampaignProgress::isUnlocked(node.levelIndex);
    const LevelProgress& progress = m_progress[static_cast<std::size_t>(node.levelIndex)];
    const bool selected = (node.levelIndex == m_selected);

    sf::Color fill = sf::Color(70, 70, 90);
    if (progress.completed) fill = sf::Color(60, 140, 70);
    else if (unlocked)      fill = sf::Color(70, 100, 170);

    // The selected node breathes, so it is findable without reading the labels.
    const float pulse = selected ? (1.0f + std::sin(m_elapsed * 5.0f) * 0.08f) : 1.0f;

    sf::CircleShape disc(NODE_RADIUS * pulse);
    disc.setOrigin({disc.getRadius(), disc.getRadius()});
    disc.setPosition(node.position);
    disc.setFillColor(fill);
    disc.setOutlineThickness(selected ? 4.0f : 2.0f);
    disc.setOutlineColor(selected ? sf::Color(255, 216, 0)
                                  : (unlocked ? sf::Color(220, 220, 220) : sf::Color(110, 110, 110)));
    target.draw(disc);

    UiRenderer::drawText(target, LevelCatalog::nameFor(node.levelIndex),
                         {node.position.x, node.position.y + NODE_RADIUS + 14.0f}, 11,
                         unlocked ? sf::Color(240, 240, 240) : sf::Color(130, 130, 130), true);

    if (!unlocked) {
        UiRenderer::drawText(target, "X", {node.position.x, node.position.y - 8.0f}, 14,
                             sf::Color(200, 90, 90), true);
    } else if (progress.completed) {
        // Completion mark: the flag from the world atlas if it is there, a tick
        // if it is not.
        bool drewFlag = false;
        if (m_scenerySheet && m_scenerySheet->hasFrame("castle_flag")) {
            sf::Sprite flag = m_scenerySheet->getSprite("castle_flag");
            const auto bounds = flag.getLocalBounds();
            if (bounds.size.x > 0.0f) {
                flag.setScale({2.0f, 2.0f});
                flag.setPosition({node.position.x - bounds.size.x,
                                  node.position.y - bounds.size.y - NODE_RADIUS - 6.0f});
                target.draw(flag);
                drewFlag = true;
            }
        }
        if (!drewFlag) {
            UiRenderer::drawText(target, "*", {node.position.x, node.position.y - NODE_RADIUS - 26.0f},
                                 16, sf::Color(255, 216, 0), true);
        }
    }

    // Star coins: filled for found, hollow for missed.
    for (int c = 0; c < 3; ++c) {
        const bool found = progress.starCoins[static_cast<std::size_t>(c)];
        sf::CircleShape pip(5.0f);
        pip.setOrigin({5.0f, 5.0f});
        pip.setPosition({node.position.x - 16.0f + static_cast<float>(c) * 16.0f,
                         node.position.y + NODE_RADIUS + 34.0f});
        pip.setFillColor(found ? sf::Color(255, 216, 0) : sf::Color(0, 0, 0, 0));
        pip.setOutlineThickness(1.5f);
        pip.setOutlineColor(found ? sf::Color(255, 240, 160) : sf::Color(120, 120, 120));
        target.draw(pip);
    }
}

void WorldMapState::render(sf::RenderTarget& target) {
    target.setView(target.getDefaultView());
    UiRenderer::drawDimmer(target, 255, sf::Color(28, 52, 96));

    const float centerX = Constants::WINDOW_WIDTH * 0.5f;
    UiRenderer::drawShadowedText(target, "WORLD MAP", {centerX, 54.0f}, 26,
                                 sf::Color(255, 216, 0), true);

    const int totalCoins = CampaignProgress::totalStarCoins();
    UiRenderer::drawText(target, "STAR COINS  " + std::to_string(totalCoins) + " / " +
                                 std::to_string(LevelCatalog::count() * 3),
                         {centerX, 96.0f}, 12, sf::Color(220, 220, 220), true);

    drawPath(target);
    for (const Node& node : m_nodes) {
        drawNode(target, node);
    }

    // The chosen character walks the map, standing on the selected node.
    if (m_playerSheet && !m_nodes.empty()) {
        static const char* kCharacters[] = {"mario", "luigi", "toad", "peach"};
        const int index = (m_characterIndex >= 0 && m_characterIndex < 4) ? m_characterIndex : 0;
        const std::string frame = std::string(kCharacters[index]) + "_small_walk_" +
                                  std::to_string(static_cast<int>(m_elapsed / 0.15f) % 2);
        if (m_playerSheet->hasFrame(frame)) {
            sf::Sprite avatar = m_playerSheet->getSprite(frame);
            avatar.setScale({2.0f, 2.0f});
            const auto bounds = avatar.getLocalBounds();
            const sf::Vector2f node = m_nodes[static_cast<std::size_t>(m_selected)].position;
            avatar.setPosition({node.x - bounds.size.x,
                                node.y - NODE_RADIUS - bounds.size.y * 2.0f - 8.0f});
            target.draw(avatar);
        }
    }

    UiRenderer::drawText(target, "LEFT/RIGHT  TRAVEL     ENTER  PLAY     ESC  BACK",
                         {centerX, 640.0f}, 11, sf::Color(200, 200, 200), true);
}
