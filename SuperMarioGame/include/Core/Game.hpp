#pragma once

#include <SFML/Graphics/RenderWindow.hpp>
#include <memory>
#include "Core/GameStateManager.hpp"

class Game {
public:
    // Delete copy/move semantics for Singleton
    Game(const Game&) = delete;
    Game& operator=(const Game&) = delete;
    Game(Game&&) = delete;
    Game& operator=(Game&&) = delete;

    // Singleton Instance
    static Game& getInstance();

    // Core Controls
    void run();
    void quit();

    // Map mouse position to coordinates safely (Encapsulation)
    sf::Vector2f getMouseWorldPosition(const sf::View& view) const;

    // Game State stack wrappers (Encapsulation of GameStateManager)
    void pushState(std::unique_ptr<IGameState> state);
    void popState();
    void changeState(std::unique_ptr<IGameState> state);

private:
    Game() = default;
    ~Game() = default;

    // Initialization helpers
    void initWindow();
    void initImGui();
    void shutdown();

    // Window and loop state
    sf::RenderWindow m_window;
    GameStateManager m_gsm;
    bool m_isRunning = false;
};
