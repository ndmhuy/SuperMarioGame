#include <utility>

#include "Core/GameStateManager.hpp"

GameStateManager::~GameStateManager() {
    clearStates();
}

void GameStateManager::pushState(std::unique_ptr<IGameState> state) {
    if (state) {
        m_pendingOps.push_back({PendingKind::Push, std::move(state)});
    }
}

void GameStateManager::popState() {
    m_pendingOps.push_back({PendingKind::Pop, nullptr});
}

void GameStateManager::changeState(std::unique_ptr<IGameState> state) {
    if (state) {
        m_pendingOps.push_back({PendingKind::Change, std::move(state)});
    }
}

void GameStateManager::doPush(std::unique_ptr<IGameState> state) {
    if (!state) return;

    // The state losing the top slot stops receiving update()/handleInput(), so
    // tell it before the newcomer's enter() runs.
    if (!m_states.empty()) {
        m_states.back()->onSuspend();
    }
    m_states.push_back(std::move(state));
    m_states.back()->enter();
}

void GameStateManager::doPop() {
    if (m_states.empty()) return;

    m_states.back()->exit();
    m_states.pop_back();
    if (!m_states.empty()) {
        m_states.back()->onResume();
    }
}

void GameStateManager::applyPendingOps() {
    // Ops are drained into a local first: enter()/exit() may queue further ops,
    // and those belong to the next boundary, not this loop.
    while (!m_pendingOps.empty()) {
        std::vector<PendingOp> batch;
        batch.swap(m_pendingOps);

        for (auto& op : batch) {
            switch (op.kind) {
                case PendingKind::Push:
                    doPush(std::move(op.state));
                    break;
                case PendingKind::Pop:
                    doPop();
                    break;
                case PendingKind::Change:
                    // Replace the top rather than layering over it. The state
                    // underneath (if any) stays suspended: it was already
                    // suspended when the outgoing state was pushed.
                    if (!m_states.empty()) {
                        m_states.back()->exit();
                        m_states.pop_back();
                    }
                    if (op.state) {
                        m_states.push_back(std::move(op.state));
                        m_states.back()->enter();
                    }
                    break;
            }
        }
    }
}

void GameStateManager::clearStates() {
    m_pendingOps.clear();
    while (!m_states.empty()) {
        m_states.back()->exit();
        m_states.pop_back();
    }
}

void GameStateManager::handleInput(const sf::Event& event) {
    applyPendingOps();
    if (!m_states.empty()) {
        m_states.back()->handleInput(event);
    }
}

void GameStateManager::update(float dt) {
    applyPendingOps();
    if (!m_states.empty()) {
        m_states.back()->update(dt);
    }
    applyPendingOps();
}

void GameStateManager::render(sf::RenderTarget& target) {
    if (m_states.empty()) return;

    // Walk down past every overlay to the first state that owns the screen, then
    // draw back up so the overlays land on top of it.
    std::size_t first = m_states.size() - 1;
    while (first > 0 && m_states[first]->isOverlay()) {
        --first;
    }
    for (std::size_t i = first; i < m_states.size(); ++i) {
        m_states[i]->render(target);
    }
}

IGameState* GameStateManager::getCurrentState() const {
    if (!m_states.empty()) {
        return m_states.back().get();
    }
    return nullptr;
}

bool GameStateManager::isEmpty() const {
    return m_states.empty();
}
