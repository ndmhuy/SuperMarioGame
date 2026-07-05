#include "Core/PlayingState.hpp"
#include "Core/MenuState.hpp"
#include "Core/Game.hpp"
#include "Core/InputManager.hpp"
#include "Entities/Mario.hpp"
#include "Entities/Luigi.hpp"
#include "Entities/Toad.hpp"
#include "Entities/Peach.hpp"
#include "Entities/Mushroom.hpp"
#include "Entities/FireFlower.hpp"
#include "Entities/Coin.hpp"
#include "Entities/Star.hpp"
#include "Entities/CapeFeather.hpp"
#include "Entities/MegaMushroom.hpp"
#include "Entities/MiniMushroom.hpp"
#include "Utils/Constants.hpp"
#include <SFML/Window/Event.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <imgui.h>
#include <iostream>
#include <cmath>
#include <algorithm>

PlayingState::PlayingState() = default;
PlayingState::~PlayingState() = default;

void PlayingState::enter() {
    std::cout << "Entering PlayingState (Physics & Input Test Playground)" << std::endl;
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
            return;
        }
    }

    if (m_player) {
        InputManager::getInstance().handleInput(event, *m_player);
    }
}

void PlayingState::update(float dt) {
    // 1. Process held keys (MoveLeft, MoveRight, Crouch, Run)
    if (m_player) {
        InputManager::getInstance().update(*m_player);
    }

    // 2. Update all active entities
    for (auto& entity : m_entities) {
        if (entity && entity->isActive()) {
            entity->update(dt);
        }
    }

    // 3. Run the physics engine (integrate velocity and resolve collisions)
    m_physicsEngine.update(m_entities, m_tileMap, dt);

    // 4. Update the Camera to follow player position and clamp to map boundaries
    if (m_player) {
        m_camera.follow(m_player->getPosition(), dt);
    }
    m_camera.update(dt);
}

