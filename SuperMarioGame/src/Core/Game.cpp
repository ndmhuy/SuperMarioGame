#include "Core/Game.hpp"
#include "Utils/Constants.hpp"
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

    m_isRunning = true;

    // TODO: Instantiate initial MenuState and push it to GSM

    while (m_isRunning && m_window.isOpen()) {
        // TODO: Implement fixed timestep game loop, handle inputs, update, and render states.
        
        // Minimal fallback loop to keep window responsive during development
        while (const std::optional<sf::Event> event = m_window.pollEvent()) {
            ImGui::SFML::ProcessEvent(m_window, *event);
            if (event->is<sf::Event::Closed>()) {
                quit();
            }
        }

        m_window.clear(sf::Color(100, 149, 237));
        // TODO: Render current game state and ImGui dev tools
        m_window.display();
    }

    shutdown();
}

void Game::quit() {
    m_isRunning = false;
    m_window.close();
}

sf::RenderWindow& Game::getWindow() {
    return m_window;
}

GameStateManager& Game::getGSM() {
    return m_gsm;
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
    ImGui::SFML::Shutdown();
}
