#pragma once

#include "Core/IGameState.hpp"
#include "Core/GameMode.hpp"
#include "Graphics/UiRenderer.hpp"
#include "Graphics/SpriteSheet.hpp"
#include "Graphics/BackgroundRenderer.hpp"
#include "Utils/MapGenerator.hpp"
#include "Utils/Serializer.hpp"
#include <array>
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
    enum class Page { Main, Generator, Multiplayer, Load };

    // Rows of the generator submenu, in display order.
    enum class GenRow { Theme, Difficulty, PitProbability, PipeFrequency,
                        EnemyRate, CoinRate, GeneratePlay, GenerateEdit, PlayEndless, Back, COUNT };

    // Rows of the multiplayer submenu, in display order.
    //
    // The main menu's "2P VERSUS" row used to drop straight into a hardcoded
    // shared-screen human-vs-human match on 1-1. There are four modes now, and
    // a CPU opponent has two more axes, so the choice needs a page of its own.
    enum class MpRow { Mode, Opponent, Difficulty, Archetype, Start, Back, COUNT };

    // Rows of the Load Game submenu: one per save slot, plus a way back out.
    enum class LoadRow { Slot1, Slot2, Slot3, Back, COUNT };

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

    // Re-reads all three save slots from disk. Called whenever the Load page is
    // opened, not cached across the menu's lifetime — a save made from the
    // pause menu since this MenuState last showed the picker must appear.
    void refreshSlotPreviews();
    std::vector<UiMenuItem> buildLoadItems() const;

    Page m_page = Page::Main;
    int m_mainSelected = 0;
    int m_genSelected = 0;
    int m_mpSelected = 0;
    int m_loadSelected = 0;
    float m_elapsed = 0.0f;

    // F5 attract mode (SPEC 10.2). Seconds since the last key this menu
    // actually handled — reset in handleInput() regardless of page or key, so
    // browsing a submenu counts as activity too. Only checked against the idle
    // threshold while sitting on Page::Main with the debug console closed (see
    // update()): a submenu page freezes this by virtue of MenuState::update()
    // itself not running while an overlay (Options/Records) sits on top, and
    // the console-visible case is checked explicitly since the console does
    // not suspend the state underneath it.
    float m_idleTime = 0.0f;

    // Refreshed by refreshSlotPreviews() every time the Load page opens.
    std::array<SaveSlotPreview, 3> m_slotPreviews;

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
