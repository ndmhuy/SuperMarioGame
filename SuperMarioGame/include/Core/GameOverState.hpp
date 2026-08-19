#pragma once

#include "Core/IGameState.hpp"
#include "Graphics/UiRenderer.hpp"
#include "Utils/MapGenerator.hpp"
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

    // True when this run made the high-score table, so the screen can say so.
    bool m_madeHighScore = false;
};