void PlayingState::render(sf::RenderTarget& target) {
    // Set view to camera view for scrolling world space rendering
    target.setView(m_camera.getView());

    // 1. Draw the tilemap tiles
    for (int y = 0; y < m_tileMap.getHeight(); ++y) {
        for (int x = 0; x < m_tileMap.getWidth(); ++x) {
            TileType tileType = m_tileMap.getTileType(x, y);
            if (tileType == TileType::Empty) continue;

            const TileInfo& info = TileMap::getInfo(tileType);

            sf::RectangleShape tileShape(sf::Vector2f(Constants::TILE_SIZE, Constants::TILE_SIZE));
            tileShape.setPosition(sf::Vector2f(x * Constants::TILE_SIZE, y * Constants::TILE_SIZE));
            tileShape.setFillColor(info.debugColor);
            tileShape.setOutlineColor(sf::Color(60, 40, 20));
            tileShape.setOutlineThickness(0.5f);
            target.draw(tileShape);

            // Add a grass top layer to Ground tiles
            if (tileType == TileType::Ground) {
                sf::RectangleShape grassShape(sf::Vector2f(Constants::TILE_SIZE, 6.0f));
                grassShape.setPosition(sf::Vector2f(x * Constants::TILE_SIZE, y * Constants::TILE_SIZE));
                grassShape.setFillColor(sf::Color(46, 139, 87)); // Sea Green grass
                target.draw(grassShape);
            }
        }
    }

    // 2. Draw all active entities
    for (auto& entity : m_entities) {
        if (entity && entity->isActive()) {
            entity->render(target);
        }
    }

    // Reset view to default view for static screen space rendering (HUD, ImGui overlays)
    target.setView(target.getDefaultView());

    // 3. ImGui Physics Simulation & Input Test panel
    ImGui::Begin("Physics & Input Test Playground (Phase 2 & 3)");

    // Character Selection
    ImGui::Text("Select Active Character:");
    const char* characters[] = { "Mario (Red)", "Luigi (Green)", "Toad (Blue)", "Peach (Pink)" };
    int oldSelected = m_selectedCharIndex;
    if (ImGui::Combo("Character", &m_selectedCharIndex, characters, 4)) {
        if (m_selectedCharIndex != oldSelected && m_player) {
            // Respawn new character at the exact position of the current player
            spawnSelectedPlayer(m_player->getPosition());
        }
    }

    ImGui::Separator();

    if (m_player) {
        ImGui::Text("Character Stats:");
        ImGui::BulletText("Position: (%.2f, %.2f)", m_player->getPosition().x, m_player->getPosition().y);
        ImGui::BulletText("Velocity: (%.2f, %.2f)", m_player->getVelocity().x, m_player->getVelocity().y);
        ImGui::BulletText("onGround: %s", m_player->isOnGround() ? "TRUE" : "FALSE");
        ImGui::BulletText("onWall: %s", m_player->isOnWall() ? "TRUE" : "FALSE");
        ImGui::BulletText("Crouched: %s", m_player->isCrouched() ? "TRUE" : "FALSE");
        ImGui::BulletText("Sliding: %s", m_player->isSliding() ? "TRUE" : "FALSE");
        ImGui::BulletText("Coyote Frames Left: %d", m_player->getCoyoteFramesLeft());
        ImGui::BulletText("Jump Buffer Frames Left: %d", m_player->getJumpBufferFramesLeft());
        ImGui::BulletText("Lives: %d, Coins: %d, Score: %d", m_player->getLives(), m_player->getCoins(), m_player->getScore());
        
        // Active player state name lookup
        std::string stateName = "Unknown";
        if (IPlayerState* state = m_player->getCurrentState()) {
            IPlayerState* baseState = state;
            bool invincible = false;
            bool mega = false;
            while (auto* decorator = dynamic_cast<PlayerStateDecorator*>(baseState)) {
                if (dynamic_cast<StarDecorator*>(decorator)) invincible = true;
                if (dynamic_cast<MegaDecorator*>(decorator)) mega = true;
                baseState = decorator->getWrappedState();
            }

            if (dynamic_cast<SmallState*>(baseState)) stateName = "Small";
            else if (dynamic_cast<SuperState*>(baseState)) stateName = "Super";
            else if (dynamic_cast<FireState*>(baseState)) stateName = "Fire";
            else if (dynamic_cast<CapeState*>(baseState)) stateName = "Cape";
            else if (dynamic_cast<MiniState*>(baseState)) stateName = "Mini";

            if (invincible) stateName += " + Star (Invincible)";
            if (mega) stateName += " + Mega (Giant)";
        }
        ImGui::BulletText("Active Form: %s", stateName.c_str());
    } else {
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "No active player.");
    }

    ImGui::Separator();
    
    // Spawner buttons
    ImGui::Text("Spawn items at player position:");
    if (m_player) {
        sf::Vector2f spawnPos = m_player->getPosition() + sf::Vector2f(0.0f, -64.0f);
        if (ImGui::Button("Spawn Mushroom")) {
            m_entities.push_back(std::make_unique<Mushroom>(spawnPos));
        }
        ImGui::SameLine();
        if (ImGui::Button("Spawn Fire Flower")) {
            m_entities.push_back(std::make_unique<FireFlower>(spawnPos));
        }
        ImGui::SameLine();
        if (ImGui::Button("Spawn Coin")) {
            m_entities.push_back(std::make_unique<Coin>(spawnPos));
        }
        if (ImGui::Button("Spawn Star")) {
            m_entities.push_back(std::make_unique<Star>(spawnPos));
        }
        ImGui::SameLine();
        if (ImGui::Button("Spawn Cape Feather")) {
            m_entities.push_back(std::make_unique<CapeFeather>(spawnPos));
        }
        ImGui::SameLine();
        if (ImGui::Button("Spawn Mega Mushroom")) {
            m_entities.push_back(std::make_unique<MegaMushroom>(spawnPos));
        }
        if (ImGui::Button("Spawn Mini Mushroom")) {
            m_entities.push_back(std::make_unique<MiniMushroom>(spawnPos));
        }
    }

    ImGui::Separator();

    if (ImGui::Button("Swap Bricks <-> Coins (P-Switch test)")) {
        m_tileMap.swapBricksAndCoins();
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

    // 1. Initialize TileMap to 40 columns and 22 rows
    m_tileMap.initialize(40, 22);

    // Create a ground layer at row y = 20 and 21
    for (int x = 0; x < 40; ++x) {
        m_tileMap.setTile(x, 20, TileType::Ground);
        m_tileMap.setTile(x, 21, TileType::Ground);
    }

    // Place a vertical Pipe (pillar) at x = 10, 11 (y = 17, 18, 19)
    for (int y = 17; y <= 19; ++y) {
        m_tileMap.setTile(10, y, TileType::Pipe);
        m_tileMap.setTile(11, y, TileType::Pipe);
    }

    // Place a Brick wall at x = 5 (y = 18, 19)
    m_tileMap.setTile(5, 18, TileType::Brick);
    m_tileMap.setTile(5, 19, TileType::Brick);

    // Place an Ice block platform at x = 15..20 (y = 15)
    for (int x = 15; x <= 20; ++x) {
        m_tileMap.setTile(x, 15, TileType::Ice);
    }

    // Place a Conveyor Belt platform at x = 23..28 (y = 15)
    for (int x = 23; x <= 28; ++x) {
        m_tileMap.setTile(x, 15, TileType::Conveyor);
    }

    // Place a Water pool zone at x = 30..35 (y = 17..19)
    for (int x = 30; x <= 35; ++x) {
        for (int y = 17; y <= 19; ++y) {
            m_tileMap.setTile(x, y, TileType::Water);
        }
    }

    // 2. Spawn Player and configure camera boundaries
    spawnSelectedPlayer(sf::Vector2f(100.0f, 100.0f));
    m_camera.setBounds(AABB{0.0f, 0.0f, m_tileMap.getWidth() * Constants::TILE_SIZE, m_tileMap.getHeight() * Constants::TILE_SIZE});

    // 3. Spawn Items
    m_entities.push_back(std::make_unique<Mushroom>(sf::Vector2f(450.0f, 400.0f)));
    m_entities.push_back(std::make_unique<FireFlower>(sf::Vector2f(550.0f, 500.0f)));
    m_entities.push_back(std::make_unique<Coin>(sf::Vector2f(650.0f, 400.0f)));
    m_entities.push_back(std::make_unique<Star>(sf::Vector2f(750.0f, 400.0f)));
    m_entities.push_back(std::make_unique<CapeFeather>(sf::Vector2f(850.0f, 400.0f)));
    m_entities.push_back(std::make_unique<MegaMushroom>(sf::Vector2f(950.0f, 400.0f)));
    m_entities.push_back(std::make_unique<MiniMushroom>(sf::Vector2f(1050.0f, 400.0f)));
}

