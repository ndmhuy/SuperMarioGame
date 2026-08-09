#include "Core/PlayingState.hpp"
#include "Core/MenuState.hpp"
#include "Core/Game.hpp"
#include "Core/ResourceManager.hpp"
#include "Graphics/Hud.hpp"
#include "Graphics/ScreenTransitionManager.hpp"
#include "Entities/Entity.hpp"
#include "Core/EventBus.hpp"
#include "Core/StatisticsTracker.hpp"
#include "Core/AchievementManager.hpp"
#include "Core/InputManager.hpp"
#include "Entities/Player.hpp"
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
#include "Entities/Fireball.hpp"
#include "Entities/EntityFactory.hpp"
#include "Core/SoundManager.hpp"
#include "Utils/Constants.hpp"
#include "Utils/Serializer.hpp"
#include <SFML/Window/Event.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <imgui.h>
#include <iostream>
#include <cmath>
#include <algorithm>

PlayingState::PlayingState(bool startInEditor, bool isProcedural, const MapGeneratorConfig& genConfig)
    : m_startInEditor(startInEditor), m_isProcedural(isProcedural), m_genConfig(genConfig) {}

PlayingState::~PlayingState() = default;

void PlayingState::enter() {
    std::cout << "Entering PlayingState (startInEditor: " << m_startInEditor << ", isProcedural: " << m_isProcedural << ")" << std::endl;
    // Initialize HUD and Level Timer
    m_hud = std::make_unique<Hud>(sf::Vector2i(Constants::WINDOW_WIDTH, Constants::WINDOW_HEIGHT));
    m_levelTimer = 300.0f;

    if (m_isProcedural) {
        cleanupTestScene();
        MapGenerator::generate(m_tileMap, m_entities, m_genConfig);
        
        m_player = nullptr;
        for (const auto& entity : m_entities) {
            if (auto p = dynamic_cast<Player*>(entity.get())) {
                m_player = p;
                break;
            }
        }
        if (!m_player) {
            spawnSelectedPlayer(sf::Vector2f(96.0f, 500.0f));
        } else {
            InputManager::getInstance().registerPlayer(m_player, 0);
            Game::getInstance().setPlayer(m_player);
        }
        Game::getInstance().setTileMap(&m_tileMap);
        m_camera.setBounds(AABB{0.0f, 0.0f, m_tileMap.getWidth() * Constants::TILE_SIZE, m_tileMap.getHeight() * Constants::TILE_SIZE});
    } else {
        setupTestScene();
    }

    if (m_startInEditor && !m_mapEditor.isActive()) {
        m_mapEditor.toggleActive();
    }

    // Auto-save at checkpoint
    m_checkpointSubId = EventBus::getInstance().subscribe(EventType::CheckpointActivated, [this](const GameEvent& ev) {
        int activeSlot = Game::getInstance().getActiveSlot();
        if (!m_entities.empty() && m_entities[0]) {
            if (auto* player = dynamic_cast<Player*>(m_entities[0].get())) {
                bool success = Serializer::saveGame(activeSlot, *player, 1, "Level 1", Constants::LEVEL_TIME, player->getPosition().x, player->getPosition().y, {true, false, false});
                if (success) {
                    std::cout << "[Auto-Save] Progress saved to Slot " << activeSlot << " at checkpoint!" << std::endl;
                }
            }
        }
    });

    // Fireball Shooting Event Listener
    m_fireballSubId = EventBus::getInstance().subscribe(EventType::PlayerShotFireball, [this](const GameEvent& ev) {
        auto* player = std::any_cast<Player*>(ev.data);
        if (!player) return;

        // Count active fireballs
        int activeFireballs = 0;
        for (const auto& entity : m_entities) {
            if (auto fb = dynamic_cast<Fireball*>(entity.get())) {
                if (fb->isActive()) {
                    activeFireballs++;
                }
            }
        }

        if (activeFireballs < 2) { // Max 2 active fireballs on screen
            float dir = player->isFacingRight() ? 1.0f : -1.0f;
            sf::Vector2f spawnPos = player->getPosition() + sf::Vector2f(dir * 16.0f, 8.0f);
            sf::Vector2f vel(dir * 350.0f, 50.0f);

            auto fireball = EntityFactory::createFireball(spawnPos, vel);
            m_entities.push_back(std::move(fireball));

            SoundManager::getInstance().playSound("fireball");
        }
    });
}

