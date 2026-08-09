#include "Core/GameStateManager.hpp"

GameStateManager::~GameStateManager() {
    while (!m_states.empty()) {
        m_states.top()->exit();
        m_states.pop();
    }
}

void GameStateManager::pushState(std::unique_ptr<IGameState> state) {
    if (state) {
        m_states.push(std::move(state));
        m_states.top()->enter();
    }
}

void GameStateManager::popState() {
    m_pendingState = nullptr;
    if (!m_states.empty()) {
        m_states.top()->exit();
        m_states.pop();
    }
}

void GameStateManager::changeState(std::unique_ptr<IGameState> state) {
    m_pendingState = std::move(state);
}

void GameStateManager::processPendingState() {
    if (m_pendingState) {
        if (!m_states.empty()) {
            m_states.top()->exit();
            m_states.pop();
        }
        pushState(std::move(m_pendingState));
        m_pendingState = nullptr;
    }
}

void GameStateManager::handleInput(const sf::Event& event) {
    processPendingState();
    if (!m_states.empty()) {
        m_states.top()->handleInput(event);
    }
}

void GameStateManager::update(float dt) {
    if (!m_states.empty()) {
        m_states.top()->update(dt);
    }
    processPendingState();
}

void GameStateManager::render(sf::RenderTarget& target) {
    if (!m_states.empty()) {
        m_states.top()->render(target);
    }
}

IGameState* GameStateManager::getCurrentState() const {
    if (!m_states.empty()) {
        return m_states.top().get();
    }
    return nullptr;
}

bool GameStateManager::isEmpty() const {
    return m_states.empty();
}
