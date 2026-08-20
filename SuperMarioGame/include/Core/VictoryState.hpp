#pragma once

#include "Core/IGameState.hpp"
#include "Graphics/UiRenderer.hpp"
#include <array>
#include <functional>
#include <string>

// What the flagpole hand-off produced. `timeBonus` has already been added to
// `finalScore` by PlayingState; the screen only animates the transfer so the
// number the player sees agrees with the number the player has.
struct LevelSummary {
    std::string levelName = "1-1";
    std::string characterName = "mario";
    int scoreBeforeBonus = 0;
    int timeBonus = 0;
    int finalScore = 0;
    int coins = 0;
    int timeRemaining = 0;
    std::array<bool, 3> starCoins = {false, false, false};
    bool isFinalLevel = false;
};

// Task 7.7 — Victory / level-clear screen.
//
// An overlay over the still-rendered level, so the flag and the celebrating
// player stay visible behind the summary.
//
// `onContinue` is supplied by PlayingState and calls its existing
// advanceToNextLevel(); this state deliberately knows nothing about the
// campaign order.
class VictoryState : public IGameState {
public:
    VictoryState(LevelSummary summary, std::function<void()> onContinue);
    ~VictoryState() override = default;

    void enter() override;
    void exit() override;
    void handleInput(const sf::Event& event) override;
    void update(float dt) override;
    void render(sf::RenderTarget& target) override;

    bool isOverlay() const override { return true; }

private:
    void dismiss();

    LevelSummary m_summary;
    std::function<void()> m_onContinue;

    float m_elapsed = 0.0f;
    // Points of the time bonus tallied into the displayed score so far.
    float m_tallied = 0.0f;
    bool m_dismissed = false;
};
