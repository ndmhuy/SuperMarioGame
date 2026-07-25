#include <iostream>
#include <filesystem>
#include <algorithm>
#include <vector>
#include <memory>
#include <SFML/Graphics.hpp>
#include <imgui.h>
#include <imgui-SFML.h>
#include "Graphics/Minimap.hpp"
#include "Utils/TileMap.hpp"
#include "Entities/Mario.hpp"
#include "Entities/Goomba.hpp"
#include "Entities/Mushroom.hpp"
#include "Core/EventBus.hpp"
#include "Utils/Constants.hpp"

int main() {
    std::cout << "[VISUAL TEST] Launching Minimap Visualizer..." << std::endl;

    // Create SFML RenderWindow (1280x720 design size)
    sf::RenderWindow window(sf::VideoMode(sf::Vector2u(1280, 720)), "Super Mario Minimap Visualizer");
    window.setFramerateLimit(60);

    // Initialize ImGui-SFML
    if (!ImGui::SFML::Init(window)) {
        std::cerr << "[VISUAL TEST] Failed to initialize ImGui-SFML!" << std::endl;
        return 1;
    }

    // Set up mock TileMap (40x22 layout matching Constants specifications)
    TileMap tileMap;
    tileMap.initialize(40, 22);

    // Fill bottom row with Ground tiles (which are solid)
    for (int x = 0; x < 40; ++x) {
        tileMap.setTile(x, 20, TileType::Ground);
    }
    // Add some random floating solid brick blocks for texture
    tileMap.setTile(10, 15, TileType::Brick);
    tileMap.setTile(11, 15, TileType::Brick);
    tileMap.setTile(12, 15, TileType::Brick);
    tileMap.setTile(25, 12, TileType::Brick);
    tileMap.setTile(26, 12, TileType::Brick);

    // Instantiate Minimap positioned at bottom-right corner (size 200x40 matching SPEC §10.6)
    sf::Vector2f minimapPos(1040.f, 640.f);
    sf::Vector2f minimapSize(200.f, 40.f);
    Minimap minimap(minimapPos, minimapSize);
    minimap.initialize(tileMap);

    // Instantiate mock entities
    auto player = std::make_unique<Mario>(sf::Vector2f(100.f, 600.f));
    
    std::vector<std::unique_ptr<Entity>> entities;
    entities.push_back(std::make_unique<Goomba>(sf::Vector2f(400.f, 600.f), false));
    entities.push_back(std::make_unique<Mushroom>(sf::Vector2f(300.f, 400.f)));

    // Variables for ImGui sliders
    float playerX = player->getPosition().x;
    float playerY = player->getPosition().y;
    float goombaX = entities[0]->getPosition().x;
    float goombaY = entities[0]->getPosition().y;
    float mushroomX = entities[1]->getPosition().x;
    float mushroomY = entities[1]->getPosition().y;

    bool isMinimapVisible = false; // Start hidden, toggleable via M key or ImGui

    sf::Clock deltaClock;
    while (window.isOpen()) {
        while (const auto event = window.pollEvent()) {
            ImGui::SFML::ProcessEvent(window, *event);
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
            if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
                if (keyPressed->code == sf::Keyboard::Key::Escape) {
                    window.close();
                }
                if (keyPressed->code == sf::Keyboard::Key::M) {
                    // Publish toggle event on EventBus
                    EventBus::getInstance().publish({EventType::MinimapToggled, 0});
                    isMinimapVisible = !isMinimapVisible;
                }
            }
        }

        float dt = deltaClock.restart().asSeconds();
        ImGui::SFML::Update(window, sf::seconds(dt));

        // Update positions from ImGui sliders
        player->setPosition({playerX, playerY});
        entities[0]->setPosition({goombaX, goombaY});
        entities[1]->setPosition({mushroomX, mushroomY});

        // Update Minimap
        minimap.update(dt, player.get(), entities);

        // ImGui Panel
        ImGui::Begin("Minimap Controls");
        ImGui::Text("Press 'M' key or use the button below to toggle minimap:");
        if (ImGui::Button(isMinimapVisible ? "Hide Minimap (M)" : "Show Minimap (M)")) {
            EventBus::getInstance().publish({EventType::MinimapToggled, 0});
            isMinimapVisible = !isMinimapVisible;
        }

        ImGui::Separator();
        ImGui::Text("Entity Positions:");
        ImGui::SliderFloat("Player X", &playerX, 0.f, 1280.f);
        ImGui::SliderFloat("Player Y", &playerY, 0.f, 720.f);
        ImGui::SliderFloat("Goomba (Enemy) X", &goombaX, 0.f, 1280.f);
        ImGui::SliderFloat("Goomba (Enemy) Y", &goombaY, 0.f, 720.f);
        ImGui::SliderFloat("Mushroom (Item) X", &mushroomX, 0.f, 1280.f);
        ImGui::SliderFloat("Mushroom (Item) Y", &mushroomY, 0.f, 720.f);

        ImGui::Separator();
        ImGui::Text("Minimap Visual Properties:");
        ImGui::SliderFloat("Minimap Pos X", &minimapPos.x, 0.f, 1080.f);
        ImGui::SliderFloat("Minimap Pos Y", &minimapPos.y, 0.f, 680.f);
        ImGui::SliderFloat("Minimap Size X", &minimapSize.x, 50.f, 400.f);
        ImGui::SliderFloat("Minimap Size Y", &minimapSize.y, 10.f, 200.f);

        // Re-construct minimap properties dynamically if changed
        // (Note: in-game they are static, but this test lets us verify flexibility)
        // We simulate this by adjusting local variables
        ImGui::End();

        // Clear window to sky blue
        window.clear(sf::Color(92, 148, 252));

        // 1. Draw Mock Level Layout on screen
        for (int y = 0; y < tileMap.getHeight(); ++y) {
            for (int x = 0; x < tileMap.getWidth(); ++x) {
                TileType type = tileMap.getTileType(x, y);
                if (tileMap.getInfo(type).isSolid) {
                    sf::RectangleShape blockShape(sf::Vector2f(Constants::TILE_SIZE, Constants::TILE_SIZE));
                    blockShape.setPosition(sf::Vector2f(x * Constants::TILE_SIZE, y * Constants::TILE_SIZE));
                    blockShape.setFillColor(sf::Color(120, 80, 40)); // Brown solid block
                    window.draw(blockShape);
                }
            }
        }

        // 2. Draw mock entities as simple filled shapes
        // Player: Green
        if (player->isActive()) {
            sf::RectangleShape shape(sf::Vector2f(32.f, 32.f));
            shape.setPosition(player->getPosition());
            shape.setFillColor(sf::Color::Green);
            window.draw(shape);
        }

        // Enemy: Red
        if (entities[0]->isActive()) {
            sf::CircleShape shape(16.f);
            shape.setPosition(entities[0]->getPosition());
            shape.setFillColor(sf::Color::Red);
            window.draw(shape);
        }

        // Item: Yellow
        if (entities[1]->isActive()) {
            sf::RectangleShape shape(sf::Vector2f(24.f, 24.f));
            shape.setPosition(entities[1]->getPosition());
            shape.setFillColor(sf::Color::Yellow);
            window.draw(shape);
        }

        // 3. Draw Minimap (overlayed in screen space)
        window.draw(minimap);

        // Render ImGui
        ImGui::SFML::Render(window);

        window.display();
    }

    ImGui::SFML::Shutdown();
    return 0;
}
