#include "Core/Game.hpp"
#include "Core/MenuState.hpp"
#include "Core/SoundManager.hpp"
#include "Core/InputManager.hpp"
#include "Core/ResourceManager.hpp"
#include "Core/StatisticsTracker.hpp"
#include "Core/AchievementManager.hpp"
#include "Core/DebugConsole.hpp"
#include "Entities/Player.hpp"
#include "Graphics/UiRenderer.hpp"
#include "Utils/Constants.hpp"
#include "Utils/Serializer.hpp"
#include <SFML/System/Clock.hpp>
#include <SFML/Window/Event.hpp>
#include <imgui.h>
#include <imgui-SFML.h>
#include <filesystem>
#include <iostream>

Game& Game::getInstance() {
    static Game instance;
    return instance;
}

void Game::run() {
    initWindow();
    initImGui();

    // Load settings from config
    Serializer::loadSettings(m_sfxVolume, m_musicVolume, m_difficulty, m_keyBindings,
                             m_keyBindings2, m_colorblindMode);
    
    // Build the difficulty strategy from what was just loaded. Without this the
    // persisted setting stayed a string nothing consumed (task 9.4).
    setDifficulty(m_difficulty);

    // Apply loaded volumes to SoundManager
    SoundManager::getInstance().setSFXVolume(m_sfxVolume);
    SoundManager::getInstance().setMusicVolume(m_musicVolume);

    // Apply the persisted key bindings. Serializer has always read and written
    // this map and Game has always held it, but nothing pushed it into the
    // InputManager, so custom bindings were silently ignored and the player kept
    // the hardcoded defaults (audit B-11, GitHub issue #9).
    InputManager::getInstance().applyBindings(m_keyBindings, 0);
    // Player 2's pad, for the same reason. Empty on a fresh install, in which
    // case InputManager keeps its built-in arrow-key layout.
    InputManager::getInstance().applyBindings(m_keyBindings2, 1);

    // Initialize tracking systems
    StatisticsTracker::getInstance().init();
    AchievementManager::getInstance().init();
    // Then restore what the player earned in previous sessions. init() only
    // builds the achievement list and subscribes; every unlocked flag starts
    // false, and until now nothing ever put them back — the data was being
    // written into save slots and read by nothing, so achievements, lifetime
    // statistics and the Toad/Peach unlocks they gate all reset on every launch.
    Serializer::loadProfile();
    // The console works through the public singletons, so it is available from
    // every state rather than only from PlayingState (task 10.4).
    DebugConsole::getInstance().init();

    // Preload the shared resources every state expects to find already loaded.
    // These used to be three hand-written candidate-path lists (audit A-13);
    // ResourceManager::resolvePath is the one place that knows where assets
    // live, and loadFont/loadTexture already call it.
    ResourceManager& rm = ResourceManager::getInstance();
    if (!rm.loadFont("PressStart2P", "assets/font/PressStart2P.ttf")) {
        // A system font keeps the UI legible rather than invisible when the
        // bundled one is missing.
        for (const char* systemFont : {"/System/Library/Fonts/Supplemental/Arial.ttf",
                                       "/System/Library/Fonts/Helvetica.ttc",
                                       "C:/Windows/Fonts/arial.ttf"}) {
            if (rm.loadFont("PressStart2P", systemFont)) break;
        }
    }
    rm.loadTexture("player", "assets/spriteSheet/player/player.png");
    // No tileset_blocks preload: that atlas moved to "[Deprecated] tileset" and
    // tile sprites come from world_scenery_item. The old candidate loop guarded
    // every path with exists(), so it silently found nothing and nobody noticed
    // the load had been dead for a long time.

    // Push initial menu state
    m_gsm.pushState(std::make_unique<MenuState>());

    sf::Clock clock;
    float lag = 0.0f;
    const float timeStep = Constants::FIXED_TIMESTEP;

    m_isRunning = true;

    while (m_isRunning && m_window && m_window->isOpen()) {
        sf::Time elapsed = clock.restart();
        lag += elapsed.asSeconds();

        // 1. Handle Events (SFML 3.0 style)
        while (const std::optional<sf::Event> event = m_window->pollEvent()) {
            ImGui::SFML::ProcessEvent(*m_window, *event);

            // Held-key state is tracked from events rather than polled from the
            // OS. Recorded here, for every event, so presses and releases stay
            // balanced even when a state chooses to ignore the key — an
            // unrecorded release is a key held down forever.
            InputManager::getInstance().noteKeyEvent(*event);

            if (event->is<sf::Event::Closed>()) {
                quit();
                continue;
            }


            // Backquote toggles the debug console, from any state. Handled here
            // rather than in a state so nothing on top can shadow it, and
            // *before* the states see the event so the key is genuinely
            // consumed (task 10.4).
            if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
                if (keyPressed->code == sf::Keyboard::Key::Grave) {
                    DebugConsole::getInstance().toggle();
                    continue;
                }
            }

            // While the console is open the keyboard belongs to it. Without
            // this, typing "give star" would also walk, jump and fire.
            if (DebugConsole::getInstance().isVisible()) {
                continue;
            }

            m_gsm.handleInput(*event);

            // Escape used to quit the process from anywhere. It is now the
            // pause key, so quitting is a decision the states own: the pause
            // menu and the main menu both offer it explicitly.
        }

        // 1b. Scripted input, for verification runs. Fed through the same three
        // steps a real event goes through directly above — held-key tracking,
        // then the state stack — so a script exercises the actual input path
        // rather than a parallel one that could agree with it and still be wrong.
        if (m_inputScript.active()) {
            std::vector<sf::Event> scripted;
            std::string shotName;
            const InputScript::Effect effect =
                m_inputScript.update(elapsed.asSeconds(), scripted, shotName);

            for (const sf::Event& event : scripted) {
                InputManager::getInstance().noteKeyEvent(event);
                if (DebugConsole::getInstance().isVisible()) continue;
                m_gsm.handleInput(event);
            }

            if (effect == InputScript::Effect::Screenshot) m_pendingShot = shotName;
            if (effect == InputScript::Effect::Quit) quit();
        }

        // 2. Fixed Timestep Update
        while (lag >= timeStep) {
            m_gsm.update(timeStep);
            // Toasts fade on their own clock. PlayingState used to be the only
            // thing advancing it, so a toast raised as a level ended froze on
            // screen once the state changed.
            AchievementManager::getInstance().update(timeStep);
            lag -= timeStep;
        }

        // 3. Update ImGui
        ImGui::SFML::Update(*m_window, elapsed);

        // ImGui Dev Tools panel
        // Bottom-right, below the map editor window (912,8 - 360x600) and clear
        // of the AI overlay. At its old spot it landed on top of both.
        ImGui::SetNextWindowPos(ImVec2(912.0f, 616.0f), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(360.0f, 96.0f), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowCollapsed(true, ImGuiCond_FirstUseEver);
        ImGui::Begin("Super Mario Engine Dev Tools");
        ImGui::Text("Application Average: %.3f ms/frame (%.1f FPS)", 
                    1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::Text("Press ` to toggle the debug console.");
        ImGui::End();

        DebugConsole::getInstance().draw();

        // 4. Render
        m_window->clear(sf::Color(100, 149, 237)); // Cornflower Blue
        
        m_gsm.render(*m_window);

        // Above every state, below ImGui. An achievement can unlock during play,
        // on the victory tally or on the world map, and the popup should show in
        // all three.
        m_window->setView(m_window->getDefaultView());
        UiRenderer::drawAchievementToasts(*m_window);
        
        ImGui::SFML::Render(*m_window);
        m_window->display();

        // After display(), so the capture is the frame the user would see —
        // including the ImGui overlays, which are drawn last.
        if (!m_pendingShot.empty()) {
            saveScreenshot(m_pendingShot);
            m_pendingShot.clear();
        }
    }

    shutdown();
}

void Game::quit() {
    m_isRunning = false;
}

bool Game::loadInputScript(const std::string& path) {
    return m_inputScript.load(path);
}

void Game::saveScreenshot(const std::string& name) {
    if (!m_window) return;

    sf::Texture texture;
    if (!texture.resize(m_window->getSize())) {
        std::cerr << "[Game] Could not allocate a texture for the screenshot." << std::endl;
        return;
    }
    texture.update(*m_window);

    std::error_code ignored;
    // Same reasoning as MapEditor's export: resolved through Serializer so a
    // screenshot lands beside the save files rather than wherever the process
    // happened to be started from.
    const std::string dir = Serializer::saveDirectory() + "/shots";
    std::filesystem::create_directories(dir, ignored);
    const std::string path = dir + "/" + name + ".png";
    if (texture.copyToImage().saveToFile(path)) {
        std::cout << "[Game] Screenshot: " << path << std::endl;
    } else {
        std::cerr << "[Game] Failed to write " << path << std::endl;
    }
}

sf::Vector2f Game::getMouseWorldPosition(const sf::View& view) const {
    // mapPixelToCoords divides by the viewport size, so it asserts on a window
    // that has not been created yet. Head-less harnesses drive the map editor
    // through exactly this path — and now that the window is an optional, they
    // reach it before there is one to dereference at all.
    if (!m_window) return {0.0f, 0.0f};
    const sf::Vector2u windowSize = m_window->getSize();
    if (windowSize.x == 0 || windowSize.y == 0) return {0.0f, 0.0f};
    return m_window->mapPixelToCoords(sf::Mouse::getPosition(*m_window), view);
}

void Game::pushState(std::unique_ptr<IGameState> state) {
    m_gsm.pushState(std::move(state));
}

void Game::popState() {
    m_gsm.popState();
}

void Game::changeState(std::unique_ptr<IGameState> state) {
    m_gsm.changeState(std::move(state));
}

void Game::initWindow() {
    // Fixed size, deliberately.
    //
    // Every screen is laid out against literal 1280x720 coordinates — the HUD,
    // all six menus, the pause overlay, the boss health bar. The window was
    // created resizable (the default style) with no Resized handler, so dragging
    // it stretched the view to whatever shape the window was and pushed the
    // right-hand HUD off the edge.
    //
    // Letterboxing would be the better answer, but it cannot work while every
    // state resets to target.getDefaultView() for screen space: that view is the
    // untouched full window and would undo it. Making the layout
    // resolution-independent is the real fix and a much larger change; until
    // then, not distorting beats distorting.
    m_window.emplace(sf::VideoMode({Constants::WINDOW_WIDTH, Constants::WINDOW_HEIGHT}),
                    Constants::WINDOW_TITLE,
                    sf::Style::Titlebar | sf::Style::Close);
    m_window->setFramerateLimit(60);
}

void Game::initImGui() {
    if (!ImGui::SFML::Init(*m_window)) {
        std::cerr << "Failed to initialize ImGui-SFML!" << std::endl;
    }

    // The saved layout wins over every ImGuiCond_FirstUseEver in the code, so a
    // stale imgui.ini pins windows wherever they last happened to be. On the
    // machine this was reported from, that file held Pos=60,60 for all six
    // panels, which is why they stacked on top of one another no matter what the
    // defaults said.
    //
    // Versioning the filename is the standard answer: the positions below take
    // effect, users can still drag windows and have that remembered, and bumping
    // the version when the default layout changes retires the old file instead
    // of fighting it.
    static const char* const kIniFile = "imgui_layout_v2.ini";
    ImGui::GetIO().IniFilename = kIniFile;

    // Retire the unversioned file so it stops being written and stops shadowing
    // the defaults. Best-effort: failing to remove it is cosmetic.
    std::error_code ignored;
    std::filesystem::remove("imgui.ini", ignored);
}

void Game::shutdown() {
    // Save configuration settings
    Serializer::saveSettings(m_sfxVolume, m_musicVolume, m_difficulty, m_keyBindings,
                             m_keyBindings2, m_colorblindMode);
    // And the profile: achievements and lifetime statistics. Written here as well
    // as on each unlock, so a session that earns nothing still persists its
    // playtime and counters.
    Serializer::saveProfile();

    // Tear the stack down immediately — pushState/popState are deferred to a
    // frame boundary, and there are no more frames.
    m_gsm.clearStates();

    m_player = nullptr;
    m_tileMap = nullptr;

    // Shutdown managers
    StatisticsTracker::getInstance().shutdown();
    AchievementManager::getInstance().shutdown();
    SoundManager::getInstance().shutdown();

    // Release every SFML resource the ResourceManager is holding *before* the
    // window (and with it the graphics context) goes away. Without this the
    // singleton's sf::Texture and sf::Font objects were destroyed during static
    // destruction, after the context — which aborted the process on exit with
    // "mutex lock failed". The game printed its whole shutdown and then died
    // with SIGABRT, so nothing in the log ever showed it.
    ResourceManager::getInstance().clear();

    ImGui::SFML::Shutdown();

    // DESTROY the window here, not merely close it.
    //
    // Closing was the previous attempt at this, and it was necessary but not
    // sufficient: close() tears down the OS window and the render context, but
    // sf::Window's GlResource base holds a shared_ptr to SFML's shared GL
    // context until the sf::RenderWindow *object* is destroyed. As a plain
    // member of a function-local static that happened during static destruction
    // at exit(), by which point SFML's own statics were gone — so releasing the
    // last reference ran SFContext::~SFContext(), which threw
    // std::system_error("mutex lock failed: Invalid argument"). A throw during
    // static destruction aborts, so every clean exit ended in SIGABRT with the
    // whole shutdown already printed.
    //
    // reset() runs it here instead, while the rest of SFML is still alive.
    if (m_window) {
        if (m_window->isOpen()) m_window->close();
        m_window.reset();
    }
}

bool Game::isWindowFocused() const {
    // Answers false once the window is gone, rather than dereferencing an empty
    // optional. Nothing should poll after shutdown, but "nothing should" is what
    // the dangling-pointer bugs in this file all had in common.
    return m_window && m_window->hasFocus();
}

Player* Game::getPlayer() const {
    return m_player;
}

void Game::setPlayer(Player* player) {
    m_player = player;
}

void Game::setSecondPlayer(Player* player) {
    m_secondPlayer = player;
}

Player* Game::getNearestPlayer(sf::Vector2f from) const {
    // A dying player is falling through the level on its way off the bottom of
    // the screen. Chasing it would drag every enemy off the map behind it.
    auto eligible = [](Player* p) { return p && p->isActive() && !p->isDying(); };

    Player* first  = eligible(m_player)       ? m_player       : nullptr;
    Player* second = eligible(m_secondPlayer) ? m_secondPlayer : nullptr;

    if (!second) return first;
    if (!first)  return second;

    const sf::Vector2f d1 = first->getPosition() - from;
    const sf::Vector2f d2 = second->getPosition() - from;
    // Squared distance: nothing here needs the actual metres, and this runs once
    // per AI-driven entity per frame.
    const float sq1 = d1.x * d1.x + d1.y * d1.y;
    const float sq2 = d2.x * d2.x + d2.y * d2.y;
    return (sq2 < sq1) ? second : first;
}

TileMap* Game::getTileMap() const {
    return m_tileMap;
}

void Game::setTileMap(TileMap* tileMap) {
    m_tileMap = tileMap;
}

void Game::setSfxVolume(float volume) {
    m_sfxVolume = volume;
    SoundManager::getInstance().setSFXVolume(volume);
}

void Game::setMusicVolume(float volume) {
    m_musicVolume = volume;
    SoundManager::getInstance().setMusicVolume(volume);
}

void Game::setDifficulty(const std::string& diff) {
    m_difficulty = diff;
    m_difficultyStrategy = IDifficultyStrategy::fromId(diff);
}

const IDifficultyStrategy& Game::difficulty() const {
    // Lazily built so a Game that never had setDifficulty called — the tests, and
    // the first frame before settings load — still gets Normal rather than a
    // null dereference.
    if (!m_difficultyStrategy) {
        const_cast<Game*>(this)->m_difficultyStrategy = IDifficultyStrategy::fromId(m_difficulty);
    }
    return *m_difficultyStrategy;
}

void Game::setKeyBinding(const std::string& action, const std::string& key, int playerIndex) {
    if (playerIndex < 0 || playerIndex > 1) return;
    auto& table = (playerIndex == 1) ? m_keyBindings2 : m_keyBindings;
    table[action] = key;
    // Push straight through so the change is live this frame; shutdown() persists
    // both maps to config.json.
    InputManager::getInstance().applyBindings({{action, key}}, playerIndex);
}
