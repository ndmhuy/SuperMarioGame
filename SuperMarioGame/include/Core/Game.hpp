#pragma once

#include <SFML/Graphics/RenderWindow.hpp>
#include <memory>
#include <unordered_map>
#include <string>
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

    // Map mouse position to coordinates safely (Encapsulation)
    sf::Vector2f getMouseWorldPosition(const sf::View& view) const;

    // Game State stack wrappers (Encapsulation of GameStateManager)
    void pushState(std::unique_ptr<IGameState> state);
    void popState();
    void changeState(std::unique_ptr<IGameState> state);

    // Game Slot and Settings accessors
    int getActiveSlot() const { return m_activeSlot; }
    void setActiveSlot(int slot) { m_activeSlot = slot; }

    float getSfxVolume() const { return m_sfxVolume; }
    void setSfxVolume(float volume);

    float getMusicVolume() const { return m_musicVolume; }
    void setMusicVolume(float volume);

    const std::string& getDifficulty() const { return m_difficulty; }
    void setDifficulty(const std::string& diff) { m_difficulty = diff; }

    Player* getPlayer() const;
    void setPlayer(Player* player);
    TileMap* getTileMap() const;
    void setTileMap(TileMap* tileMap);

    const std::unordered_map<std::string, std::string>& getKeyBindings() const { return m_keyBindings; }
    std::string getKeyBinding(const std::string& action) const {
        auto it = m_keyBindings.find(action);
        return (it != m_keyBindings.end()) ? it->second : "";
    }
    void setKeyBinding(const std::string& action, const std::string& key) { m_keyBindings[action] = key; }

    bool getColorblindMode() const { return m_colorblindMode; }
    void setColorblindMode(bool enabled) { m_colorblindMode = enabled; }

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

    // Persistent Settings & Slot State
    int m_activeSlot = 1;
    float m_sfxVolume = 80.0f;
    float m_musicVolume = 60.0f;
    std::string m_difficulty = "normal";
    std::unordered_map<std::string, std::string> m_keyBindings;
    bool m_colorblindMode = false;

    Player* m_player = nullptr;
    TileMap* m_tileMap = nullptr;
};
