#pragma once

#include "Core/IGameState.hpp"
#include "Graphics/UiRenderer.hpp"
#include "Utils/Serializer.hpp"
#include <array>
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
    // Which list the keyboard is driving.
    //
    // SAVE GAME used to run the save callback immediately, and that callback is
    // PlayingState's `saveToSlot(Game::getActiveSlot())`. Nothing in the game
    // ever changed the active slot, so it was 1 on every launch: a player with
    // three slots could only ever write one, and every save silently destroyed
    // the previous one with no way to keep it.
    //
    // The picker fixes that from this side of the callback — it calls
    // Game::setActiveSlot() *before* invoking the save action, so the existing
    // `saveToSlot(getActiveSlot())` call needed no change at all and this state
    // still knows nothing about levels or serialization.
    enum class Page { Menu, SlotPicker, OverwriteConfirm };

    void activateSelection();
    void moveSelection(int delta);

    // Page::Menu
    void activateMenuSelection();

    // Page::SlotPicker — opened by SAVE GAME.
    void openSlotPicker();
    void refreshSlotPreviews();
    std::vector<UiMenuItem> buildSlotItems() const;
    void activateSlotSelection();

    // Page::OverwriteConfirm — only reached for a slot that already holds a
    // save. Overwriting one is irreversible for the player, so it is never the
    // consequence of a single keypress.
    void activateConfirmSelection();

    // Writes to `slot` through the callback and returns to Page::Menu with the
    // "SAVED TO SLOT n" notice showing.
    void commitSaveTo(int slot);

    void renderMenuPage(sf::RenderTarget& target) const;
    void renderSlotPickerPage(sf::RenderTarget& target) const;
    void renderConfirmPage(sf::RenderTarget& target) const;

    std::function<void()> m_onRestartLevel;
    std::function<void()> m_onSaveGame;
    std::function<void()> m_onQuitToMenu;

    // Feedback line shown after a save, so the choice is not silent.
    std::string m_notice;
    float m_noticeTimer = 0.0f;

    std::vector<UiMenuItem> m_items;
    int m_selected = 0;
    float m_elapsed = 0.0f;

    Page m_page = Page::Menu;

    // Re-read from disk every time the picker opens, never cached across it: a
    // slot written earlier in this same pause session must show its new
    // contents, or the player is told they would overwrite something stale.
    std::array<SaveSlotPreview, 3> m_slotPreviews;

    // Rows 0..2 are slots 1..3; row 3 is BACK.
    int m_slotSelected = 0;

    // The slot the confirm page is asking about, 1-based. 0 means "nothing
    // pending", which is what the confirm page must never be entered with.
    int m_pendingSlot = 0;

    // Confirm rows: 0 = KEEP (cancel), 1 = OVERWRITE. Defaults to the harmless
    // one, so Enter pressed out of habit does not destroy a save.
    int m_confirmSelected = 0;

    // Set once a choice has been taken, so a second Enter in the same frame
    // cannot queue two state operations.
    bool m_dismissed = false;
};
