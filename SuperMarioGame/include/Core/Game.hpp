#pragma once

#include <SFML/Graphics/RenderWindow.hpp>
#include <memory>
#include "Core/GameStateManager.hpp"

class Player;
class TileMap;

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

    // Game State stack wrappers (Encapsulation of GameStateManager)
    void pushState(std::unique_ptr<IGameState> state);
    void popState();
    void changeState(std::unique_ptr<IGameState> state);

    // Player and TileMap Registry
    Player* getPlayer() const;
    void setPlayer(Player* player);
    TileMap* getTileMap() const;
    void setTileMap(TileMap* tileMap);

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

    Player* m_player = nullptr;
    TileMap* m_tileMap = nullptr;
};
