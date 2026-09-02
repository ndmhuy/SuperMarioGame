#include "Core/DevPanel.hpp"

#include "Core/PlayingState.hpp"
#include "Core/Game.hpp"
#include "Core/MenuState.hpp"
#include "Core/StatisticsTracker.hpp"
#include "Core/AchievementManager.hpp"
#include "Core/SoundManager.hpp"
#include "Entities/Player.hpp"
#include "Entities/Mario.hpp"
#include "Entities/Luigi.hpp"
#include "Entities/ShadowMario.hpp"
#include "Entities/AIController.hpp"
#include "Entities/Pipe.hpp"
#include "Entities/Mushroom.hpp"
#include "Entities/FireFlower.hpp"
#include "Entities/Coin.hpp"
#include "Entities/Star.hpp"
#include "Entities/CapeFeather.hpp"
#include "Entities/MegaMushroom.hpp"
#include "Entities/MiniMushroom.hpp"
#include "Entities/IPlayerState.hpp"
#include "Entities/Enemy.hpp"
#include "Entities/Boss.hpp"
#include "Entities/IMovementStrategy.hpp"
#include "Utils/Constants.hpp"
#include "Utils/Serializer.hpp"
#include "Utils/LevelCatalog.hpp"
#include "Utils/MapGenerator.hpp"

#include <imgui.h>

#include <cfloat>
#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <vector>

namespace {

// The campaign order lives in LevelCatalog. This file used to hold its own copy
// of it, which is how the dropdown kept offering seven levels after the campaign
// dropped to four.

const char* const kCharacters[] = { "Mario (Red)", "Luigi (Green)", "Toad (Blue)", "Peach (Pink)" };

} // namespace

void DevPanel::flush(PlayingState& state) {
    if (m_pending.empty()) return;

    // Move first: an action may queue further actions (a level load can request a
    // state change), and those must run on the next flush rather than mid-iteration.
    std::vector<Action> ready = std::move(m_pending);
    m_pending.clear();
    for (auto& action : ready) {
        action(state);
    }
}

void DevPanel::draw(PlayingState& state) {
    drawNavigationPanel(state);

    if (state.m_isProcedural) {
        drawGeneratorPanel(state);
    }

    if (!state.m_mapEditor.isActive()) {
        drawCheatPanel(state);
        drawPlaygroundPanel(state);
        drawPersistencePanel(state);
        // Achievement toasts are NOT drawn here. UiRenderer::drawAchievementToasts,
        // called once from Game::run(), is the single renderer of
        // AchievementManager's toast model — this panel used to draw a second
        // ImGui card from the same model, anchored within ~4px of it, and painted
        // over it every frame an achievement unlocked (defect 9, R21).
        if (m_showAiOverlay) {
            drawAiOverlay(state);
        }
        // Not behind the overlay toggle: this one appears only in the modes it
        // applies to, so it cannot clutter an ordinary run.
        drawMatchPanel(state);
        drawLightingPanel(state);
    }
}

// ---------------------------------------------------------------------------

void DevPanel::drawAiOverlay(PlayingState& state) {
    ImGui::SetNextWindowPos(ImVec2(744.0f, 8.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(520.0f, 300.0f), ImGuiCond_FirstUseEver);
    ImGui::Begin("AI Debug Overlay");

    ImGui::Text("Live enemies: what each one is running, and what state it is in.");
    ImGui::Separator();

    if (ImGui::BeginTable("ai_overlay", 6,
                          ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                          ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn("Enemy");
        ImGui::TableSetupColumn("Strategy");
        ImGui::TableSetupColumn("State");
        ImGui::TableSetupColumn("Position");
        ImGui::TableSetupColumn("Velocity");
        ImGui::TableSetupColumn("Status");
        ImGui::TableHeadersRow();

        int shown = 0;
        for (const auto& entity : state.m_entities) {
            auto* enemy = dynamic_cast<Enemy*>(entity.get());
            if (!enemy || !enemy->isActive()) continue;

            const IMovementStrategy* strategy = enemy->getStrategy();
            const std::string strategyName = strategy ? strategy->getName() : "-- none --";
            const std::string debugState = strategy ? strategy->getDebugState() : "";

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("%s", enemy->getTypeName().c_str());
            ImGui::TableNextColumn();
            // A strategy-less enemy is worth spotting: it will never move.
            if (strategy) ImGui::Text("%s", strategyName.c_str());
            else          ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.3f, 1.0f), "%s", strategyName.c_str());
            ImGui::TableNextColumn();
            ImGui::Text("%s", debugState.empty() ? "-" : debugState.c_str());
            ImGui::TableNextColumn();
            ImGui::Text("%.0f, %.0f", enemy->getPosition().x, enemy->getPosition().y);
            ImGui::TableNextColumn();
            ImGui::Text("%.0f, %.0f", enemy->getVelocity().x, enemy->getVelocity().y);
            ImGui::TableNextColumn();
            if (auto* boss = dynamic_cast<Boss*>(enemy)) {
                ImGui::Text("BOSS %d/%d p%d", boss->getHealth(), boss->getMaxHealth(), boss->getPhase());
            } else if (enemy->isDeadOrDying()) {
                ImGui::Text("dying");
            } else {
                ImGui::Text("%s", enemy->isFacingRight() ? "-> right" : "left <-");
            }
            ++shown;
        }
        ImGui::EndTable();

        if (shown == 0) {
            ImGui::TextDisabled("No live enemies in this level.");
        }
    }

    ImGui::End();
}

