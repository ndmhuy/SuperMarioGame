#pragma once

#include <stack>
#include <memory>
#include "Core/IGameState.hpp"

class GameStateManager {
public:
    GameStateManager() = default;
    ~GameStateManager();

    // State Operations
    void pushState(std::unique_ptr<IGameState> state);
    void popState();
    void changeState(std::unique_ptr<IGameState> state);
    
    // Core loop delegation
    void handleInput(const sf::Event& event);
    void update(float dt);
    void render(sf::RenderTarget& target);

    // Getters
    IGameState* getCurrentState() const;
    bool isEmpty() const;

private:
    std::stack<std::unique_ptr<IGameState>> m_states;
};
