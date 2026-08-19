#pragma once

#include <vector>
#include <memory>
#include "Core/IGameState.hpp"

class GameStateManager {
public:
    GameStateManager() = default;
    ~GameStateManager();

    // State Operations.
    //
    // All three are *deferred*: the request is queued and applied at a frame
    // boundary, never in the middle of the caller's own update() or render().
    // A state that pops or replaces itself would otherwise be destroyed while
    // its member function is still executing.
    void pushState(std::unique_ptr<IGameState> state);
    void popState();
    void changeState(std::unique_ptr<IGameState> state);

    // Core loop delegation
    void handleInput(const sf::Event& event);
    void update(float dt);
    void render(sf::RenderTarget& target);

    // Tear the whole stack down immediately. Only for shutdown, where there is
    // no further frame in which a deferred pop could be applied.
    void clearStates();

    // Getters
    IGameState* getCurrentState() const;
    bool isEmpty() const;
    std::size_t getStateCount() const { return m_states.size(); }

private:
    enum class PendingKind { Push, Pop, Change };

    struct PendingOp {
        PendingKind kind;
        std::unique_ptr<IGameState> state;   // null for Pop
    };

    void applyPendingOps();
    void doPush(std::unique_ptr<IGameState> state);
    void doPop();

    // A vector rather than a stack because render() has to walk down through the
    // overlays to the first state that owns the screen.
    std::vector<std::unique_ptr<IGameState>> m_states;
    std::vector<PendingOp> m_pendingOps;
};