void DevPanel::drawCheatPanel(PlayingState& state) {
    // (912, 8) is the slot the old floating map-editor window used, and this
    // panel is only ever drawn while the editor is NOT active (see draw()), so
    // the two can never be up at once. Collapsed by default, like every other
    // dev window: a panel that covers the game is useless to someone recording
    // the game.
    ImGui::SetNextWindowPos(ImVec2(912.0f, 8.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(360.0f, 600.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowCollapsed(true, ImGuiCond_FirstUseEver);
    ImGui::Begin("Debug > Cheats (recording aids)");

    DebugCheats& cheats = Game::getInstance().debugCheats();

    // ImGui's TextDisabled and TextColored do not wrap, and this window is
    // narrow on purpose (it must not cover the game while recording), so the
    // explanatory lines below would run off the right edge and be unreadable.
    auto wrappedText = [](ImVec4 colour, const char* text) {
        ImGui::PushStyleColor(ImGuiCol_Text, colour);
        ImGui::PushTextWrapPos(0.0f);
        ImGui::TextUnformatted(text);
        ImGui::PopTextWrapPos();
        ImGui::PopStyleColor();
    };
    const ImVec4 dimmed = ImGui::GetStyle().Colors[ImGuiCol_TextDisabled];

    ImGui::TextWrapped("Switches for recording an instruction video. All of them "
                       "reset when a run starts, and a run that used any of them "
                       "writes no high score and unlocks no achievement.");
    ImGui::Separator();

    // Written straight through rather than queued: DebugCheats holds no game
    // state — no entity list, no tilemap — so flipping one during render()
    // cannot upset the frame the way the queued actions in this file's other
    // panels would. What READS the flag is already on the fixed timestep.
    for (const DebugCheats::Cheat cheat : DebugCheats::all()) {
        bool on = cheats.isOn(cheat);
        if (ImGui::Checkbox(DebugCheats::label(cheat), &on)) {
            cheats.set(cheat, on);
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", DebugCheats::description(cheat));
        }
    }
    wrappedText(dimmed, "Shortcuts, in this order: F2 F3 F4 F6 F7 F8 F9 F10. "
                        "(F1 is the editor and F5 is playtest, so both are skipped.)");

    ImGui::Separator();
    ImGui::Text("Time scale (simulation only)");
    float timeScale = cheats.simulationTimeScale();
    if (ImGui::SliderFloat("##timescale", &timeScale,
                           DebugCheats::MIN_TIME_SCALE, DebugCheats::MAX_TIME_SCALE,
                           "%.2fx")) {
        cheats.setTimeScale(timeScale);
    }
    ImGui::SameLine();
    if (ImGui::Button("1x")) cheats.setTimeScale(1.0f);
    wrappedText(dimmed, "Slow motion for showing coyote time, wall jumps and ground pounds. "
                        "Input and this panel keep real time. "
                        "F11 steps 1x - 0.5x - 0.25x - 0.1x without the mouse.");

    ImGui::Separator();
    ImGui::Text("Power state");
    if (state.m_player) {
        // Through setForm()/powerUp(), which are the State and Decorator
        // machinery's own doors: setForm swaps the base form and leaves an
        // active Star or Mega decorator wrapped around it (audit A-8), and
        // powerUp() is what applies the decorators. Assigning m_currentState
        // here would discard whichever of those was running.
        struct FormButton { const char* label; Player::Form form; };
        static const FormButton kForms[] = {
            {"Small", Player::Form::Small}, {"Super", Player::Form::Super},
            {"Fire",  Player::Form::Fire},  {"Cape",  Player::Form::Cape},
            {"Mini",  Player::Form::Mini}
        };
        for (int i = 0; i < 5; ++i) {
            if (i > 0) ImGui::SameLine();
            if (ImGui::Button(kForms[i].label)) {
                const Player::Form form = kForms[i].form;
                queue([form](PlayingState& s) {
                    if (s.m_player) s.m_player->setForm(form);
                });
            }
        }
        // The two decorators. Item type indices match Player::powerUp's switch,
        // the same table the console's `give` command indexes.
        if (ImGui::Button("Star (10s)")) {
            queue([](PlayingState& s) { if (s.m_player) s.m_player->powerUp(4); });
        }
        ImGui::SameLine();
        if (ImGui::Button("Mega (8s)")) {
            queue([](PlayingState& s) { if (s.m_player) s.m_player->powerUp(5); });
        }
    } else {
        ImGui::TextDisabled("No active player.");
    }

    ImGui::Separator();
    ImGui::Text("Staging");
    if (ImGui::Button("Kill all enemies on screen")) {
        queue([](PlayingState& s) { s.clearOnScreenEnemies(); });
    }
    if (ImGui::Button("Instant level complete")) {
        // The same event the flagpole publishes, so the flagpole, the castle
        // flag, the walk to the door and the summary all run exactly as they
        // would have. A boss level deliberately refuses while its boss lives —
        // PlayingState's LevelComplete handler enforces SPEC 6.4 and a cheat
        // must not be the one way around a rule the level data can already get
        // wrong.
        queue([](PlayingState&) {
            EventBus::getInstance().publish({EventType::LevelComplete, 0});
        });
    }

    if (cheats.tainted()) {
        ImGui::Separator();
        wrappedText(ImVec4(1.0f, 0.75f, 0.2f, 1.0f),
                    "This run is a demo: no high score, no achievements.");
    }

    ImGui::End();
}

void DevPanel::drawMatchPanel(PlayingState& state) {
    // Only present when there is a match to tune. In a single-player run every
    // control here would be dead, and a window full of greyed-out sliders is
    // worse than no window.
    if (!state.m_aiController && !state.m_shadow) return;

    ImGui::SetNextWindowPos(ImVec2(744.0f, 320.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(520.0f, 360.0f), ImGuiCond_FirstUseEver);
    ImGui::Begin("Match: Shadow & CPU");

    if (state.m_shadow) {
        ImGui::TextColored(ImVec4(0.75f, 0.5f, 1.0f, 1.0f), "Shadow Mario");
        ImGui::Text("Replaying: %s", state.m_shadow->hasStarted() ? "yes" : "filling buffer");
        ImGui::Text("Spatial gap: %.2fs", state.shadowProximitySeconds());

        // The delay is the single number that decides whether the mode is tense
        // or trivial, so it is tunable while the game runs rather than a
        // recompile away.
        float delay = state.m_shadow->getDelay();
        if (ImGui::SliderFloat("Delay (s)", &delay, 0.5f, 8.0f, "%.2f")) {
            const float requested = delay;
            queue([requested](PlayingState& s) {
                if (s.m_shadow) s.m_shadow->setDelay(requested);
            });
        }

        static float correctionThreshold = 4.0f;
        static float correctionFactor = 0.1f;
        bool changed = ImGui::SliderFloat("Drift threshold (px)", &correctionThreshold, 0.0f, 32.0f, "%.1f");
        changed |= ImGui::SliderFloat("Drift correction", &correctionFactor, 0.0f, 1.0f, "%.2f");
        if (changed) {
            const float threshold = correctionThreshold;
            const float factor = correctionFactor;
            queue([threshold, factor](PlayingState& s) {
                if (s.m_shadow) s.m_shadow->setCorrection(threshold, factor);
            });
        }
        ImGui::TextDisabled("Inputs drive the shadow; the recorded position is the leash.");
        ImGui::Separator();
    }

    if (state.m_aiController) {
        AIController& ai = *state.m_aiController;
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.3f, 1.0f), "CPU opponent");
        ImGui::Text("Policy: %s   Doing: %s", ai.policyName(), ai.reason());
        ImGui::Text("Vision radius: %d tiles", ai.getVisionRadius());

        int difficulty = static_cast<int>(ai.getDifficulty());
        if (ImGui::Combo("Skill", &difficulty, "Easy\0Normal\0Hard\0")) {
            const auto requested = static_cast<AIDifficulty>(difficulty);
            queue([requested](PlayingState& s) {
                if (s.m_aiController) s.m_aiController->setDifficulty(requested);
            });
        }
        int archetype = static_cast<int>(ai.getArchetype());
        if (ImGui::Combo("Style", &archetype, "Speedrunner\0Hunter\0Collector\0")) {
            const auto requested = static_cast<AIArchetype>(archetype);
            queue([requested](PlayingState& s) {
                if (s.m_aiController) s.m_aiController->setArchetype(requested);
            });
        }

        // Both of these are set by the skill preset above; overriding them
        // afterwards is how the difficulty table's numbers get felt rather than
        // argued about.
        float latency = ai.getReactionLatency();
        if (ImGui::SliderFloat("Reaction (s)", &latency, 0.0f, 1.0f, "%.3f")) {
            const float requested = latency;
            queue([requested](PlayingState& s) {
                if (s.m_aiController) s.m_aiController->setReactionLatency(requested);
            });
        }
        float noise = ai.getActionNoise();
        if (ImGui::SliderFloat("Action noise", &noise, 0.0f, 1.0f, "%.2f")) {
            const float requested = noise;
            queue([requested](PlayingState& s) {
                if (s.m_aiController) s.m_aiController->setActionNoise(requested);
            });
        }

        ImGui::Checkbox("Draw vision grid", &m_showAiVision);
        if (m_showAiVision) {
            // The grid as the bot sees it, one character per cell. A bot walking
            // into a wall is much easier to diagnose when you can see that it
            // believed the wall was empty.
            ImGui::TextDisabled("# solid  ^ enemy  $ reward  ! hazard  . empty  ? unseen");
            const AIObservation& obs = ai.lastObservation();
            const int halfW = kAIVisionWidth / 2;
            const int halfH = kAIVisionHeight / 2;
            for (int dy = -halfH; dy <= halfH; ++dy) {
                std::string row;
                row.reserve(static_cast<std::size_t>(kAIVisionWidth) + 1);
                for (int dx = -halfW; dx <= halfW; ++dx) {
                    if (dx == 0 && dy == 0) { row += '@'; continue; }
                    switch (obs.at(dx, dy)) {
                        case AICellState::Solid:   row += '#'; break;
                        case AICellState::Enemy:   row += '^'; break;
                        case AICellState::Reward:  row += '$'; break;
                        case AICellState::Hazard:  row += '!'; break;
                        case AICellState::Empty:   row += '.'; break;
                        case AICellState::Unknown: row += '?'; break;
                    }
                }
                ImGui::TextUnformatted(row.c_str());
            }
        }
    }

    ImGui::End();
}

void DevPanel::drawLightingPanel(PlayingState& state) {
    // Collapsed and placed, like every other window in this file: they all used
    // to open at ImGui's default position and stack over the player.
    //
    // (912, 320) is the last free cell of the 4x2 grid the other seven windows
    // sit in. It is NOT (8, 320): that is four pixels off the generator panel's
    // (8, 316), and a first run put this window's collapsed title bar straight
    // on top of "Procedural Level Generator Tuning" — the exact stacking the
    // comment above records having already been fixed once. Observed in
    // saves/shots/r21o_06_t235s.png before this line was changed.
    ImGui::SetNextWindowPos(ImVec2(912.0f, 320.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(360.0f, 340.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowCollapsed(true, ImGuiCond_FirstUseEver);
    ImGui::Begin("Debug > Lighting (Bonus D)");

    const ImVec4 dimmed = ImGui::GetStyle().Colors[ImGuiCol_TextDisabled];
    auto wrapped = [](ImVec4 colour, const char* text) {
        ImGui::PushStyleColor(ImGuiCol_Text, colour);
        ImGui::PushTextWrapPos(0.0f);
        ImGui::TextUnformatted(text);
        ImGui::PopTextWrapPos();
        ImGui::PopStyleColor();
    };

    // Say up front whether there is a GPU behind any of this. LightingRenderer
    // degrades to drawing nothing on a machine with no GLSL, which is correct
    // and completely silent — so a panel of live sliders that changed nothing on
    // screen would read as a broken panel rather than as a driver without
    // shaders. isOperational() performs the one-time load, hence the non-const
    // reference.
    const bool operational = state.m_lighting.isOperational();
    if (operational) {
        ImGui::TextColored(ImVec4(0.6f, 1.0f, 0.7f, 1.0f), "Shader loaded - the pass is drawing.");
    } else {
        ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.5f, 1.0f),
                           "No GLSL on this machine - the pass draws nothing.");
        wrapped(dimmed, "The sliders below still move the model; nothing will change on screen.");
    }

    // The model's current answer, so a slider can be judged against a number
    // rather than against an impression of the frame.
    const float clock = state.m_lightingTunables.clockFrozen
                      ? state.m_lightingTunables.frozenPhase * LightingRenderer::DAY_NIGHT_PERIOD
                      : state.m_runElapsed;
    const float phase = LightingRenderer::dayNightPhase(clock);
    ImGui::Text("Phase %.3f   night %.3f   darkness %.3f", phase,
                LightingRenderer::nightFactor(phase),
                LightingRenderer::darknessFor(state.m_background.getTheme(), clock));

    ImGui::Separator();
    ImGui::Text("Day/night clock");

    // Queued rather than written through, like every other mutation in this
    // file: draw() runs on the render path and DevPanel's contract is that it
    // touches nothing but its own UI state and this queue (see DevPanel.hpp).
    bool frozen = state.m_lightingTunables.clockFrozen;
    if (ImGui::Checkbox("Freeze at phase", &frozen)) {
        const bool requested = frozen;
        queue([requested](PlayingState& s) { s.m_lightingTunables.clockFrozen = requested; });
    }
    float frozenPhase = state.m_lightingTunables.frozenPhase;
    if (ImGui::SliderFloat("Phase (0 noon, 0.5 midnight)", &frozenPhase, 0.0f, 1.0f, "%.3f")) {
        const float requested = frozenPhase;
        queue([requested](PlayingState& s) { s.m_lightingTunables.frozenPhase = requested; });
    }
    wrapped(dimmed, "Holding the cycle is what makes a night level checkable: unfrozen, "
                    "reaching midnight means waiting up to 50 s of the 100 s period. "
                    "Phase 0.5 reaches the full NIGHT_DARKNESS, so nothing is out of reach.");

    ImGui::Separator();
    ImGui::Text("Player lamp");
    float playerRadius = state.m_lightingTunables.playerRadius;
    if (ImGui::SliderFloat("Radius (px)", &playerRadius, 32.0f, 640.0f, "%.0f")) {
        const float requested = playerRadius;
        queue([requested](PlayingState& s) { s.m_lightingTunables.playerRadius = requested; });
    }
    float breathe = state.m_lightingTunables.playerBreathe;
    if (ImGui::SliderFloat("Breathe", &breathe, 0.0f, 0.25f, "%.3f")) {
        const float requested = breathe;
        queue([requested](PlayingState& s) { s.m_lightingTunables.playerBreathe = requested; });
    }
    float tint[3] = {state.m_lightingTunables.playerShadowTint[0],
                     state.m_lightingTunables.playerShadowTint[1],
                     state.m_lightingTunables.playerShadowTint[2]};
    if (ImGui::ColorEdit3("Shadow tint", tint)) {
        const float r = tint[0], g = tint[1], b = tint[2];
        queue([r, g, b](PlayingState& s) {
            s.m_lightingTunables.playerShadowTint[0] = r;
            s.m_lightingTunables.playerShadowTint[1] = g;
            s.m_lightingTunables.playerShadowTint[2] = b;
        });
    }
    // The single mistake this control invites, named where it is made. See
    // LightingRenderer::Light::shadowTint.
    wrapped(dimmed, "This is the colour of shadow the lamp has NOT cleared, not the lamp's "
                    "own brightness. Picking something brighter than the scene draws a bright "
                    "ring with a dark hole in it.");

    ImGui::Separator();
    ImGui::Text("Fireball lamp");
    float fireballRadius = state.m_lightingTunables.fireballRadius;
    if (ImGui::SliderFloat("Radius (px)##fireball", &fireballRadius, 16.0f, 480.0f, "%.0f")) {
        const float requested = fireballRadius;
        queue([requested](PlayingState& s) { s.m_lightingTunables.fireballRadius = requested; });
    }
    float fireballIntensity = state.m_lightingTunables.fireballIntensity;
    if (ImGui::SliderFloat("Intensity", &fireballIntensity, 0.0f, 1.0f, "%.2f")) {
        const float requested = fireballIntensity;
        queue([requested](PlayingState& s) { s.m_lightingTunables.fireballIntensity = requested; });
    }

    ImGui::Separator();
    ImGui::Text("Free camera lamp (F9)");
    float freeCameraRadius = state.m_lightingTunables.freeCameraRadius;
    if (ImGui::SliderFloat("Radius (px)##freecam", &freeCameraRadius, 64.0f, 960.0f, "%.0f")) {
        const float requested = freeCameraRadius;
        queue([requested](PlayingState& s) { s.m_lightingTunables.freeCameraRadius = requested; });
    }

    ImGui::Separator();
    if (ImGui::Button("Reset to shipped defaults")) {
        queue([](PlayingState& s) { s.m_lightingTunables = PlayingState::LightingTunables{}; });
    }
    ImGui::Text("Lamp slots in use: up to %zu", LightingRenderer::MAX_LIGHTS);

    ImGui::End();
}

void DevPanel::drawNavigationPanel(PlayingState& state) {
    // Placed and collapsed by default. Every dev window opened at ImGui's
    // default position, so they stacked on top of one another and covered the
    // left half of the screen — including the player.
    ImGui::SetNextWindowPos(ImVec2(8.0f, 8.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(360.0f, 300.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowCollapsed(true, ImGuiCond_FirstUseEver);
    ImGui::Begin("Gameplay Controls & Navigation");

    ImGui::Text("Simulation State:");
    if (state.m_player) {
        ImGui::Text("Player Position: (%.1f, %.1f)", state.m_player->getPosition().x, state.m_player->getPosition().y);
        ImGui::Text("Player Velocity: (%.1f, %.1f)", state.m_player->getVelocity().x, state.m_player->getVelocity().y);
        ImGui::Text("Lives: %d | Coins: %d | Score: %d",
                    state.m_player->getLives(), state.m_player->getCoins(), state.m_player->getScore());
    } else {
        ImGui::Text("No active player.");
    }

    ImGui::Separator();
    ImGui::Text("Select Campaign Level:");
    int levelIdx = state.m_selectedLevelIndex;
    ImGui::SetNextItemWidth(-FLT_MIN);
    const auto& levelItems = LevelCatalog::longNameItems();
    if (ImGui::Combo("##SelectLevel", &levelIdx, levelItems.data(),
                     static_cast<int>(levelItems.size()))) {
        queue([levelIdx](PlayingState& s) {
            s.m_selectedLevelIndex = levelIdx;
            s.m_isProcedural = false;
            s.setupTestScene();
        });
    }

    ImGui::Separator();
    ImGui::Text("Active Level Tube / Warp Pipe Destinations:");

    std::vector<Pipe*> activePipes;
    std::vector<std::string> pipeLabels;
    for (const auto& entity : state.m_entities) {
        if (auto pipe = dynamic_cast<Pipe*>(entity.get())) {
            activePipes.push_back(pipe);
            pipeLabels.push_back(
                "Tube @" + std::to_string(static_cast<int>(pipe->getPosition().x / Constants::TILE_SIZE)) +
                " -> " + (pipe->getTargetLevel().empty() ? "Same Level Teleport" : pipe->getTargetLevel()));
        }
    }

    if (activePipes.empty()) {
        ImGui::TextDisabled("No active warp tubes in current level.");
    } else {
        if (m_selectedPipeIndex >= static_cast<int>(activePipes.size())) m_selectedPipeIndex = 0;

        std::vector<const char*> pipeItems;
        pipeItems.reserve(pipeLabels.size());
        for (const auto& l : pipeLabels) pipeItems.push_back(l.c_str());

        ImGui::SetNextItemWidth(-90.0f);
        ImGui::Combo("##TubeDropdown", &m_selectedPipeIndex, pipeItems.data(), static_cast<int>(pipeItems.size()));
        ImGui::SameLine();
        if (ImGui::Button("Enter Tube")) {
            // Capture by value: the Pipe* may be destroyed by the very level load
            // this action triggers, so the closure must not dereference it later.
            const std::string targetLevel = activePipes[m_selectedPipeIndex]->getTargetLevel();
            const sf::Vector2f exitPos    = activePipes[m_selectedPipeIndex]->getExitPosition();
            queue([targetLevel, exitPos](PlayingState& s) {
                if (!s.m_player) return;
                SoundManager::getInstance().playSound("pipe");
                if (!targetLevel.empty()) {
                    s.loadLevelByPath(targetLevel, exitPos);
                } else if (exitPos.x != 0.0f || exitPos.y != 0.0f) {
                    s.m_player->setPosition(exitPos);
                    s.m_player->setVelocity({0.0f, 0.0f});
                }
            });
        }
    }

    ImGui::Separator();
    if (state.m_mapEditor.isActive()) {
        ImGui::TextDisabled("Editing. The level editor window owns navigation.");
    } else {
        if (ImGui::Button("Return to Main Menu")) {
            queue([](PlayingState&) { Game::getInstance().changeState(std::make_unique<MenuState>()); });
        }
        ImGui::SameLine();
        if (ImGui::Button("Toggle Map Editor (F1)")) {
            queue([](PlayingState& s) { s.m_mapEditor.toggleActive(); });
        }
    }

    ImGui::End();
}

// ---------------------------------------------------------------------------

void DevPanel::drawGeneratorPanel(PlayingState& state) {
    ImGui::SetNextWindowPos(ImVec2(8.0f, 316.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(360.0f, 300.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowCollapsed(true, ImGuiCond_FirstUseEver);
    ImGui::Begin("Procedural Level Generator Tuning");
    if (state.m_lastLevelUnverified) {
        ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.2f, 1.0f),
                            "Layout unverified: every solvability reseed failed.");
    }
    ImGui::Text("Live tuning parameters:");

    // The config is plain data with no side effects, so sliders may edit it in
    // place. Only the regenerate action is deferred.
    MapGeneratorConfig& cfg = state.m_genConfig;

    int themeIdx = static_cast<int>(cfg.theme);
    const char* themes[] = { "Overworld", "Underground", "Castle", "Ice" };
    if (ImGui::Combo("Theme", &themeIdx, themes, 4)) {
        cfg.theme = static_cast<MapTheme>(themeIdx);
    }

    int diffIdx = static_cast<int>(cfg.difficulty);
    const char* diffs[] = { "Easy", "Medium", "Hard" };
    if (ImGui::Combo("Difficulty", &diffIdx, diffs, 3)) {
        cfg.difficulty = static_cast<MapDifficulty>(diffIdx);
    }

    ImGui::SliderFloat("Roughness", &cfg.roughness, 0.0f, 1.0f, "%.2f");
    ImGui::SliderFloat("Pit Ratio", &cfg.pitProbability, 0.0f, 0.35f, "%.2f");
    ImGui::SliderFloat("Enemy Rate", &cfg.enemySpawnRate, 0.0f, 0.40f, "%.2f");
    ImGui::Checkbox("Castle Lava Hazards", &cfg.enableLava);
    ImGui::Checkbox("Moving Platforms", &cfg.enableMovingPlatforms);

    int seedVal = static_cast<int>(cfg.seed);
    if (ImGui::InputInt("Seed (0=Random)", &seedVal)) {
        cfg.seed = (seedVal < 0) ? 0u : static_cast<unsigned int>(seedVal);
    }
    ImGui::SameLine();
    if (ImGui::Button("New Seed")) {
        cfg.seed = std::random_device{}();
    }

    if (ImGui::Button("Regenerate Level")) {
        queue([](PlayingState& s) { s.regenerateProceduralLevel(); });
    }

    ImGui::End();
}

// ---------------------------------------------------------------------------

void DevPanel::drawPlaygroundPanel(PlayingState& state) {
    ImGui::SetNextWindowPos(ImVec2(376.0f, 8.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowCollapsed(true, ImGuiCond_FirstUseEver);
    ImGui::Begin("Physics & Input Test Playground (Phase 2 & 3)");

    ImGui::Text("Select Active Character:");
    int charIdx = state.m_selectedCharIndex;
    if (ImGui::Combo("Character", &charIdx, kCharacters, 4)) {
        queue([charIdx](PlayingState& s) {
            s.m_selectedCharIndex = charIdx;
            if (s.m_player) s.spawnSelectedPlayer(s.m_player->getPosition());
        });
    }

    ImGui::Text("Select Active Level:");
    int levelIdx = state.m_selectedLevelIndex;
    const auto& playgroundLevelItems = LevelCatalog::longNameItems();
    if (ImGui::Combo("Level", &levelIdx, playgroundLevelItems.data(),
                     static_cast<int>(playgroundLevelItems.size()))) {
        queue([levelIdx](PlayingState& s) {
            s.m_selectedLevelIndex = levelIdx;
            s.m_isProcedural = false;
            s.setupTestScene();
        });
    }

    ImGui::Separator();

    if (state.m_player) {
        const Player& p = *state.m_player;
        ImGui::Text("Character Stats:");
        ImGui::BulletText("Position: (%.2f, %.2f)", p.getPosition().x, p.getPosition().y);
        ImGui::BulletText("Velocity: (%.2f, %.2f)", p.getVelocity().x, p.getVelocity().y);
        ImGui::BulletText("onGround: %s", p.isOnGround() ? "TRUE" : "FALSE");
        ImGui::BulletText("onWall: %s", p.isOnWall() ? "TRUE" : "FALSE");
        ImGui::BulletText("Crouched: %s", p.isCrouched() ? "TRUE" : "FALSE");
        ImGui::BulletText("Sliding: %s", p.isSliding() ? "TRUE" : "FALSE");
        ImGui::BulletText("Coyote Frames Left: %d", p.getCoyoteFramesLeft());
        ImGui::BulletText("Jump Buffer Frames Left: %d", p.getJumpBufferFramesLeft());
        ImGui::BulletText("Lives: %d, Coins: %d, Score: %d", p.getLives(), p.getCoins(), p.getScore());

        std::string stateName = "Unknown";
        if (IPlayerState* current = p.getCurrentState()) {
            bool invincible = false;
            bool mega = false;
            IPlayerState* baseState = current;
            while (auto* decorator = dynamic_cast<PlayerStateDecorator*>(baseState)) {
                if (dynamic_cast<StarDecorator*>(decorator)) invincible = true;
                if (dynamic_cast<MegaDecorator*>(decorator)) mega = true;
                baseState = decorator->getWrappedState();
            }
            if (dynamic_cast<SmallState*>(baseState)) stateName = "Small";
            else if (dynamic_cast<SuperState*>(baseState)) stateName = "Super";
            else if (dynamic_cast<FireState*>(baseState)) stateName = "Fire";
            else if (dynamic_cast<CapeState*>(baseState)) stateName = "Cape";
            else if (dynamic_cast<MiniState*>(baseState)) stateName = "Mini";

            if (invincible) stateName += " + Star (Invincible)";
            if (mega) stateName += " + Mega (Giant)";
        }
        ImGui::BulletText("Active Form: %s", stateName.c_str());
    } else {
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "No active player.");
    }

    ImGui::Separator();
    ImGui::Text("Spawn items at player position:");
    if (state.m_player) {
        // Each spawner queues a factory rather than pushing into m_entities here:
        // render() must not touch the entity list the physics pass owns.
        auto spawner = [this](auto makeItem) {
            queue([makeItem](PlayingState& s) {
                if (!s.m_player) return;
                const sf::Vector2f pos = s.m_player->getPosition() + sf::Vector2f(0.0f, -64.0f);
                auto item = makeItem(pos);
                s.wireEntityAnimations(item.get());
                s.m_entities.push_back(std::move(item));
            });
        };

        if (ImGui::Button("Spawn Mushroom"))
            spawner([](sf::Vector2f p) { return std::unique_ptr<Entity>(new Mushroom(p)); });
        ImGui::SameLine();
        if (ImGui::Button("Spawn Fire Flower"))
            spawner([](sf::Vector2f p) { return std::unique_ptr<Entity>(new FireFlower(p)); });
        ImGui::SameLine();
        if (ImGui::Button("Spawn Coin"))
            spawner([](sf::Vector2f p) { return std::unique_ptr<Entity>(new Coin(p)); });

        if (ImGui::Button("Spawn Star"))
            spawner([](sf::Vector2f p) { return std::unique_ptr<Entity>(new Star(p)); });
        ImGui::SameLine();
        if (ImGui::Button("Spawn Cape Feather"))
            spawner([](sf::Vector2f p) { return std::unique_ptr<Entity>(new CapeFeather(p)); });
        ImGui::SameLine();
        if (ImGui::Button("Spawn Mega Mushroom"))
            spawner([](sf::Vector2f p) { return std::unique_ptr<Entity>(new MegaMushroom(p)); });

        if (ImGui::Button("Spawn Mini Mushroom"))
            spawner([](sf::Vector2f p) { return std::unique_ptr<Entity>(new MiniMushroom(p)); });
    }

    ImGui::Separator();
    if (ImGui::Button("Swap Bricks <-> Coins (P-Switch test)")) {
        queue([](PlayingState& s) { s.m_tileMap.swapBricksAndCoins(); });
    }

    ImGui::Separator();
    ImGui::Checkbox("Show Physics AABB Overlays", &m_showAABB);
    ImGui::Checkbox("Show AI Debug Overlay", &m_showAiOverlay);

    ImGui::Separator();
    if (ImGui::Button("Reset Simulation")) {
        queue([](PlayingState& s) { s.setupTestScene(); });
    }

    ImGui::End();
}

// ---------------------------------------------------------------------------

void DevPanel::drawPersistencePanel(PlayingState& state) {
    ImGui::SetNextWindowPos(ImVec2(376.0f, 320.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowCollapsed(true, ImGuiCond_FirstUseEver);
    ImGui::Begin("Save/Load & Persistence (Phase 8)");

    Player* player = state.m_player;
    if (player) {
        ImGui::Text("Active Slot: %d", Game::getInstance().getActiveSlot());
        ImGui::Text("Active Character: %s", player->getCharacterName().c_str());
        ImGui::Text("Lives: %d | Coins: %d | Score: %d",
                    player->getLives(), player->getCoins(), player->getScore());
        ImGui::Text("Position: (%.1f, %.1f)", player->getPosition().x, player->getPosition().y);
    } else {
        ImGui::Text("No active player character loaded.");
    }

    ImGui::Separator();

    if (ImGui::CollapsingHeader("Settings Configuration")) {
        // Volume changes are audible immediately and touch no simulation state,
        // so they stay direct.
        float sfx = Game::getInstance().getSfxVolume();
        float music = Game::getInstance().getMusicVolume();
        bool colorblind = Game::getInstance().getColorblindMode();

        if (ImGui::SliderFloat("SFX Volume", &sfx, 0.0f, 100.0f))     Game::getInstance().setSfxVolume(sfx);
        if (ImGui::SliderFloat("Music Volume", &music, 0.0f, 100.0f)) Game::getInstance().setMusicVolume(music);
        if (ImGui::Checkbox("Colorblind Mode", &colorblind))          Game::getInstance().setColorblindMode(colorblind);

        const std::string diff = Game::getInstance().getDifficulty();
        const char* difficulties[] = { "easy", "normal", "hard" };
        int activeDiffIdx = 0;
        for (int i = 0; i < 3; ++i) {
            if (diff == difficulties[i]) activeDiffIdx = i;
        }
        if (ImGui::Combo("Difficulty", &activeDiffIdx, difficulties, 3)) {
            Game::getInstance().setDifficulty(difficulties[activeDiffIdx]);
        }
    }

    if (ImGui::CollapsingHeader("Save/Load Slots")) {
        for (int slot = 1; slot <= 3; ++slot) {
            ImGui::PushID(slot);
            const SaveSlotPreview preview = Serializer::getSlotPreview(slot);
            if (preview.exists) {
                ImGui::Text("Slot %d: [%s] Lvl:%d (%s) Score:%d, Star Coins:%d, Play Time:%.1fs, Saved:%s",
                            slot, preview.character.c_str(), preview.levelId, preview.levelName.c_str(),
                            preview.score, preview.starCoinsCount, preview.playTime, preview.timestamp.c_str());
            } else {
                ImGui::Text("Slot %d: Empty", slot);
            }

            if (ImGui::Button("Save")) {
                queue([slot](PlayingState& s) { s.saveToSlot(slot); });
            }
            ImGui::SameLine();
            if (ImGui::Button("Load")) {
                queue([slot](PlayingState& s) { s.loadFromSlot(slot); });
            }
            ImGui::SameLine();
            if (ImGui::Button("Delete")) {
                queue([slot](PlayingState&) { Serializer::deleteSlot(slot); });
            }
            ImGui::Separator();
            ImGui::PopID();
        }
    }

    if (ImGui::CollapsingHeader("Session & Overall Statistics")) {
        const auto& stats = StatisticsTracker::getInstance().getStats();
        ImGui::Text("Total Enemies Defeated: %d", stats.totalEnemiesDefeated);
        ImGui::Text("Total Coins Collected: %d", stats.totalCoinsCollected);
        ImGui::Text("Total Deaths: %d", stats.totalDeaths);
        ImGui::Text("Total Time Played: %.1fs", stats.totalTimePlayed);
        ImGui::Text("Highest Combo: %d", stats.highestCombo);
        if (ImGui::Button("Reset Stats")) {
            queue([](PlayingState&) { StatisticsTracker::getInstance().reset(); });
        }
    }

    if (ImGui::CollapsingHeader("Achievements Monitor")) {
        const auto& achievements = AchievementManager::getInstance().getAchievements();
        int unlockedCount = 0;
        for (const auto& a : achievements) {
            if (a.unlocked) ++unlockedCount;
        }
        ImGui::Text("Unlocked: %d / %d", unlockedCount, static_cast<int>(achievements.size()));
        ImGui::Separator();
        for (const auto& a : achievements) {
            ImGui::Text("[%s] %s: %s (%s)",
                        (a.unlocked ? "UNLOCKED" : "LOCKED"),
                        a.name.c_str(), a.condition.c_str(), a.icon.c_str());
        }
    }

    ImGui::Separator();
    if (ImGui::Button("Reset Game Data (Lock all & wipe slot 1)")) {
        queue([](PlayingState&) {
            AchievementManager::getInstance().reset();
            StatisticsTracker::getInstance().reset();
            Serializer::deleteSlot(1);
        });
    }

    ImGui::TextDisabled("\nSimulation Keyboard Testing Commands:\n"
                        "- Num1: Collect coin     - Num2: Defeat enemy\n"
                        "- Num3: Take damage      - Num4: Lose life/Die\n"
                        "- Num5: Cross Checkpoint - Num6: Clear Level 3\n"
                        "- Num7: Defeat Bowser    - Num8: Collect star coin\n"
                        "- Num9: Find hidden block");

    ImGui::End();
}
