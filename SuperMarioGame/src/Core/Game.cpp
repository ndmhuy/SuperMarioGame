#include "Core/Game.hpp"
#include "Core/MenuState.hpp"
#include "Core/SoundManager.hpp"
#include "Core/ResourceManager.hpp"
#include "Core/StatisticsTracker.hpp"
#include "Core/AchievementManager.hpp"
#include "Utils/Constants.hpp"
#include "Utils/Serializer.hpp"
#include <SFML/System/Clock.hpp>
#include <SFML/Window/Event.hpp>
#include <imgui.h>
#include <imgui-SFML.h>
#include <iostream>

Game& Game::getInstance() {
    static Game instance;
    return instance;
}

void Game::run() {
    initWindow();
    initImGui();

    // Load settings from config
    Serializer::loadSettings(m_sfxVolume, m_musicVolume, m_difficulty, m_keyBindings, m_colorblindMode);
    
    // Apply loaded volumes to SoundManager
    SoundManager::getInstance().setSFXVolume(m_sfxVolume);
    SoundManager::getInstance().setMusicVolume(m_musicVolume);

    // Initialize tracking systems
    StatisticsTracker::getInstance().init();
    AchievementManager::getInstance().init();

    // Ensure HUD font is loaded in ResourceManager before any state (and its Hud) is constructed
    ResourceManager& rm = ResourceManager::getInstance();
    std::vector<std::string> fontCandidates = {
        "assets/font/PressStart2P.ttf",
        "SuperMarioGame/assets/font/PressStart2P.ttf",
        "asset/font/PressStart2P.ttf",
        "assets/fonts/PressStart2P.ttf",
        "SuperMarioGame/asset/font/PressStart2P.ttf",
        "SuperMarioGame/assets/fonts/PressStart2P.ttf",
        "../asset/font/PressStart2P.ttf",
        "../assets/fonts/PressStart2P.ttf",
        "/System/Library/Fonts/Supplemental/Arial.ttf",
        "/System/Library/Fonts/Helvetica.ttc",
        "/Library/Fonts/Arial.ttf",
        "C:/Windows/Fonts/consola.ttf",
        "C:/Windows/Fonts/arial.ttf"
    };
    for (const auto& path : fontCandidates) {
        if (std::filesystem::exists(path)) {
            if (rm.loadFont("PressStart2P", path)) break;
        }
    }

    // Load Player texture into ResourceManager
    std::vector<std::string> playerTextureCandidates = {
        "assets/spriteSheet/player/player.png",
        "SuperMarioGame/assets/spriteSheet/player/player.png",
        "../assets/spriteSheet/player/player.png"
    };
    for (const auto& path : playerTextureCandidates) {
        if (std::filesystem::exists(path)) {
            if (rm.loadTexture("player", path)) break;
        }
    }

    // Load Tileset texture into ResourceManager
    std::vector<std::string> tilesetCandidates = {
        "assets/spriteSheet/tileset/tileset_blocks.png",
        "SuperMarioGame/assets/spriteSheet/tileset/tileset_blocks.png",
        "../assets/spriteSheet/tileset/tileset_blocks.png"
    };
    for (const auto& path : tilesetCandidates) {
        if (std::filesystem::exists(path)) {
            if (rm.loadTexture("tileset_blocks", path)) break;
        }
    }

    // Push initial menu state
    m_gsm.pushState(std::make_unique<MenuState>());

    sf::Clock clock;
    float lag = 0.0f;
    const float timeStep = Constants::FIXED_TIMESTEP;

    m_isRunning = true;

    while (m_isRunning && m_window.isOpen()) {
        sf::Time elapsed = clock.restart();
        lag += elapsed.asSeconds();

        // 1. Handle Events (SFML 3.0 style)
        while (const std::optional<sf::Event> event = m_window.pollEvent()) {
            ImGui::SFML::ProcessEvent(m_window, *event);
            m_gsm.handleInput(*event);

            if (event->is<sf::Event::Closed>()) {
                quit();
            }
            else if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
                if (keyPressed->code == sf::Keyboard::Key::Escape) {
                    quit();
                }
            }
        }

        // 2. Fixed Timestep Update
        while (lag >= timeStep) {
            m_gsm.update(timeStep);
            lag -= timeStep;
        }

        // 3. Update ImGui
        ImGui::SFML::Update(m_window, elapsed);

        // ImGui Dev Tools panel
        ImGui::Begin("Super Mario Engine Dev Tools");
        ImGui::Text("Application Average: %.3f ms/frame (%.1f FPS)", 
                    1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::End();

        // 4. Render
        m_window.clear(sf::Color(100, 149, 237)); // Cornflower Blue
        
        m_gsm.render(m_window);
        
        ImGui::SFML::Render(m_window);
        m_window.display();
    }

    shutdown();
}

void Game::quit() {
    m_isRunning = false;
}

sf::Vector2f Game::getMouseWorldPosition(const sf::View& view) const {
    return m_window.mapPixelToCoords(sf::Mouse::getPosition(m_window), view);
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
    m_window.create(sf::VideoMode({Constants::WINDOW_WIDTH, Constants::WINDOW_HEIGHT}), Constants::WINDOW_TITLE);
    m_window.setFramerateLimit(60);
}

void Game::initImGui() {
    if (!ImGui::SFML::Init(m_window)) {
        std::cerr << "Failed to initialize ImGui-SFML!" << std::endl;
    }
}

void Game::shutdown() {
    // Save configuration settings
    Serializer::saveSettings(m_sfxVolume, m_musicVolume, m_difficulty, m_keyBindings, m_colorblindMode);

    // Pop all remaining game states before shutting down window and managers
    while (!m_gsm.isEmpty()) {
        m_gsm.popState();
    }

    m_player = nullptr;
    m_tileMap = nullptr;

    // Shutdown managers
    StatisticsTracker::getInstance().shutdown();
    AchievementManager::getInstance().shutdown();
    SoundManager::getInstance().shutdown();
    ImGui::SFML::Shutdown();

    // Explicitly close window before static destructors run
    if (m_window.isOpen()) {
        m_window.close();
    }
}

Player* Game::getPlayer() const {
    return m_player;
}

void Game::setPlayer(Player* player) {
    m_player = player;
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
