#include "Core/PlayingState.hpp"
#include "Core/MenuState.hpp"
#include "Core/Game.hpp"
#include <SFML/Window/Event.hpp>
#include <imgui.h>
#include <iostream>

void PlayingState::enter() {
    std::cout << "Entering PlayingState" << std::endl;
}

void PlayingState::exit() {
    std::cout << "Exiting PlayingState" << std::endl;
}

void PlayingState::handleInput(const sf::Event& event) {
    if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>()) {
        if (keyPressed->code == sf::Keyboard::Key::Backspace) {
            Game::getInstance().getGSM().changeState(std::make_unique<MenuState>());
        }
    }
}

void PlayingState::update(float dt) {
    // No update logic yet
}

void PlayingState::render(sf::RenderTarget& target) {
    ImGui::Begin("Gameplay Simulation");
    ImGui::Text("Playing State (Simulated)");
    ImGui::Text("Press Backspace to return to main menu.");
    ImGui::Separator();
    if (ImGui::Button("Back to Menu (Backspace)")) {
        Game::getInstance().getGSM().changeState(std::make_unique<MenuState>());
    }
    ImGui::End();
}
