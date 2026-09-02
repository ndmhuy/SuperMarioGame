#include "Core/PauseState.hpp"
#include "Core/Game.hpp"
#include "Core/OptionsState.hpp"
#include "Core/EventBus.hpp"
#include "Core/SoundManager.hpp"
#include "Utils/Constants.hpp"

#include <SFML/Window/Event.hpp>
#include <iostream>

namespace {
constexpr float PANEL_W = 460.0f;
constexpr float PANEL_H = 360.0f;

// The picker and the confirm page carry a slot summary each ("MARIO  L2
// 4200PT  STARS 1/3  3:07"), which is 31 characters at its shortest. They get
// their own, wider frame rather than shrinking the text into the 460px pause
// panel — the point of showing the summary is that the player can read what
// they are about to overwrite.
constexpr float WIDE_PANEL_W = 620.0f;
constexpr float PICKER_PANEL_H = 300.0f;
constexpr float CONFIRM_PANEL_H = 250.0f;

// Rows 0..2 select slots 1..3; row 3 backs out without saving.
constexpr int SLOT_COUNT = 3;
constexpr int SLOT_BACK_ROW = SLOT_COUNT;
constexpr int SLOT_ROW_COUNT = SLOT_COUNT + 1;

constexpr int CONFIRM_ROW_KEEP = 0;
constexpr int CONFIRM_ROW_OVERWRITE = 1;
constexpr int CONFIRM_ROW_COUNT = 2;
}

PauseState::PauseState(std::function<void()> onRestartLevel,
                       std::function<void()> onSaveGame,
                       std::function<void()> onQuitToMenu)
    : m_onRestartLevel(std::move(onRestartLevel)),
      m_onSaveGame(std::move(onSaveGame)),
      m_onQuitToMenu(std::move(onQuitToMenu)) {
    m_items.emplace_back("RESUME");
    m_items.emplace_back("SAVE GAME");
    m_items.emplace_back("OPTIONS");
    m_items.emplace_back("RESTART LEVEL");
    m_items.emplace_back("QUIT TO MENU");
}

void PauseState::enter() {
    std::cout << "Entering PauseState" << std::endl;
    // Subscribers (HUD, sound) learn about the pause from the bus rather than
    // from a direct call, same as every other cross-system notification.
    EventBus::getInstance().publish({EventType::PauseToggled, 1});
    SoundManager::getInstance().playSound("pause");
}

void PauseState::exit() {
    std::cout << "Exiting PauseState" << std::endl;
    EventBus::getInstance().publish({EventType::PauseToggled, 0});
}

void PauseState::moveSelection(int delta) {
    if (m_page == Page::SlotPicker) {
        m_slotSelected = (m_slotSelected + delta + SLOT_ROW_COUNT) % SLOT_ROW_COUNT;
        return;
    }
    if (m_page == Page::OverwriteConfirm) {
        m_confirmSelected = (m_confirmSelected + delta + CONFIRM_ROW_COUNT) % CONFIRM_ROW_COUNT;
        return;
    }

    const int n = static_cast<int>(m_items.size());
    if (n == 0) return;
    m_selected = (m_selected + delta % n + n) % n;
}

void PauseState::refreshSlotPreviews() {
    for (int slot = 1; slot <= SLOT_COUNT; ++slot) {
        m_slotPreviews[static_cast<std::size_t>(slot - 1)] = Serializer::getSlotPreview(slot);
    }
}

void PauseState::openSlotPicker() {
    refreshSlotPreviews();
    m_page = Page::SlotPicker;
    // Open on the slot the session is already using, so the common case —
    // saving over your own run on purpose — is one Enter away and the player
    // can still see the other two.
    const int active = Game::getInstance().getActiveSlot();
    m_slotSelected = (active >= 1 && active <= SLOT_COUNT) ? active - 1 : 0;
    m_notice.clear();
    m_noticeTimer = 0.0f;
}

std::vector<UiMenuItem> PauseState::buildSlotItems() const {
    std::vector<UiMenuItem> rows;
    for (int i = 0; i < SLOT_COUNT; ++i) {
        // Same summary string the LOAD GAME page shows, from the same
        // SaveSlotPreview::summary() — see the comment on it. Every row stays
        // selectable, including an empty one: an empty slot is the *safest*
        // thing to save into, so hiding it would be backwards.
        rows.emplace_back("SLOT " + std::to_string(i + 1),
                          m_slotPreviews[static_cast<std::size_t>(i)].summary());
    }
    rows.emplace_back("BACK");
    return rows;
}

void PauseState::commitSaveTo(int slot) {
    if (!m_onSaveGame) return;

    // The callback is PlayingState's saveToSlot(Game::getActiveSlot()), so the
    // slot is communicated by setting it here first. Doing it this way is what
    // keeps the fix inside the pause menu.
    Game::getInstance().setActiveSlot(slot);
    m_onSaveGame();

    m_notice = "SAVED TO SLOT " + std::to_string(slot);
    m_noticeTimer = 2.5f;
    m_pendingSlot = 0;
    m_page = Page::Menu;
    // The picker may be reopened before this pause session ends, and it must
    // then report what is actually on disk now.
    refreshSlotPreviews();
}

