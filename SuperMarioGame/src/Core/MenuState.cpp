#include "Core/MenuState.hpp"
#include "Core/PlayingState.hpp"
#include "Core/Game.hpp"
#include "Utils/Constants.hpp"
#include <SFML/Window/Event.hpp>
#include <imgui.h>
#include <iostream>

void MenuState::enter() {
    std::cout << "Entering MenuState" << std::endl;
}

void MenuState::exit() {
    std::cout << "Exiting MenuState" << std::endl;
}

void MenuState::handleInput(const sf::Event& event) {
    if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>()) {
        if (keyPressed->code == sf::Keyboard::Key::Enter) {
            Game::getInstance().changeState(std::make_unique<PlayingState>(false, false));
        }
    }
}

void MenuState::update(float dt) {
    // Menu animation updates if any
}

void MenuState::render(sf::RenderTarget& target) {
    ImGui::SetNextWindowPos(ImVec2(Constants::WINDOW_WIDTH * 0.5f - 220.0f, Constants::WINDOW_HEIGHT * 0.5f - 200.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(440.0f, 400.0f), ImGuiCond_FirstUseEver);

    ImGui::Begin("Super Mario Engine - Main Menu");
    ImGui::Text("SUPER MARIO BROS - CS202 FINAL PROJECT");
    ImGui::Separator();

    ImGui::Spacing();
    ImGui::Text("Choose Mode to Start:");
    ImGui::Spacing();

    if (ImGui::Button("🎮 Play Game Mode (Level 1-1)", ImVec2(400.0f, 40.0f))) {
        Game::getInstance().changeState(std::make_unique<PlayingState>(false, false));
    }

    if (ImGui::Button("🛠️ In-Game Map Editor (Mario Maker)", ImVec2(400.0f, 40.0f))) {
        Game::getInstance().changeState(std::make_unique<PlayingState>(true, false));
    }

    if (ImGui::Button("🎲 Procedural Level Generator...", ImVec2(400.0f, 40.0f))) {
        m_showGeneratorPanel = !m_showGeneratorPanel;
    }

    if (m_showGeneratorPanel) {
        ImGui::Indent();
        ImGui::Separator();
        ImGui::Text("Procedural Map Generator Settings:");

        const char* themes[] = { "Overworld", "Underground", "Castle", "Ice" };
        if (ImGui::Combo("Theme", &m_selectedThemeIdx, themes, 4)) {
            m_generatorConfig.theme = static_cast<MapTheme>(m_selectedThemeIdx);
        }

        const char* difficulties[] = { "Easy", "Medium", "Hard" };
        if (ImGui::Combo("Difficulty", &m_selectedDifficultyIdx, difficulties, 3)) {
            m_generatorConfig.difficulty = static_cast<MapDifficulty>(m_selectedDifficultyIdx);
            if (m_selectedDifficultyIdx == 0) { // Easy
                m_generatorConfig.pitProbability = 0.05f;
                m_generatorConfig.enemySpawnRate = 0.10f;
            } else if (m_selectedDifficultyIdx == 1) { // Medium
                m_generatorConfig.pitProbability = 0.12f;
                m_generatorConfig.enemySpawnRate = 0.20f;
            } else { // Hard
                m_generatorConfig.pitProbability = 0.22f;
                m_generatorConfig.enemySpawnRate = 0.35f;
            }
        }

        ImGui::SliderFloat("Pit Probability", &m_generatorConfig.pitProbability, 0.0f, 0.4f);
        ImGui::SliderFloat("Pipe Frequency", &m_generatorConfig.pipeFrequency, 0.0f, 0.2f);
        ImGui::SliderFloat("Enemy Spawn Rate", &m_generatorConfig.enemySpawnRate, 0.0f, 0.5f);
        ImGui::SliderFloat("Coin Cluster Rate", &m_generatorConfig.coinClusterRate, 0.0f, 0.5f);

        if (ImGui::Button("🚀 Generate & Play", ImVec2(190.0f, 30.0f))) {
            Game::getInstance().changeState(std::make_unique<PlayingState>(false, true, m_generatorConfig));
        }
        ImGui::SameLine();
        if (ImGui::Button("✏️ Generate & Edit", ImVec2(190.0f, 30.0f))) {
            Game::getInstance().changeState(std::make_unique<PlayingState>(true, true, m_generatorConfig));
        }
        ImGui::Unindent();
    }

    ImGui::Separator();
    if (ImGui::Button("❌ Quit Game", ImVec2(400.0f, 30.0f))) {
        Game::getInstance().quit();
    }

    ImGui::End();
}
