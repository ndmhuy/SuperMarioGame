#pragma once

#include <SFML/Window/Event.hpp>
#include <SFML/Graphics/RenderTarget.hpp>

class IGameState {
public:
    virtual ~IGameState() = default;

    virtual void enter() = 0;
    virtual void exit() = 0;
    virtual void handleInput(const sf::Event& event) = 0;
    virtual void update(float dt) = 0;
    virtual void render(sf::RenderTarget& target) = 0;

    // Overlay states do not own the screen: GameStateManager keeps walking down
    // the stack and draws everything beneath them first. This is what makes a
    // pause screen possible at all — before it existed, render() drew only the
    // top of the stack, so pushing anything hid the game entirely.
    virtual bool isOverlay() const { return false; }

    // Called on the state that was on top when something is pushed over it, and
    // again when that cover is removed. Only the top of the stack receives
    // update() and handleInput(), so a suspended state should use these to park
    // anything that keeps running outside its own update — music, ImGui panels.
    virtual void onSuspend() {}
    virtual void onResume() {}
};