void PauseState::activateSlotSelection() {
    if (m_slotSelected == SLOT_BACK_ROW) {
        m_page = Page::Menu;
        return;
    }

    const int slot = m_slotSelected + 1;
    if (m_slotPreviews[static_cast<std::size_t>(m_slotSelected)].exists) {
        // Occupied: never overwritten by the same keypress that selected it.
        m_pendingSlot = slot;
        m_confirmSelected = CONFIRM_ROW_KEEP;
        m_page = Page::OverwriteConfirm;
        return;
    }
    commitSaveTo(slot);
}

void PauseState::activateConfirmSelection() {
    if (m_confirmSelected == CONFIRM_ROW_OVERWRITE && m_pendingSlot >= 1) {
        commitSaveTo(m_pendingSlot);
        return;
    }
    // KEEP returns to the picker rather than the pause menu: the player still
    // wanted to save, just not there.
    m_pendingSlot = 0;
    m_page = Page::SlotPicker;
}

void PauseState::activateSelection() {
    if (m_dismissed) return;

    switch (m_page) {
        case Page::SlotPicker:      activateSlotSelection();    return;
        case Page::OverwriteConfirm: activateConfirmSelection(); return;
        case Page::Menu:            break;
    }
    activateMenuSelection();
}

void PauseState::activateMenuSelection() {
    switch (m_selected) {
        case 0: // Resume
            m_dismissed = true;
            Game::getInstance().popState();
            break;
        case 1: // Save game — opens the slot picker, stays on the pause menu
            openSlotPicker();
            break;
        case 2: // Options — pushed over the pause menu, pops back to it
            Game::getInstance().pushState(std::make_unique<OptionsState>());
            break;
        case 3: // Restart level
            m_dismissed = true;
            Game::getInstance().popState();
            if (m_onRestartLevel) m_onRestartLevel();
            break;
        case 4: // Quit to menu
            m_dismissed = true;
            Game::getInstance().popState();
            if (m_onQuitToMenu) m_onQuitToMenu();
            break;
        default:
            break;
    }
}

void PauseState::handleInput(const sf::Event& event) {
    const auto* keyPressed = event.getIf<sf::Event::KeyPressed>();
    if (!keyPressed) return;

    using Key = sf::Keyboard::Key;
    switch (keyPressed->code) {
        case Key::Up:
        case Key::W:
            moveSelection(-1);
            break;
        case Key::Down:
        case Key::S:
            moveSelection(1);
            break;
        case Key::Left:
        case Key::A:
            // Only the confirm page is a horizontal pair; elsewhere this is a
            // dead key, as it was before.
            if (m_page == Page::OverwriteConfirm) moveSelection(-1);
            break;
        case Key::Right:
        case Key::D:
            if (m_page == Page::OverwriteConfirm) moveSelection(1);
            break;
        case Key::Enter:
        case Key::Space:
            activateSelection();
            break;
        case Key::Escape:
        case Key::P:
            // Escape backs out one level at a time, and cancelling the save is
            // the whole point of it here: neither sub-page may save on the way
            // out. Only from the pause menu itself does it resume the game,
            // because Escape is the pause key.
            if (m_page == Page::OverwriteConfirm) {
                m_pendingSlot = 0;
                m_page = Page::SlotPicker;
            } else if (m_page == Page::SlotPicker) {
                m_page = Page::Menu;
            } else if (!m_dismissed) {
                m_dismissed = true;
                Game::getInstance().popState();
            }
            break;
        default:
            break;
    }
}

void PauseState::update(float dt) {
    m_elapsed += dt;
    if (m_noticeTimer > 0.0f) {
        m_noticeTimer -= dt;
        if (m_noticeTimer <= 0.0f) m_notice.clear();
    }
}

void PauseState::render(sf::RenderTarget& target) {
    // PlayingState has already drawn the world in its own camera view.
    target.setView(target.getDefaultView());

    UiRenderer::drawDimmer(target, 150);

    switch (m_page) {
        case Page::SlotPicker:       renderSlotPickerPage(target); return;
        case Page::OverwriteConfirm: renderConfirmPage(target);    return;
        case Page::Menu:             break;
    }
    renderMenuPage(target);
}

