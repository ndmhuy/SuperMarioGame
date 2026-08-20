#pragma once

#include "Core/IGameState.hpp"
#include "Graphics/UiRenderer.hpp"
#include "Utils/Serializer.hpp"
#include <string>
#include <vector>

// Task 7.8 — Options & High Scores.
//
// Every value edited here already had a backing store before this screen
// existed: Game holds the volumes, difficulty and colourblind flag, Serializer
// persists them, and InputManager applies key bindings live (audit B-11). This
// state is the UI over them, nothing more — it owns no settings of its own.
//
// It is an overlay so it can be opened from the main menu *and* from the pause
// menu without either of them needing to know how to rebuild itself afterwards:
// closing it pops back to whichever pushed it.
class OptionsState : public IGameState {
public:
    // Which page it opens on. The main menu's RECORDS row wants Statistics, the
    // OPTIONS row wants Settings, and Tab cycles all four either way.
    enum class Page { Settings, HighScores, Statistics, Achievements };

    OptionsState() = default;
    explicit OptionsState(Page startPage) : m_page(startPage) {}
    ~OptionsState() override = default;

    void enter() override;
    void exit() override;
    void handleInput(const sf::Event& event) override;
    void update(float dt) override;
    void render(sf::RenderTarget& target) override;

    bool isOverlay() const override { return true; }

private:
    // Row kinds drive what Left/Right and Enter do on the selected line.
    enum class RowKind { Volume, Difficulty, Toggle, Binding, Action };

    struct Row {
        RowKind kind;
        std::string label;
        std::string actionId;   // Binding rows: the InputManager action name
        bool isMusic = false;   // Volume rows: music vs sfx
    };

    void buildRows();
    void adjustSelected(int direction);
    void activateSelected();
    std::string valueTextFor(const Row& row) const;
    void close();
    void renderStatisticsPage(sf::RenderTarget& target) const;
    void renderAchievementsPage(sf::RenderTarget& target) const;

    Page m_page = Page::Settings;
    std::vector<Row> m_rows;
    int m_selected = 0;
    float m_elapsed = 0.0f;

    // Index of the row waiting for a key press, or -1 when not rebinding.
    int m_awaitingBindingRow = -1;

    // Transient feedback line — a reset confirmation, or a warning that a
    // rebind displaced another control.
    std::string m_notice;
    float m_noticeTimer = 0.0f;

    std::vector<HighScoreEntry> m_highScores;
    // Achievements can outnumber the panel; the list scrolls.
    int m_achievementScroll = 0;
    bool m_closing = false;
};
