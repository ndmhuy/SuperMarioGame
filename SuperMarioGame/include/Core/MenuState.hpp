#pragma once

#include "Core/IGameState.hpp"
#include "Core/GameMode.hpp"
#include "Graphics/UiRenderer.hpp"
#include "Graphics/SpriteSheet.hpp"
#include "Graphics/BackgroundRenderer.hpp"
#include "Utils/MapGenerator.hpp"
#include <memory>
#include <vector>

// Task 7.1 — the animated main menu.
//
// Rewritten off ImGui. The old screen issued its state changes from inside
// render() (audit X-5: five `changeState` calls in a draw function), because an
// ImGui button *is* a draw call. Everything here is keyboard-driven from
// handleInput(), and render() only draws.
class MenuState : public IGameState {
public:
    MenuState() = default;
    ~MenuState() override = default;

    void enter() override;
    void exit() override;
    void handleInput(const sf::Event& event) override;
    void update(float dt) override;
    void render(sf::RenderTarget& target) override;

private:
    // Which list the keyboard is currently driving.
    enum class Page { Main, Generator, Multiplayer };

    // Rows of the generator submenu, in display order.
    enum class GenRow { Theme, Difficulty, PitProbability, PipeFrequency,
                        EnemyRate, CoinRate, GeneratePlay, GenerateEdit, Back, COUNT };

    // Rows of the multiplayer submenu, in display order.
    //
    // The main menu's "2P VERSUS" row used to drop straight into a hardcoded
    // shared-screen human-vs-human match on 1-1. There are four modes now, and
    // a CPU opponent has two more axes, so the choice needs a page of its own.
    enum class MpRow { Mode, Opponent, Difficulty, Archetype, Start, Back, COUNT };

    void moveSelection(int delta);
    void adjustSelection(int direction);
    void activateSelection();
    void applyDifficultyPreset(int index);
    void drawBackground(sf::RenderTarget& target) const;

    // Rows the current multiplayer selection actually offers. The AI rows are
    // meaningless for a human opponent or a shadow, and a disabled row the
    // cursor can still land on reads as a bug.
    bool isMultiplayerRowEnabled(MpRow row) const;
    std::vector<UiMenuItem> buildMultiplayerItems() const;

    Page m_page = Page::Main;
    int m_mainSelected = 0;
    int m_genSelected = 0;
    int m_mpSelected = 0;
    float m_elapsed = 0.0f;

    // What the multiplayer page is currently configuring. Defaults to the mode
    // that used to be the only one, so the page opens on familiar ground.
    MatchConfig m_match{GameMode::VersusHuman};

    // Guards against a second confirm in the same frame queueing two states.
    bool m_dismissed = false;

    MapGeneratorConfig m_generatorConfig;
    int m_selectedThemeIdx = 0;
    int m_selectedDifficultyIdx = 0;

    std::vector<UiMenuItem> m_mainItems;

    // Atlases for the animated backdrop. Optional: the menu degrades to its
    // painted sky and hills if either is missing.
    std::unique_ptr<SpriteSheet> m_playerSheet;
    std::unique_ptr<SpriteSheet> m_scenerySheet;

    // The same parallax backdrop the levels use, so the menu and the game look
    // like the same product.
    BackgroundRenderer m_background;

    // Backdrop animation clocks.
    float m_cloudScroll = 0.0f;
    float m_walkerX = -64.0f;
};
