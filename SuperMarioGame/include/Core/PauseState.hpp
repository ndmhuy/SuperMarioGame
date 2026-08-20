#pragma once

#include "Core/IGameState.hpp"
#include "Graphics/UiRenderer.hpp"
#include <functional>
#include <string>
#include <vector>

// Task 7.5 — the pause overlay.
//
// It is an *overlay*: GameStateManager keeps drawing PlayingState beneath it, so
// the frozen frame of the level stays on screen behind the menu. That is only
// possible because render() now walks the stack (see IGameState::isOverlay).
//
// The two destructive choices are supplied as callbacks by whoever pushed this
// state, so PauseState never needs to know what a level is.
class PauseState : public IGameState {
public:
    PauseState(std::function<void()> onRestartLevel,
               std::function<void()> onSaveGame,
               std::function<void()> onQuitToMenu);
    ~PauseState() override = default;

    void enter() override;
    void exit() override;
    void handleInput(const sf::Event& event) override;
    void update(float dt) override;
    void render(sf::RenderTarget& target) override;

    bool isOverlay() const override { return true; }

private:
    void activateSelection();
    void moveSelection(int delta);

    std::function<void()> m_onRestartLevel;
    std::function<void()> m_onSaveGame;
    std::function<void()> m_onQuitToMenu;

    // Feedback line shown after a save, so the choice is not silent.
    std::string m_notice;
    float m_noticeTimer = 0.0f;

    std::vector<UiMenuItem> m_items;
    int m_selected = 0;
    float m_elapsed = 0.0f;

    // Set once a choice has been taken, so a second Enter in the same frame
    // cannot queue two state operations.
    bool m_dismissed = false;
};