void PlayingState::cleanupTestScene() {
    m_entities.clear();
    m_player = nullptr;
}

void PlayingState::spawnSelectedPlayer(const sf::Vector2f& pos) {
    int oldCoins = 0;
    int oldScore = 0;
    int oldLives = 3;
    if (m_player) {
        oldCoins = m_player->getCoins();
        oldScore = m_player->getScore();
        oldLives = m_player->getLives();

        auto it = std::find_if(m_entities.begin(), m_entities.end(), [this](const std::unique_ptr<Entity>& e) {
            return e.get() == m_player;
        });
        if (it != m_entities.end()) {
            m_entities.erase(it);
        }
        m_player = nullptr;
    }

    std::unique_ptr<Player> newPlayer;
    if (m_selectedCharIndex == 0) {
        newPlayer = std::make_unique<Mario>(pos);
    } else if (m_selectedCharIndex == 1) {
        newPlayer = std::make_unique<Luigi>(pos);
    } else if (m_selectedCharIndex == 2) {
        newPlayer = std::make_unique<Toad>(pos);
    } else {
        newPlayer = std::make_unique<Peach>(pos);
    }

    // Restore stats
    if (oldCoins > 0) newPlayer->addCoins(oldCoins);
    if (oldScore > 0) newPlayer->addScore(oldScore);
    while (newPlayer->getLives() < oldLives) newPlayer->gainLife();
    while (newPlayer->getLives() > oldLives) newPlayer->loseLife();

    m_player = newPlayer.get();
    m_entities.insert(m_entities.begin(), std::move(newPlayer));

    InputManager::getInstance().registerPlayer(m_player, 0);
}

