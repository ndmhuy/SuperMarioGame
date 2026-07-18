#include "Core/PlayingState.hpp"
#include "Core/MenuState.hpp"
#include "Core/Game.hpp"
#include "Core/EventBus.hpp"
#include "Core/StatisticsTracker.hpp"
#include "Core/AchievementManager.hpp"
#include "Entities/Player.hpp"
#include "Entities/Mario.hpp"
#include "Entities/Luigi.hpp"
#include "Entities/Toad.hpp"
#include "Entities/Peach.hpp"
#include "Utils/Constants.hpp"
#include "Utils/Serializer.hpp"
#include <SFML/Window/Event.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <imgui.h>
#include <iostream>

PlayingState::PlayingState() = default;
PlayingState::~PlayingState() = default;

void PlayingState::enter() {
    std::cout << "Entering PlayingState" << std::endl;
    setupTestScene();

    // Auto-save at checkpoint
    m_checkpointSubId = EventBus::getInstance().subscribe(EventType::CheckpointActivated, [this](const GameEvent& ev) {
        int activeSlot = Game::getInstance().getActiveSlot();
        if (!m_entities.empty() && m_entities[0]) {
            if (auto* player = dynamic_cast<Player*>(m_entities[0].get())) {
                bool success = Serializer::saveGame(activeSlot, *player, 1, "Level 1", 300.0f, player->getPosition().x, player->getPosition().y, {true, false, false});
                if (success) {
                    std::cout << "[Auto-Save] Progress saved to Slot " << activeSlot << " at checkpoint!" << std::endl;
                }
            }
        }
    });
}

void PlayingState::exit() {
    std::cout << "Exiting PlayingState" << std::endl;
    cleanupTestScene();

    if (m_checkpointSubId != 0) {
        EventBus::getInstance().unsubscribe(m_checkpointSubId);
        m_checkpointSubId = 0;
    }
}

void PlayingState::handleInput(const sf::Event& event) {
    if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>()) {
        EventBus& bus = EventBus::getInstance();
        
        switch (keyPressed->code) {
            case sf::Keyboard::Key::Backspace:
                Game::getInstance().changeState(std::make_unique<MenuState>());
                break;
            case sf::Keyboard::Key::C: // Collect Coin
                bus.publish({EventType::CoinCollected, 1});
                break;
            case sf::Keyboard::Key::K: // Defeat Enemy
                bus.publish({EventType::EnemyDefeated, 0});
                break;
            case sf::Keyboard::Key::D: // Take Damage
                bus.publish({EventType::PlayerDamaged, 0});
                break;
            case sf::Keyboard::Key::L: // Lose Life / Die
                bus.publish({EventType::PlayerDied, 0});
                break;
            case sf::Keyboard::Key::P: // Checkpoint Activated
                bus.publish({EventType::CheckpointActivated, 0});
                break;
            case sf::Keyboard::Key::U: // Finish Level 3 (unlocks character achievements)
                bus.publish({EventType::LevelComplete, 3});
                break;
            case sf::Keyboard::Key::B: // Boss Defeated
                bus.publish({EventType::BossDefeated, 0});
                break;
            case sf::Keyboard::Key::S: // Star Coin Collected
                bus.publish({EventType::StarCoinCollected, 0});
                break;
            case sf::Keyboard::Key::H: // Hidden Block Broken
                bus.publish({EventType::BlockBroken, 0});
                break;
            default:
                break;
        }
    }
}

void PlayingState::update(float dt) {
    // 1. Update trackers
    StatisticsTracker::getInstance().update(dt);
    AchievementManager::getInstance().update(dt);

    // 2. Update entities
    for (auto& entity : m_entities) {
        if (entity) {
            entity->update(dt);
        }
    }

    // 3. Update physics
    m_physicsEngine.update(m_entities, m_tileMap, dt);
}

void PlayingState::render(sf::RenderTarget& target) {
    // Draw TileMap
    for (int y = 0; y < m_tileMap.getHeight(); ++y) {
        for (int x = 0; x < m_tileMap.getWidth(); ++x) {
            TileType tileType = m_tileMap.getTileType(x, y);
            if (tileType == TileType::Ground) {
                sf::RectangleShape tileShape(sf::Vector2f(Constants::TILE_SIZE, Constants::TILE_SIZE));
                tileShape.setPosition(sf::Vector2f(x * Constants::TILE_SIZE, y * Constants::TILE_SIZE));
                tileShape.setFillColor(sf::Color(120, 80, 40));
                tileShape.setOutlineColor(sf::Color(60, 40, 20));
                tileShape.setOutlineThickness(1.0f);
                target.draw(tileShape);

                sf::RectangleShape grassShape(sf::Vector2f(Constants::TILE_SIZE, 8.0f));
                grassShape.setPosition(sf::Vector2f(x * Constants::TILE_SIZE, y * Constants::TILE_SIZE));
                grassShape.setFillColor(sf::Color(0, 180, 0));
                target.draw(grassShape);
            }
        }
    }

    // Draw Entities
    for (auto& entity : m_entities) {
        if (entity && entity->isActive()) {
            entity->render(target);
        }
    }

    // --- ImGui Dev Interface for Phase 8 ---
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
                    Serializer::saveGame(slot, *player, 1, "Level 1", 300.0f, player->getPosition().x, player->getPosition().y, {true, false, false});
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
                        "- C: Collect coin       - K: Defeat enemy\n"
                        "- D: Take damage        - L: Lose life/Die\n"
                        "- P: Cross Checkpoint   - U: Clear Level 3\n"
                        "- B: Defeat Bowser      - S: Collect star coin\n"
                        "- H: Find hidden block");

    ImGui::End();

    // --- Overlay Toast Notifications ---
    const auto& toasts = AchievementManager::getInstance().getActiveToasts();
    if (!toasts.empty()) {
        ImGui::SetNextWindowPos(ImVec2(1280.0f - 320.0f, 20.0f), ImGuiCond_Always);
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

void PlayingState::setupTestScene() {
    cleanupTestScene();

    m_tileMap.initialize(40, 22);

    for (int x = 0; x < 40; ++x) {
        m_tileMap.setTile(x, 20, TileType::Ground);
    }

    // Spawn Mario character instead of dummy entity
    m_entities.push_back(std::make_unique<Mario>(sf::Vector2f(300.0f, 100.0f)));
}

void PlayingState::cleanupTestScene() {
    m_entities.clear();
}
