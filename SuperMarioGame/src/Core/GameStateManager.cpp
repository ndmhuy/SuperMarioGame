#include "Core/GameStateManager.hpp"

GameStateManager::~GameStateManager() {
    // TODO: Implement by hand
}

void GameStateManager::pushState(std::unique_ptr<IGameState> state) {
    // TODO: Implement by hand
}

void GameStateManager::popState() {
    // TODO: Implement by hand
}

void GameStateManager::changeState(std::unique_ptr<IGameState> state) {
    // TODO: Implement by hand
}

void GameStateManager::handleInput(const sf::Event& event) {
    // TODO: Implement by hand
}

void GameStateManager::update(float dt) {
    // TODO: Implement by hand
}

void GameStateManager::render(sf::RenderTarget& target) {
    // TODO: Implement by hand
}

IGameState* GameStateManager::getCurrentState() const {
    // TODO: Implement by hand
    return nullptr;
}

bool GameStateManager::isEmpty() const {
    // TODO: Implement by hand
    return true;
}
