#include "Core/PlayingState.hpp"
#include "Core/MenuState.hpp"
#include "Core/Game.hpp"
#include "Core/ResourceManager.hpp"
#include "Entities/Entity.hpp"
#include "Entities/Player.hpp"
#include "Utils/Constants.hpp"
#include <SFML/Window/Event.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <imgui.h>
#include <iostream>

// Simple dummy entity to represent the falling box in the physics test
class DummyPhysicsEntity : public Entity {
public:
    DummyPhysicsEntity(sf::Vector2f pos, sf::Vector2f size) {
        position = pos;
        boundingBox = AABB{pos.x, pos.y, size.x, size.y};
        active = true;
    }

    void update(float dt) override {
        // Synchronize boundingBox coordinates with the integrated position
        boundingBox.x = position.x;
        boundingBox.y = position.y;
    }

    void render(sf::RenderTarget& target) override {
        sf::RectangleShape rect(sf::Vector2f(boundingBox.width, boundingBox.height));
        rect.setPosition(position);
        rect.setFillColor(sf::Color::Red);
        rect.setOutlineColor(sf::Color::White);
        rect.setOutlineThickness(1.0f);
        target.draw(rect);
    }

    const AABB& getBoundingBox() const override {
        return boundingBox;
    }
};

PlayingState::PlayingState() = default;
PlayingState::~PlayingState() = default;

void PlayingState::enter() {
    std::cout << "Entering PlayingState" << std::endl;
    
    // Ensure HUD font is loaded in ResourceManager
    ResourceManager& rm = ResourceManager::getInstance();
    if (!rm.loadFont("PressStart2P", "asset/font/PressStart2P.ttf")) {
        if (!rm.loadFont("PressStart2P", "C:/Windows/Fonts/consola.ttf")) {
            rm.loadFont("PressStart2P", "C:/Windows/Fonts/arial.ttf");
        }
    }

    // Initialize HUD and Level Timer
    m_hud = std::make_unique<Hud>(sf::Vector2i(Constants::WINDOW_WIDTH, Constants::WINDOW_HEIGHT));
    m_levelTimer = 300.0f;

    setupTestScene();
}

void PlayingState::exit() {
    std::cout << "Exiting PlayingState" << std::endl;
    cleanupTestScene();
}

void PlayingState::handleInput(const sf::Event& event) {
    if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>()) {
        if (keyPressed->code == sf::Keyboard::Key::Backspace) {
            Game::getInstance().changeState(std::make_unique<MenuState>());
        }
    }
}

void PlayingState::update(float dt) {
    // 1. Update level timer
    m_levelTimer = std::max(0.0f, m_levelTimer - dt);

    // 2. Update entities (synchronize visual positions to bounds)
    for (auto& entity : m_entities) {
        if (entity) {
            entity->update(dt);
        }
    }

    // 3. Run the physics engine pipeline (apply gravity, integrate velocity, check/resolve collisions)
    m_physicsEngine.update(m_entities, m_tileMap, dt);

    // 4. Sync HUD with player stats or fallback mock data
    if (m_hud) {
        HudData hudData;
        hudData.timeLeft = static_cast<int>(m_levelTimer);
        
        if (auto* player = Game::getInstance().getPlayer()) {
            hudData.score = player->getScore();
            hudData.coins = player->getCoins();
            hudData.lives = player->getLives();
            hudData.comboCount = player->getComboCounter();
            hudData.characterName = "MARIO";
            hudData.starCoinsCollected = {false, false, false};
        } else {
            // Mockup values matching the visual reference when running the test scene
            hudData.score = 102520;
            hudData.coins = 57;
            hudData.lives = 9;
            hudData.worldMajor = 1;
            hudData.worldMinor = 1;
            hudData.characterName = "mario";
            hudData.starCoinsCollected = {true, true, false}; // 2 out of 3 collected
        }
        m_hud->sync(hudData);
    }
}

void PlayingState::render(sf::RenderTarget& target) {
    // Draw the tilemap floor tiles using SFML shapes for visualization
    for (int y = 0; y < m_tileMap.getHeight(); ++y) {
        for (int x = 0; x < m_tileMap.getWidth(); ++x) {
            TileType tileType = m_tileMap.getTileType(x, y);
            if (tileType == TileType::Ground) { // Ground tile
                sf::RectangleShape tileShape(sf::Vector2f(Constants::TILE_SIZE, Constants::TILE_SIZE));
                tileShape.setPosition(sf::Vector2f(x * Constants::TILE_SIZE, y * Constants::TILE_SIZE));
                tileShape.setFillColor(sf::Color(120, 80, 40)); // Brown ground
                tileShape.setOutlineColor(sf::Color(60, 40, 20));
                tileShape.setOutlineThickness(1.0f);
                target.draw(tileShape);

                // Add a green grass top for visual clarity
                sf::RectangleShape grassShape(sf::Vector2f(Constants::TILE_SIZE, 8.0f));
                grassShape.setPosition(sf::Vector2f(x * Constants::TILE_SIZE, y * Constants::TILE_SIZE));
                grassShape.setFillColor(sf::Color(0, 180, 0)); // Green grass
                target.draw(grassShape);
            }
        }
    }

    // Draw the active entities
    for (auto& entity : m_entities) {
        if (entity && entity->isActive()) {
            entity->render(target);
        }
    }

    // Draw the screen-space HUD overlay
    if (m_hud) {
        sf::View oldView = target.getView();
        target.setView(target.getDefaultView());
        target.draw(*m_hud);
        target.setView(oldView);
    }

    // ImGui Panel for controlling and monitoring the physics simulation
    ImGui::Begin("Physics Simulation (Phase 2)");
    ImGui::Text("Simulation State:");
    if (!m_entities.empty() && m_entities[0]) {
        ImGui::Text("Entity Position: (%.1f, %.1f)", m_entities[0]->getPosition().x, m_entities[0]->getPosition().y);
        ImGui::Text("Entity Velocity: (%.1f, %.1f)", m_entities[0]->getVelocity().x, m_entities[0]->getVelocity().y);
    } else {
        ImGui::Text("No active entities.");
    }
    ImGui::Separator();
    if (ImGui::Button("Reset Simulation")) {
        setupTestScene();
    }
    ImGui::SameLine();
    if (ImGui::Button("Back to Menu (Backspace)")) {
        Game::getInstance().changeState(std::make_unique<MenuState>());
    }
    ImGui::End();
}

void PlayingState::setupTestScene() {
    cleanupTestScene();

    // Initialize TileMap to 40 columns and 22 rows
    m_tileMap.initialize(40, 22);

    // Create a ground layer at row y = 20
    for (int x = 0; x < 40; ++x) {
        m_tileMap.setTile(x, 20, TileType::Ground); // Ground tile
    }

    // Spawn a DummyPhysicsEntity at (300, 50) with size (32, 32)
    m_entities.push_back(std::make_unique<DummyPhysicsEntity>(sf::Vector2f(300.0f, 50.0f), sf::Vector2f(32.0f, 32.0f)));
}

void PlayingState::cleanupTestScene() {
    m_entities.clear();
}
