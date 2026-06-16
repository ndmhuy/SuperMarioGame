#include "Core/Game.hpp"
#include "Core/MenuState.hpp"
#include "Utils/Constants.hpp"
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