void PlayingState::exit() {
    std::cout << "Exiting PlayingState" << std::endl;
    cleanupTestScene();

    if (m_checkpointSubId != static_cast<EventBus::SubscriptionId>(-1)) {
        EventBus::getInstance().unsubscribe(m_checkpointSubId);
        m_checkpointSubId = static_cast<EventBus::SubscriptionId>(-1);
    }
    if (m_fireballSubId != static_cast<EventBus::SubscriptionId>(-1)) {
        EventBus::getInstance().unsubscribe(m_fireballSubId);
        m_fireballSubId = static_cast<EventBus::SubscriptionId>(-1);
    }

    // Unregister player from InputManager to prevent dangling pointer crashes on exit
    InputManager::getInstance().registerPlayer(nullptr, 0);
}

void PlayingState::handleInput(const sf::Event& event) {
    if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>()) {
        if (keyPressed->code == sf::Keyboard::Key::F1) {
            m_mapEditor.toggleActive();
            std::cout << "[Editor] F1 pressed. Active = " << m_mapEditor.isActive() << std::endl;
        }

        if (!m_mapEditor.isActive()) {
            if (keyPressed->code == sf::Keyboard::Key::Backspace) {
                Game::getInstance().changeState(std::make_unique<MenuState>());
            }

            EventBus& bus = EventBus::getInstance();
            switch (keyPressed->code) {
                case sf::Keyboard::Key::Num1: // Collect Coin
                    bus.publish({EventType::CoinCollected, 1});
                    break;
                case sf::Keyboard::Key::Num2: // Defeat Enemy
                    bus.publish({EventType::EnemyDefeated, 0});
                    break;
                case sf::Keyboard::Key::Num3: // Take Damage
                    bus.publish({EventType::PlayerDamaged, 0});
                    break;
                case sf::Keyboard::Key::Num4: // Lose Life / Die
                    bus.publish({EventType::PlayerDied, 0});
                    break;
                case sf::Keyboard::Key::Num5: // Checkpoint Activated
                    bus.publish({EventType::CheckpointActivated, 0});
                    break;
                case sf::Keyboard::Key::Num6: // Finish Level 3 (unlocks character achievements)
                    bus.publish({EventType::LevelComplete, 3});
                    break;
                case sf::Keyboard::Key::Num7: // Boss Defeated
                    bus.publish({EventType::BossDefeated, 0});
                    break;
                case sf::Keyboard::Key::Num8: // Star Coin Collected
                    bus.publish({EventType::StarCoinCollected, 0});
                    break;
                case sf::Keyboard::Key::Num9: // Hidden Block Broken
                    bus.publish({EventType::BlockBroken, 0});
                    break;
                default:
                    break;
            }
        }
    }

    if (!m_mapEditor.isActive() && m_player) {
        InputManager::getInstance().handleInput(event, *m_player);
    }
}

