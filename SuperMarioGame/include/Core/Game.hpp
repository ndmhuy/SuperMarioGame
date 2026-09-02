#pragma once

#include <SFML/Window/Mouse.hpp>

#include <SFML/Graphics/RenderWindow.hpp>
#include <memory>
#include <optional>
#include <unordered_map>
#include <string>
#include "Core/GameStateManager.hpp"
#include "Core/DifficultyStrategy.hpp"
#include "Core/DebugCheats.hpp"
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

    // Mouse position in WINDOW pixels, or (-1,-1) when there is no window.
    //
    // The editor needs this as well as the world position: whether a click is
    // over the canvas or over a panel is a screen-space question, and asking
    // ImGui instead (io.WantCaptureMouse) reads a frame stale here — the state
    // machine updates before ImGui::SFML::Update does.
    sf::Vector2f getMousePixelPosition() const;

    // Whether `button` is down, from the SCRIPT's pointer when one is driving
    // and from the real mouse otherwise.
    //
    // The level editor is the only mouse-driven screen in the game, so it was
    // the only one a --script verification run could not exercise at all
    // (directive 10). Polling through here rather than sf::Mouse directly is
    // what lets a script place a Goomba; nothing else in the game polls the
    // mouse, so nothing else is affected.
    bool isMouseButtonDown(sf::Mouse::Button button) const;

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

    // Whether the ImGui developer surfaces are drawn at all. Off by default and
    // persisted like every other setting, because six of the ten ImGui windows
    // had no flag, no compile guard and no keybinding: the engine Dev Tools
    // panel was up in every state including the main menu of a release build,
    // and Num1..Num9 fired achievement events in every gameplay frame.
    //
    // Deliberately does NOT gate the Mario Maker level editor (F1) or the debug
    // console (backtick): the editor is a shipped feature and the console owns
    // its own visibility flag.
    bool getDebugMode() const { return m_debugMode; }
    // Arms or disarms the cheats with the flag, so turning debug mode off in
    // Options cannot leave an immortal player behind in the level underneath it.
    void setDebugMode(bool enabled);

    // The Debug > Cheats switches — immortality, slow motion, hidden HUD.
    //
    // Handed out as a reference rather than proxied through nine pairs of
    // delegating accessors on Game (directive 5's "avoid unnecessary, trivial
    // getters/setters"). DebugCheats is the encapsulation here: it owns its own
    // armed gate and taint rule, and its methods are named for the decisions
    // callers make rather than for the bools behind them. The non-const form
    // exists for the two surfaces that legitimately flip switches — the ImGui
    // panel and the console's `god` command; everything else asks the const one.
    DebugCheats& debugCheats() { return m_cheats; }
    const DebugCheats& debugCheats() const { return m_cheats; }

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

    // Window and loop state.
    //
    // The window is held in an optional so shutdown() can *destroy* it, not just
    // close it. Game is a function-local static, so a plain sf::RenderWindow
    // member is destroyed during static destruction at exit() — and by then
    // SFML's own statics, including the mutex guarding the shared GL context,
    // may already be gone. Destroying the window then made
    // sf::GlResource::~GlResource release the last reference to the shared
    // context, whose destructor threw std::system_error("mutex lock failed"),
    // and a throw during static destruction aborts the process.
    //
    // Closing was not enough: close() tears down the OS window and the render
    // context, but the GlResource base still holds its shared_ptr until the
    // object itself is destroyed. Only reset() releases it, and reset() can be
    // called while the rest of SFML is still alive.
    GameStateManager m_gsm;
    std::optional<sf::RenderWindow> m_window;
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
    bool m_debugMode = false;
    // Deliberately NOT persisted alongside the settings above: a cheat is a
    // per-take aid, and a config.json that remembered immortality would make
    // the next ordinary launch a cheated one.
    DebugCheats m_cheats;
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
