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
#include "Utils/MapGenerator.hpp"

#include <imgui.h>

#include <cfloat>
#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <vector>

namespace {

// Shared by both level dropdowns so they cannot drift apart — they were two
// identical literal arrays before.
const char* const kCampaignLevels[] = {
    "World 1-1: Grassland Overworld",
    "World 1-1 Sub: Underground Vault",
    "World 1-2: Ice Cavern Path",
    "World 1-2 Sub: Sky Platform Canopy",
    "World 1-3: Bowser's Castle Fortress",
    "World 1-3 Sub: Secret Castle Vault",
    "Bonus Stage 1: Coin Paradise"
};
constexpr int kLevelCount = 7;

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
        drawPlaygroundPanel(state);
        drawPersistencePanel(state);
        drawAchievementToasts();
        if (m_showAiOverlay) {
            drawAiOverlay(state);
        }
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
    if (ImGui::Combo("##SelectLevel", &levelIdx, kCampaignLevels, kLevelCount)) {
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
    if (ImGui::Combo("Level", &levelIdx, kCampaignLevels, kLevelCount)) {
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

// ---------------------------------------------------------------------------

void DevPanel::drawAchievementToasts() {
    const auto& toasts = AchievementManager::getInstance().getActiveToasts();
    if (toasts.empty()) return;

    ImGui::SetNextWindowPos(ImVec2(Constants::WINDOW_WIDTH - 320.0f, 20.0f), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(300.0f, 80.0f * toasts.size()), ImGuiCond_Always);
    ImGui::Begin("Achievements Toasts Overlay", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
                 ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
                 ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoBackground);

    for (const auto& toast : toasts) {
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, toast.alpha);
        ImGui::BeginChild(toast.id.c_str(), ImVec2(290, 70), true);
        ImGui::TextColored(ImVec4(1.0f, 0.84f, 0.0f, 1.0f), "Achievement Unlocked!");
        ImGui::Text("[%s] %s", toast.icon.c_str(), toast.name.c_str());
        ImGui::EndChild();
        ImGui::PopStyleVar();
    }
    ImGui::End();
}
