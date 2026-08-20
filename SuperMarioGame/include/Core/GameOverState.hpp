#pragma once

#include "Core/IGameState.hpp"
#include "Core/GameMode.hpp"
#include "Graphics/UiRenderer.hpp"
#include "Utils/MapGenerator.hpp"
#include <string>
#include <vector>

// Everything the game-over screen needs to show a summary and to rebuild the
// run the player just lost. Passed by value from PlayingState, which is about to
// be destroyed — the screen must not hold a pointer back into it.
struct RunSummary {
    int score = 0;
    int coins = 0;
    int starCoins = 0;
    int levelIndex = 0;
    int characterIndex = 0;
    std::string characterName = "mario";
    bool isProcedural = false;
    MapGeneratorConfig generatorConfig;

    // Which match ended, and how. Without these the game-over screen shows the
    // same "GAME OVER" for a solo run, a versus loss and being caught by your
    // own shadow — and the retry it offers has to guess which mode to rebuild.
    MatchConfig match;
    std::string cause;
    // Whether Shadow Mario was what actually killed the player. The mode is not
    // the cause: a Shadow Chase run can just as easily end on a Goomba or in a
    // pit, and the screen said "CAUGHT" for all three.
    bool caughtByShadow = false;
    // The opponent's final score, for the versus verdict. Meaningless unless
    // match.hasSecondPlayer().
    int opponentScore = 0;
};

// Task 7.6 — Game Over.
//
// Replaces the previous behaviour, which faded straight back to the main menu:
// the player was told nothing about the run that just ended and had to walk the
// whole menu again to retry.
class GameOverState : public IGameState {
public:
    explicit GameOverState(RunSummary summary);
    ~GameOverState() override = default;

    void enter() override;
    void exit() override;
    void handleInput(const sf::Event& event) override;
    void update(float dt) override;
    void render(sf::RenderTarget& target) override;

private:
    void activateSelection();

    RunSummary m_summary;
    std::vector<UiMenuItem> m_items;
    int m_selected = 0;
    float m_elapsed = 0.0f;
    bool m_dismissed = false;

    // How long this screen ignores input after appearing. Long enough that a key
    // held through the death cannot dismiss it, short enough not to feel stuck.
    static constexpr float kInputLockout = 0.75f;

    // True when this run made the high-score table, so the screen can say so.
    bool m_madeHighScore = false;
};
