#include "Core/MenuState.hpp"
#include <iostream>

void MenuState::enter() {
    std::cout << "Entering MenuState" << std::endl;
}

void MenuState::exit() {
    std::cout << "Exiting MenuState" << std::endl;
}

void MenuState::handleInput(const sf::Event& event) {
    // Stub
}

void MenuState::update(float dt) {
    // Stub
}

void MenuState::render(sf::RenderTarget& target) {
    // Stub
}
