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

    std::unordered_map<std::string, std::string> getKeyBindings() const { return m_keyBindings; }
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
};
