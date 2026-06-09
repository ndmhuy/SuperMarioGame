#include "Core/PlayingState.hpp"
#include <iostream>

void PlayingState::enter() {
    std::cout << "Entering PlayingState" << std::endl;
}

void PlayingState::exit() {
    std::cout << "Exiting PlayingState" << std::endl;
}

void PlayingState::handleInput(const sf::Event& event) {
    // Stub
}

void PlayingState::update(float dt) {
    // Stub
}

void PlayingState::render(sf::RenderTarget& target) {
    // Stub
}
