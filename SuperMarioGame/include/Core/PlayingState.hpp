#pragma once

#include "Core/IGameState.hpp"
#include "Physics/PhysicsEngine.hpp"
#include "Utils/TileMap.hpp"
#include "Graphics/Camera.hpp"
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
    Player* m_player = nullptr;
    int m_selectedCharIndex = 0; // 0: Mario, 1: Luigi, 2: Toad, 3: Peach
    Camera m_camera;

    void setupTestScene();
    void cleanupTestScene();
    void spawnSelectedPlayer(const sf::Vector2f& pos);
};
