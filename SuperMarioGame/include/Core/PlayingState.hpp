#pragma once

#include "Core/IGameState.hpp"
#include "Physics/PhysicsEngine.hpp"
#include "Utils/TileMap.hpp"
#include "Graphics/Hud.hpp"
#include <vector>
#include <memory>

class Entity;

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

    std::unique_ptr<Hud> m_hud;
    float m_levelTimer = 300.0f;

    void setupTestScene();
    void cleanupTestScene();
};