void PlayingState::update(float dt) {
    if (m_mapEditor.isActive()) {
        sf::View defaultView(sf::FloatRect({0.f, 0.f}, {1280.f, 720.f}));
        sf::Vector2f mouseWorldPos = Game::getInstance().getMouseWorldPosition(defaultView);
        m_mapEditor.update(m_tileMap, m_entities, mouseWorldPos, dt);
        return;
    }

    // 1. Update trackers
    StatisticsTracker::getInstance().update(dt);
    AchievementManager::getInstance().update(dt);

    // 0. Time Rewind check (Hold R key to rewind time backwards using Memento snapshots)
    bool rewindRequested = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::R);

    if (rewindRequested && m_rewindManager.hasSnapshots()) {
        m_rewindManager.setRewinding(true);
        GameSnapshot snapshot = m_rewindManager.popSnapshot();

        m_levelTimer = snapshot.levelTimer;
        m_camera.getView().setCenter(snapshot.cameraCenter);

        if (m_player) {
            m_player->setPosition(snapshot.playerState.position);
            m_player->setVelocity(snapshot.playerState.velocity);
            m_player->score = snapshot.playerState.score;
            m_player->coins = snapshot.playerState.coins;
            m_player->lives = snapshot.playerState.lives;
            m_player->onGround = snapshot.playerState.onGround;
        }

        // Restore active entities states
        for (std::size_t i = 0; i < m_entities.size() && i < snapshot.entityStates.size(); ++i) {
            if (m_entities[i]) {
                m_entities[i]->setPosition(snapshot.entityStates[i].position);
                m_entities[i]->setVelocity(snapshot.entityStates[i].velocity);
                m_entities[i]->active = snapshot.entityStates[i].active;
            }
        }
        return; // Skip forward physics integration while rewinding
    } else {
        m_rewindManager.setRewinding(false);
    }

    // 2. Process held keys (MoveLeft, MoveRight, Crouch, Run)
    if (m_player) {
        InputManager::getInstance().update(*m_player);
    }

    // 3. Update all active entities
    for (auto& entity : m_entities) {
        if (entity && entity->isActive()) {
            entity->update(dt);
        }
    }

    // 3. Run the physics engine pipeline (apply gravity, integrate velocity, check/resolve collisions)
    m_physicsEngine.update(m_entities, m_tileMap, dt);

    // 3b. Prune inactive entities (keep m_player intact even if inactive)
    m_entities.erase(
        std::remove_if(m_entities.begin(), m_entities.end(), [this](const std::unique_ptr<Entity>& entity) {
            return entity && !entity->isActive() && entity.get() != m_player;
        }),
        m_entities.end()
    );

    // 4. Update Camera & Screen Transitions
    if (m_player) {
        m_camera.follow(m_player->getPosition(), dt);
    }
    m_camera.update(dt);
    ScreenTransitionManager::getInstance().update(dt);

    // 5. Sync HUD with player stats or fallback mock data
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

    // Record GameSnapshot Memento state at end of frame update
    if (!m_rewindManager.isRewinding()) {
        GameSnapshot snapshot;
        snapshot.levelTimer = m_levelTimer;
        snapshot.cameraCenter = m_camera.getView().getCenter();

        if (m_player) {
            snapshot.playerState.position = m_player->getPosition();
            snapshot.playerState.velocity = m_player->getVelocity();
            snapshot.playerState.score = m_player->getScore();
            snapshot.playerState.coins = m_player->getCoins();
            snapshot.playerState.lives = m_player->getLives();
            snapshot.playerState.onGround = m_player->isOnGround();
        }

        for (const auto& entity : m_entities) {
            if (entity) {
                snapshot.entityStates.push_back({
                    entity->getPosition(),
                    entity->getVelocity(),
                    entity->isActive()
                });
            }
        }

        m_rewindManager.recordSnapshot(snapshot);
    }
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
            } else if (tileType == TileType::Brick) {
                sf::RectangleShape tileShape(sf::Vector2f(Constants::TILE_SIZE, Constants::TILE_SIZE));
                tileShape.setPosition(sf::Vector2f(x * Constants::TILE_SIZE, y * Constants::TILE_SIZE));
                tileShape.setFillColor(sf::Color(180, 50, 50)); // Reddish brown brick
                tileShape.setOutlineColor(sf::Color(90, 25, 25));
                tileShape.setOutlineThickness(1.0f);
                target.draw(tileShape);
            } else if (tileType == TileType::Question) {
                sf::RectangleShape tileShape(sf::Vector2f(Constants::TILE_SIZE, Constants::TILE_SIZE));
                tileShape.setPosition(sf::Vector2f(x * Constants::TILE_SIZE, y * Constants::TILE_SIZE));
                tileShape.setFillColor(sf::Color(240, 180, 30)); // Yellow Question block
                tileShape.setOutlineColor(sf::Color(120, 90, 15));
                tileShape.setOutlineThickness(1.0f);
                target.draw(tileShape);
            } else if (tileType == TileType::Pipe) {
                sf::RectangleShape tileShape(sf::Vector2f(Constants::TILE_SIZE, Constants::TILE_SIZE));
                tileShape.setPosition(sf::Vector2f(x * Constants::TILE_SIZE, y * Constants::TILE_SIZE));
                tileShape.setFillColor(sf::Color(0, 150, 0)); // Green pipe
                tileShape.setOutlineColor(sf::Color(0, 75, 0));
                tileShape.setOutlineThickness(1.0f);
                target.draw(tileShape);
            } else if (tileType == TileType::Ice) {
                sf::RectangleShape tileShape(sf::Vector2f(Constants::TILE_SIZE, Constants::TILE_SIZE));
                tileShape.setPosition(sf::Vector2f(x * Constants::TILE_SIZE, y * Constants::TILE_SIZE));
                tileShape.setFillColor(sf::Color(150, 200, 250)); // Light blue Ice
                target.draw(tileShape);
            } else if (tileType == TileType::Conveyor) {
                sf::RectangleShape tileShape(sf::Vector2f(Constants::TILE_SIZE, Constants::TILE_SIZE));
                tileShape.setPosition(sf::Vector2f(x * Constants::TILE_SIZE, y * Constants::TILE_SIZE));
                tileShape.setFillColor(sf::Color(100, 100, 100)); // Dark grey conveyor
                tileShape.setOutlineColor(sf::Color(50, 50, 50));
                tileShape.setOutlineThickness(1.0f);
                target.draw(tileShape);
            } else if (tileType == TileType::Water) {
                sf::RectangleShape tileShape(sf::Vector2f(Constants::TILE_SIZE, Constants::TILE_SIZE));
                tileShape.setPosition(sf::Vector2f(x * Constants::TILE_SIZE, y * Constants::TILE_SIZE));
                tileShape.setFillColor(sf::Color(30, 80, 220, 150)); // Blue transparent water
                target.draw(tileShape);
            } else if (tileType == TileType::Coin) {
                sf::RectangleShape tileShape(sf::Vector2f(Constants::TILE_SIZE, Constants::TILE_SIZE));
                tileShape.setPosition(sf::Vector2f(x * Constants::TILE_SIZE, y * Constants::TILE_SIZE));
                tileShape.setFillColor(sf::Color(255, 215, 0)); // Gold coin tile
                tileShape.setOutlineColor(sf::Color(150, 120, 0));
                tileShape.setOutlineThickness(1.0f);
                target.draw(tileShape);
            }
        }
    }

    // 2. Draw all active entities
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

        if (m_rewindManager.isRewinding()) {
            // Full-screen cyan vignette scanline filter overlay
            sf::RectangleShape rewindOverlay(sf::Vector2f(Constants::WINDOW_WIDTH, Constants::WINDOW_HEIGHT));
            rewindOverlay.setFillColor(sf::Color(0, 200, 255, 45));
            target.draw(rewindOverlay);

            sf::RectangleShape rewindBox(sf::Vector2f(420.0f, 44.0f));
            rewindBox.setPosition({430.0f, 65.0f});
            rewindBox.setFillColor(sf::Color(0, 0, 0, 220));
            rewindBox.setOutlineColor(sf::Color(0, 255, 255));
            rewindBox.setOutlineThickness(2.0f);
            target.draw(rewindBox);

            sf::Text rewindText(ResourceManager::getInstance().getFont("PressStart2P"));
            rewindText.setString("<< MEMENTO TIME REWINDING (" + std::to_string(m_rewindManager.getSnapshotCount()) + ")");
            rewindText.setCharacterSize(12);
            rewindText.setFillColor(sf::Color(0, 255, 255));
            rewindText.setPosition({445.0f, 80.0f});
            target.draw(rewindText);
        }

        target.setView(oldView);
    }

    // Render screen transitions overlay
    ScreenTransitionManager::getInstance().render(target);

    // ImGui Panel for controlling and monitoring the physics simulation
    ImGui::Begin("Physics Simulation (Phase 2)");
    ImGui::Text("Simulation State:");
    if (!m_entities.empty() && m_entities[0]) {
        ImGui::Text("Entity Position: (%.1f, %.1f)", m_entities[0]->getPosition().x, m_entities[0]->getPosition().y);
        ImGui::Text("Entity Velocity: (%.1f, %.1f)", m_entities[0]->getVelocity().x, m_entities[0]->getVelocity().y);
    } else {
        ImGui::Text("No active entities.");
    }
    ImGui::End();
    // Draw Map Editor overlays if active
    if (m_mapEditor.isActive()) {
        m_mapEditor.render(target, m_tileMap, m_entities);
        m_mapEditor.renderImGui(m_tileMap, m_entities);
    } else {
        // Reset view to default view for static screen space rendering (HUD, ImGui overlays)
        target.setView(target.getDefaultView());

        // --- ImGui Dev Interface for Phase 2, 3 & 4 (Playground) ---
        ImGui::Begin("Physics & Input Test Playground (Phase 2 & 3)");

        // Character Selection
        ImGui::Text("Select Active Character:");
        const char* characters[] = { "Mario (Red)", "Luigi (Green)", "Toad (Blue)", "Peach (Pink)" };
        int oldCharSelected = m_selectedCharIndex;
        if (ImGui::Combo("Character", &m_selectedCharIndex, characters, 4)) {
            if (m_selectedCharIndex != oldCharSelected && m_player) {
                spawnSelectedPlayer(m_player->getPosition());
            }
        }

        // Level Selection
        ImGui::Text("Select Active Level:");
        const char* levels[] = { "Level 1 (Grassland)", "Level 2 (Cave)", "Level 3 (Castle)", "Bonus 1 (Sky)" };
        int oldLevelSelected = m_selectedLevelIndex;
        if (ImGui::Combo("Level", &m_selectedLevelIndex, levels, 4)) {
            if (m_selectedLevelIndex != oldLevelSelected) {
                setupTestScene();
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

        ImGui::End();

        // --- ImGui Dev Interface for Phase 8 (Save/Load & Persistence) ---
        ImGui::Begin("Save/Load & Persistence (Phase 8)");

        Player* player = (!m_entities.empty() && m_entities[0]) ? dynamic_cast<Player*>(m_entities[0].get()) : nullptr;
        if (player) {
            ImGui::Text("Active Slot: %d", Game::getInstance().getActiveSlot());
            ImGui::Text("Active Character: %s", (dynamic_cast<Mario*>(player) ? "Mario" : (dynamic_cast<Luigi*>(player) ? "Luigi" : "Other")));
            ImGui::Text("Lives: %d | Coins: %d | Score: %d", player->getLives(), player->getCoins(), player->getScore());
            ImGui::Text("Position: (%.1f, %.1f)", player->getPosition().x, player->getPosition().y);
        } else {
            ImGui::Text("No active player character loaded.");
        }
        
        ImGui::Separator();

        // 1. Settings Persistence Tab
        if (ImGui::CollapsingHeader("Settings Configuration")) {
            float sfx = Game::getInstance().getSfxVolume();
            float music = Game::getInstance().getMusicVolume();
            bool colorblind = Game::getInstance().getColorblindMode();

            if (ImGui::SliderFloat("SFX Volume", &sfx, 0.0f, 100.0f)) {
                Game::getInstance().setSfxVolume(sfx);
            }
            if (ImGui::SliderFloat("Music Volume", &music, 0.0f, 100.0f)) {
                Game::getInstance().setMusicVolume(music);
            }
            if (ImGui::Checkbox("Colorblind Mode", &colorblind)) {
                Game::getInstance().setColorblindMode(colorblind);
            }

            std::string diff = Game::getInstance().getDifficulty();
            const char* difficulties[] = { "easy", "normal", "hard" };
            int activeDiffIdx = 0;
            for (int i = 0; i < 3; ++i) {
                if (diff == difficulties[i]) activeDiffIdx = i;
            }
            if (ImGui::Combo("Difficulty", &activeDiffIdx, difficulties, 3)) {
                Game::getInstance().setDifficulty(difficulties[activeDiffIdx]);
            }
        }

        // 2. Save Slots Persistence Tab
        if (ImGui::CollapsingHeader("Save/Load Slots")) {
            for (int slot = 1; slot <= 3; ++slot) {
                ImGui::PushID(slot);
                SaveSlotPreview preview = Serializer::getSlotPreview(slot);
                if (preview.exists) {
                    ImGui::Text("Slot %d: [%s] Lvl:%d (%s) Score:%d, Star Coins:%d, Play Time:%.1fs, Saved:%s",
                                slot, preview.character.c_str(), preview.levelId, preview.levelName.c_str(),
                                preview.score, preview.starCoinsCount, preview.playTime, preview.timestamp.c_str());
                } else {
                    ImGui::Text("Slot %d: Empty", slot);
                }

                if (ImGui::Button("Save")) {
                    if (player) {
                        Serializer::saveGame(slot, *player, 1, "Level 1", Constants::LEVEL_TIME, player->getPosition().x, player->getPosition().y, {true, false, false});
                    }
                }
                ImGui::SameLine();
                if (ImGui::Button("Load")) {
                    std::unique_ptr<Player> loadedPlayer;
                    int lvlId;
                    std::string lvlName;
                    float timeRem, checkX, checkY;
                    std::vector<bool> starCoins;
                    bool success = Serializer::loadGame(slot, loadedPlayer, lvlId, lvlName, timeRem, checkX, checkY, starCoins);
                    if (success && loadedPlayer) {
                        m_entities[0] = std::move(loadedPlayer);
                        Game::getInstance().setActiveSlot(slot);
                        std::cout << "Loaded save slot " << slot << " successfully!" << std::endl;
                    }
                }
                ImGui::SameLine();
                if (ImGui::Button("Delete")) {
                    Serializer::deleteSlot(slot);
                }
                ImGui::Separator();
                ImGui::PopID();
            }
        }

        // 3. Statistics Persistence Tab
        if (ImGui::CollapsingHeader("Session & Overall Statistics")) {
            const auto& stats = StatisticsTracker::getInstance().getStats();
            ImGui::Text("Total Enemies Defeated: %d", stats.totalEnemiesDefeated);
            ImGui::Text("Total Coins Collected: %d", stats.totalCoinsCollected);
            ImGui::Text("Total Deaths: %d", stats.totalDeaths);
            ImGui::Text("Total Time Played: %.1fs", stats.totalTimePlayed);
            ImGui::Text("Highest Combo: %d", stats.highestCombo);
            if (ImGui::Button("Reset Stats")) {
                StatisticsTracker::getInstance().reset();
            }
        }

        // 4. Achievements Persistence Tab
        if (ImGui::CollapsingHeader("Achievements Monitor")) {
            const auto& achievements = AchievementManager::getInstance().getAchievements();
            int unlockedCount = 0;
            for (const auto& a : achievements) {
                if (a.unlocked) unlockedCount++;
            }
            ImGui::Text("Unlocked: %d / %d", unlockedCount, static_cast<int>(achievements.size()));
            ImGui::Separator();
            for (const auto& a : achievements) {
                ImGui::Text("[%s] %s: %s (%s)",
                            (a.unlocked ? "UNLOCKED" : "LOCKED"),
                            a.name.c_str(),
                            a.condition.c_str(),
                            a.icon.c_str());
            }
        }

        ImGui::Separator();
        if (ImGui::Button("Reset Game Data (Lock all & wipe slot 1)")) {
            AchievementManager::getInstance().reset();
            StatisticsTracker::getInstance().reset();
            Serializer::deleteSlot(1);
        }
        
        ImGui::TextDisabled("\nSimulation Keyboard Testing Commands:\n"
                            "- Num1: Collect coin     - Num2: Defeat enemy\n"
                            "- Num3: Take damage      - Num4: Lose life/Die\n"
                            "- Num5: Cross Checkpoint - Num6: Clear Level 3\n"
                            "- Num7: Defeat Bowser    - Num8: Collect star coin\n"
                            "- Num9: Find hidden block");

        ImGui::End();

        // --- Overlay Toast Notifications ---
        const auto& toasts = AchievementManager::getInstance().getActiveToasts();
        if (!toasts.empty()) {
            ImGui::SetNextWindowPos(ImVec2(Constants::WINDOW_WIDTH - 320.0f, 20.0f), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(300.0f, 80.0f * toasts.size()), ImGuiCond_Always);
            ImGui::Begin("Achievements Toasts Overlay", nullptr,
                         ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
                         ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
                         ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoBackground);
            
            for (const auto& toast : toasts) {
                ImGui::PushStyleVar(ImGuiStyleVar_Alpha, toast.alpha);
                ImGui::BeginChild(toast.id.c_str(), ImVec2(290, 70), true);
                ImGui::TextColored(ImVec4(1.0f, 0.84f, 0.0f, 1.0f), "Achievement Unlocked!");
                ImGui::Text("[%s] %s", toast.icon.c_str(), toast.name.c_str());
                ImGui::EndChild();
                ImGui::PopStyleVar();
            }
            ImGui::End();
        }
    }
}

void PlayingState::setupTestScene() {
    cleanupTestScene();

    LevelLoader loader;
    LevelData levelData;
    std::string levelPath = "assets/levels/level_1.json";
    if (m_selectedLevelIndex == 1) levelPath = "assets/levels/level_2.json";
    else if (m_selectedLevelIndex == 2) levelPath = "assets/levels/level_3.json";
    else if (m_selectedLevelIndex == 3) levelPath = "assets/levels/bonus_1.json";

    std::vector<std::string> pathCandidates = {
        levelPath,
        "SuperMarioGame/" + levelPath,
        "../" + levelPath
    };
    std::string chosenPath = levelPath;
    for (const auto& candidate : pathCandidates) {
        if (std::filesystem::exists(candidate)) {
            chosenPath = candidate;
            break;
        }
    }

    if (loader.loadLevel(chosenPath, m_tileMap, levelData)) {
        // Spawn active player character at the spawnPoint loaded from JSON
        spawnSelectedPlayer(levelData.spawnPoint);
        
        // Transfer all loaded items/entities to m_entities
        for (auto& entity : levelData.entities) {
            m_entities.push_back(std::move(entity));
        }

        // Set camera bounds matching the level size
        m_camera.setBounds(AABB{0.0f, 0.0f, levelData.width * Constants::TILE_SIZE, levelData.height * Constants::TILE_SIZE});
    } else {
        // Fallback: manually setup scene if file loading fails
        m_tileMap.initialize(40, 22);
        for (int x = 0; x < 40; ++x) {
            m_tileMap.setTile(x, 20, TileType::Ground);
            m_tileMap.setTile(x, 21, TileType::Ground);
        }
        spawnSelectedPlayer(sf::Vector2f(100.0f, 100.0f));
        m_camera.setBounds(AABB{0.0f, 0.0f, 40.0f * Constants::TILE_SIZE, 22.0f * Constants::TILE_SIZE});
    }
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

