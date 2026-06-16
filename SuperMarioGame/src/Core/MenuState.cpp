#include "Core/MenuState.hpp"
#include "Core/PlayingState.hpp"
#include "Core/Game.hpp"
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
            Game::getInstance().getGSM().changeState(std::make_unique<PlayingState>());
        }
    }
}

void MenuState::update(float dt) {
    // No update logic yet
}

void MenuState::render(sf::RenderTarget& target) {
    ImGui::Begin("Main Menu");
    ImGui::Text("Super Mario Bros (CS202)");
    ImGui::Separator();
    if (ImGui::Button("Start Game (Enter)")) {
        Game::getInstance().getGSM().changeState(std::make_unique<PlayingState>());
    }
    if (ImGui::Button("Quit")) {
        Game::getInstance().quit();
    }
    ImGui::End();
}
