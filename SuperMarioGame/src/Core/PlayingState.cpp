#include "Core/PlayingState.hpp"
#include "Core/MenuState.hpp"
#include "Core/Game.hpp"
#include "Entities/Entity.hpp"
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

void PlayingState::enter() {
    std::cout << "Entering PlayingState" << std::endl;
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
    // 1. Update entities (synchronize visual positions to bounds)
    for (auto* entity : m_entities) {
        if (entity) {
            entity->update(dt);
        }
    }

    // 2. Run the physics engine pipeline (apply gravity, integrate velocity, check/resolve collisions)
    m_physicsEngine.update(m_entities, m_tileMap, dt);
}

void PlayingState::render(sf::RenderTarget& target) {
    // Draw the tilemap floor tiles using SFML shapes for visualization
    for (int y = 0; y < m_tileMap.getHeight(); ++y) {
        for (int x = 0; x < m_tileMap.getWidth(); ++x) {
            int tileType = m_tileMap.getTileType(x, y);
            if (tileType == 1) { // Ground tile
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
    for (auto* entity : m_entities) {
        if (entity && entity->isActive()) {
            entity->render(target);
        }
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
        m_tileMap.setTile(x, 20, 1); // Ground tile
    }

    // Spawn a DummyPhysicsEntity at (300, 50) with size (32, 32)
    m_entities.push_back(new DummyPhysicsEntity(sf::Vector2f(300.0f, 50.0f), sf::Vector2f(32.0f, 32.0f)));
}

void PlayingState::cleanupTestScene() {
    for (auto* entity : m_entities) {
        delete entity;
    }
    m_entities.clear();
}
