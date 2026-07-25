#pragma once

#include "Core/IGameState.hpp"
#include "Core/EventBus.hpp"
#include "Physics/PhysicsEngine.hpp"
#include "Utils/TileMap.hpp"
#include "Utils/MapEditor.hpp"
#include "Graphics/Camera.hpp"
#include "Utils/LevelLoader.hpp"
#include <vector>
#include <memory>

class Entity;
class Player;

class PlayingState : public IGameState {
public:
    PlayingState();
    ~PlayingState() override;

    void enter() override;
    void exit() override;
    void handleInput(const sf::Event& event) override;
    void update(float dt) override;
    void render(sf::RenderTarget& target) override;

private:
    PhysicsEngine m_physicsEngine;
    TileMap m_tileMap;
    std::vector<std::unique_ptr<Entity>> m_entities;
    MapEditor m_mapEditor;
    EventBus::SubscriptionId m_checkpointSubId = static_cast<EventBus::SubscriptionId>(-1);
    Player* m_player = nullptr;
    int m_selectedCharIndex = 0; // 0: Mario, 1: Luigi, 2: Toad, 3: Peach
    int m_selectedLevelIndex = 0; // 0: Level 1, 1: Level 2, 2: Level 3, 3: Bonus 1
    Camera m_camera;

    std::unique_ptr<Hud> m_hud;
    float m_levelTimer = 300.0f;

    void setupTestScene();
    void cleanupTestScene();
    void spawnSelectedPlayer(const sf::Vector2f& pos);
};
