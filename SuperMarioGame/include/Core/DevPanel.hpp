#pragma once

#include <functional>
#include <vector>

class PlayingState;

// Developer/debug ImGui interface for PlayingState.
//
// Why this class exists
// ---------------------
// These panels used to live inside PlayingState::render(), where their buttons
// called setupTestScene(), MapGenerator::generate(), loadLevelByPath(),
// Game::changeState() and friends directly. That made rendering mutate the
// world: render() runs once per rendered frame while update() runs on the fixed
// timestep, so those mutations landed off-cadence from the physics they affected
// (audit A-9).
//
// Ordering constraint
// -------------------
// ImGui calls are only legal between ImGui::SFML::Update() and
// ImGui::SFML::Render(), and Game::run() calls ImGui::SFML::Update() *after* the
// fixed-timestep update loop. Drawing therefore has to stay on the render path.
//
// So the split is by effect, not by call site: draw() issues ImGui calls and
// records any requested change as a closure; flush() replays those closures from
// PlayingState::update(). render() still paints the panels, but no longer
// changes a single piece of game state.
class DevPanel {
public:
    // Issues all dev ImGui windows. Call from PlayingState::render().
    // Mutates only this panel's own UI state and the pending-action queue.
    void draw(PlayingState& state);

    // Applies everything draw() queued. Call from PlayingState::update(),
    // before the frame's simulation runs.
    void flush(PlayingState& state);

    // Read by PlayingState::render() to decide whether to paint AABB overlays.
    bool showAABB() const { return m_showAABB; }

    void clearPending() { m_pending.clear(); }

private:
    using Action = std::function<void(PlayingState&)>;
    void queue(Action action) { m_pending.push_back(std::move(action)); }

    // Individual windows, split out so each stays readable.
    void drawNavigationPanel(PlayingState& state);
    void drawGeneratorPanel(PlayingState& state);
    void drawPlaygroundPanel(PlayingState& state);
    void drawPersistencePanel(PlayingState& state);
    void drawAchievementToasts();
    // Task 9.1's AI debug overlay: what every live enemy is doing and why.
    void drawAiOverlay(PlayingState& state);
    // Live tunables for the two-player modes: the shadow's delay and drift
    // correction, and the CPU opponent's skill, style, reaction and noise. These
    // are the numbers docs/two_player_ai_plan.md picks values for, and picking
    // them by feel needs them on sliders rather than in a rebuild.
    void drawMatchPanel(PlayingState& state);

    std::vector<Action> m_pending;

    // Purely presentational state — owned here, not by PlayingState.
    bool m_showAABB = false;
    bool m_showAiOverlay = false;
    bool m_showAiVision = false;
    int  m_selectedPipeIndex = 0;
};
