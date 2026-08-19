#pragma once

#include "Core/IGameState.hpp"
#include "Graphics/UiRenderer.hpp"
#include "Graphics/SpriteSheet.hpp"
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
    enum class Page { Main, Generator };

    // Rows of the generator submenu, in display order.
    enum class GenRow { Theme, Difficulty, PitProbability, PipeFrequency,
                        EnemyRate, CoinRate, GeneratePlay, GenerateEdit, Back, COUNT };

    void moveSelection(int delta);
    void adjustSelection(int direction);
    void activateSelection();
    void applyDifficultyPreset(int index);
    void drawBackground(sf::RenderTarget& target) const;

    Page m_page = Page::Main;
    int m_mainSelected = 0;
    int m_genSelected = 0;
    float m_elapsed = 0.0f;

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

    // Backdrop animation clocks.
    float m_cloudScroll = 0.0f;
    float m_walkerX = -64.0f;
};
