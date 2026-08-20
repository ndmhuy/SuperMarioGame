#pragma once

#include <SFML/Graphics/RenderWindow.hpp>
#include <memory>
#include <unordered_map>
#include <string>
#include "Core/GameStateManager.hpp"
#include "Core/DifficultyStrategy.hpp"
#include "Core/GameMode.hpp"
#include "Utils/InputScript.hpp"

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

    // Play a recorded input script instead of waiting for a human, for the
    // verification runs AGENTS.md directive 10 asks for. Call before run(). See
    // InputScript.hpp for why this is in-process rather than driven through the
    // OS keyboard. No effect on a normal launch.
    bool loadInputScript(const std::string& path);

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
    // Swaps the live strategy as well as the stored id, so the change applies to
    // the next level load rather than only to config.json.
    void setDifficulty(const std::string& diff);

    // The numbers behind the current difficulty. Never null: the setter falls
    // back to Normal for an unrecognised id.
    const IDifficultyStrategy& difficulty() const;

    // Whether the OS has keyboard focus on our window.
    //
    // sf::Keyboard::isKeyPressed reads *global* key state, not per-window, so
    // held-key polling kept driving the player while the game was in the
    // background or while an ImGui field had the keyboard. Callers that poll
    // must ask this first.
    bool isWindowFocused() const;

    Player* getPlayer() const;
    void setPlayer(Player* player);

    // The second participant in a two-player match, or null in a single-player
    // run. Kept separate from setPlayer() so that every existing single-player
    // path keeps meaning "player one" and nothing has to be re-pointed.
    void setSecondPlayer(Player* player);

    // Whichever registered player is closest to `from` — the one an enemy should
    // chase, a Thwomp should drop on, or a platform should carry.
    //
    // Enemy AI asked getPlayer() for its target, which is Player 1 by
    // definition, so in two-player modes every enemy in the level ignored
    // Player 2 completely: they walked through Goombas untouched while Player 1
    // was hunted. Callers that genuinely mean "player one" — the HUD, the debug
    // console — still use getPlayer().
    //
    // Skips dying players, so a corpse falling through the level does not keep
    // drawing the whole level's aggro while the survivor plays on.
    Player* getNearestPlayer(sf::Vector2f from) const;

    TileMap* getTileMap() const;
    void setTileMap(TileMap* tileMap);

    const std::unordered_map<std::string, std::string>& getKeyBindings() const { return m_keyBindings; }
    std::string getKeyBinding(const std::string& action) const {
        auto it = m_keyBindings.find(action);
        return (it != m_keyBindings.end()) ? it->second : "";
    }
    // Rebinding applies immediately as well as persisting, so the options UI
    // does not require a restart to take effect.
    //
    // `playerIndex` selects the pad. Player 2's bindings were previously
    // unreachable: InputManager has always kept two tables, but only pad 0 was
    // ever loaded, saved or exposed — so a rebind of Player 2's controls could
    // not be expressed at all, let alone survive a restart.
    void setKeyBinding(const std::string& action, const std::string& key, int playerIndex = 0);

    bool getColorblindMode() const { return m_colorblindMode; }
    void setColorblindMode(bool enabled) { m_colorblindMode = enabled; }

    // What match is being played. PlayingState publishes this when it sets a
    // level up, so systems too deep to be handed the mode — the collision
    // resolver deciding whether a stomp is an attack or a co-op boost — can ask.
    // Reset to single-player when the level tears down, so a mode never leaks
    // into the next run.
    const MatchConfig& matchConfig() const { return m_matchConfig; }
    void setMatchConfig(const MatchConfig& config) { m_matchConfig = config; }

private:
    Game() = default;
    ~Game() = default;

    // Initialization helpers
    void initWindow();
    void initImGui();
    void shutdown();

    // Window and loop state
    GameStateManager m_gsm;
    sf::RenderWindow m_window;
    bool m_isRunning = false;

    // Persistent Settings & Slot State
    int m_activeSlot = 1;
    float m_sfxVolume = 80.0f;
    float m_musicVolume = 60.0f;
    std::string m_difficulty = "normal";
    std::unique_ptr<IDifficultyStrategy> m_difficultyStrategy;
    std::unordered_map<std::string, std::string> m_keyBindings;
    std::unordered_map<std::string, std::string> m_keyBindings2;
    bool m_colorblindMode = false;
    MatchConfig m_matchConfig;

    // Scripted input for verification runs. Inactive unless loadInputScript()
    // was called.
    InputScript m_inputScript;
    // Screenshot requested by the script, taken after the frame is presented.
    std::string m_pendingShot;
    // Writes the window to a PNG under saves/shots/. Used only by the script's
    // `shot` command, so a verification run can capture what it saw without an
    // external screen grab stealing the user's focus.
    void saveScreenshot(const std::string& name);

    Player* m_player = nullptr;
    Player* m_secondPlayer = nullptr;
    TileMap* m_tileMap = nullptr;
};