void PauseState::renderMenuPage(sf::RenderTarget& target) const {
    const float px = (Constants::WINDOW_WIDTH - PANEL_W) * 0.5f;
    const float py = (Constants::WINDOW_HEIGHT - PANEL_H) * 0.5f;
    UiRenderer::drawPanel(target, {px, py}, {PANEL_W, PANEL_H});

    constexpr float TITLE_Y = 36.0f;
    constexpr float LIST_TOP = 118.0f;

    UiRenderer::drawShadowedText(target, "PAUSED",
                                 {Constants::WINDOW_WIDTH * 0.5f, py + TITLE_Y},
                                 28, sf::Color(255, 216, 0), true);

    // The notice goes in the empty band between the title and the first row,
    // not at the bottom of the panel. Down there it landed on top of QUIT TO
    // MENU — the last of five 42px rows ends at py+482 and the notice was drawn
    // at py+290+... i.e. y 470, overlapping in both axes every single time the
    // player saved. This band belongs to nothing else, so a sixth row can be
    // added without recreating the collision.
    if (!m_notice.empty()) {
        UiRenderer::drawTextFitted(target, m_notice,
                                   {Constants::WINDOW_WIDTH * 0.5f,
                                    py + (TITLE_Y + 34.0f + LIST_TOP) * 0.5f},
                                   10, sf::Color(120, 255, 140), PANEL_W - 32.0f, true);
    }

    UiRenderer::drawMenuItems(target, m_items, m_selected,
                              {px + 78.0f, py + LIST_TOP}, 42.0f, 16, 0.0f, m_elapsed,
                              px + PANEL_W);

    UiRenderer::drawText(target, "UP/DOWN  SELECT   ENTER  CONFIRM",
                         {Constants::WINDOW_WIDTH * 0.5f, py + PANEL_H - 44.0f},
                         10, sf::Color(160, 160, 160), true);
}

void PauseState::renderSlotPickerPage(sf::RenderTarget& target) const {
    const std::vector<UiMenuItem> rows = buildSlotItems();

    const float centerX = Constants::WINDOW_WIDTH * 0.5f;
    const float px = (Constants::WINDOW_WIDTH - WIDE_PANEL_W) * 0.5f;
    const float py = (Constants::WINDOW_HEIGHT - PICKER_PANEL_H) * 0.5f;
    UiRenderer::drawPanel(target, {px, py}, {WIDE_PANEL_W, PICKER_PANEL_H});

    UiRenderer::drawShadowedText(target, "SAVE GAME", {centerX, py + 26.0f}, 22,
                                 sf::Color(255, 216, 0), true);
    UiRenderer::drawText(target, "CHOOSE A SLOT", {centerX, py + 60.0f}, 10,
                         sf::Color(200, 200, 200), true);

    // No explicit value column: the labels are all "SLOT n"/"BACK", so letting
    // drawMenuItems derive it hands the summary the rest of the panel — the
    // same reason the LOAD GAME page passes 0 here.
    UiRenderer::drawMenuItems(target, rows, m_slotSelected,
                              {px + 40.0f, py + 88.0f}, 38.0f, 12, 0.0f, m_elapsed,
                              px + WIDE_PANEL_W);

    UiRenderer::drawText(target, "UP/DOWN  SELECT   ENTER  SAVE HERE   ESC  CANCEL",
                         {centerX, py + PICKER_PANEL_H - 30.0f},
                         10, sf::Color(160, 160, 160), true);
}

void PauseState::renderConfirmPage(sf::RenderTarget& target) const {
    const float centerX = Constants::WINDOW_WIDTH * 0.5f;
    const float px = (Constants::WINDOW_WIDTH - WIDE_PANEL_W) * 0.5f;
    const float py = (Constants::WINDOW_HEIGHT - CONFIRM_PANEL_H) * 0.5f;
    // Red border: this is the only page in the pause overlay that destroys
    // something, and it should not look like the ones that do not.
    UiRenderer::drawPanel(target, {px, py}, {WIDE_PANEL_W, CONFIRM_PANEL_H},
                          sf::Color(0, 0, 0, 235), sf::Color(255, 120, 120, 240));

    UiRenderer::drawShadowedText(target, "OVERWRITE SLOT " + std::to_string(m_pendingSlot) + "?",
                                 {centerX, py + 26.0f}, 20, sf::Color(255, 140, 140), true);

    // What is actually at risk, spelled out. "Are you sure?" on its own does
    // not tell the player which run they would be throwing away.
    const std::size_t index = (m_pendingSlot >= 1 && m_pendingSlot <= SLOT_COUNT)
                            ? static_cast<std::size_t>(m_pendingSlot - 1) : 0;
    UiRenderer::drawText(target, "THIS SAVE WILL BE LOST:", {centerX, py + 66.0f}, 10,
                         sf::Color(200, 200, 200), true);
    UiRenderer::drawTextFitted(target, m_slotPreviews[index].summary(),
                               {centerX, py + 88.0f}, 13, sf::Color(255, 255, 255),
                               WIDE_PANEL_W - 48.0f, true);

    std::vector<UiMenuItem> rows;
    rows.emplace_back("KEEP IT");
    rows.emplace_back("OVERWRITE");
    UiRenderer::drawMenuItems(target, rows, m_confirmSelected,
                              {px + 220.0f, py + 128.0f}, 34.0f, 14, 0.0f, m_elapsed,
                              px + WIDE_PANEL_W);

    UiRenderer::drawText(target, "UP/DOWN  SELECT   ENTER  CONFIRM   ESC  CANCEL",
                         {centerX, py + CONFIRM_PANEL_H - 28.0f},
                         10, sf::Color(160, 160, 160), true);
}
