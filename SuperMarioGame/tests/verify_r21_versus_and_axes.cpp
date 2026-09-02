// verify_r21_versus_and_axes.cpp — three defects that only exist on a LIVE
// PlayingState, so all three are driven against one.
//
//  1. In a two-player match, eliminating one player FROZE the camera. The
//     dispatch in update() chose updateVersusCamera() whenever m_player2 was
//     non-null, and that method returns at its first line unless BOTH players
//     are present — so after Player 1 was knocked out the view never moved
//     again and the survivor simply ran out of frame. It was not "fails to
//     focus Player 2"; it focused nobody.
//
//  2. The two HUD badges behaved differently and neither said "out". Player 1's
//     badge is drawn unconditionally while its data stopped being refreshed
//     (Game::getPlayer() goes null, so update() fell through to the test-scene
//     mock values), and Player 2's whole HUD block was gated on the live
//     m_player2 pointer, so that badge vanished. The user asked for a dead
//     player to be SHOWN as dead.
//
//  3. Bowser's bridge now carries one axe per difficulty tier — EASY 1,
//     NORMAL 2, HARD 3 — and EVERY axe has to be reached before the bridge
//     drops. More axes is therefore harder, not easier.
//
// Assertions are made against what the running state actually produced: the
// real camera, the real HudData the real Hud was handed, the real tilemap after
// a real BridgeAxe::activate(). A harness that composed its own HudData would
// pass while the game still drew a stale badge, which is the defect.
//
// Run via:  ctest -R r21_versus_and_axes --output-on-failure

#include "Core/EventBus.hpp"
#include "Core/Game.hpp"
#include "Core/GameMode.hpp"
#include "Core/PlayingState.hpp"
#include "Entities/Boss.hpp"
#include "Entities/BridgeAxe.hpp"
#include "Entities/Player.hpp"
#include "Graphics/Camera.hpp"
#include "Graphics/Hud.hpp"
#include "Utils/Constants.hpp"
#include "Utils/LevelCatalog.hpp"
#include "Utils/MapGenerator.hpp"
#include "Utils/TileMap.hpp"

#include "TestSaveSandbox.hpp"

#include <imgui.h>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>

// Declared in the global namespace to match the `friend class
// VersusAndAxesTestHooks;` declarations in PlayingState.hpp and Hud.hpp.
class VersusAndAxesTestHooks {
public:
    static Player* playerOne(const PlayingState& state) { return state.m_player; }
    static Player* playerTwo(const PlayingState& state) { return state.m_player2; }
    static Boss*   boss(const PlayingState& state)      { return state.m_activeBoss; }

    static sf::Vector2f cameraCentre(const PlayingState& state) {
        return state.m_camera.getPosition();
    }

    // The real entry point the hazard checks use, so the elimination that
    // follows is the one the game performs rather than a hand-forged one.
    static void kill(PlayingState& state, Player* who) {
        state.killPlayer(who, "harness");
    }

    // What the HUD was actually handed on the last completed frame.
    static const HudData& hudData(const PlayingState& state) {
        return state.m_hud->m_curData;
    }
    static bool hasHud(const PlayingState& state) { return state.m_hud != nullptr; }

    static int axesTotal(const PlayingState& state)     { return state.m_axesTotal; }
    static int axesRemaining(const PlayingState& state) { return state.m_axesRemaining; }

    static const TileMap& tiles(const PlayingState& state) { return state.m_tileMap; }

    // Every axe still in the level, left to right.
    static std::vector<BridgeAxe*> axes(const PlayingState& state) {
        std::vector<BridgeAxe*> found;
        for (const auto& entity : state.m_entities) {
            auto* axe = dynamic_cast<BridgeAxe*>(entity.get());
            if (axe && axe->isActive()) found.push_back(axe);
        }
        std::sort(found.begin(), found.end(), [](const BridgeAxe* a, const BridgeAxe* b) {
            return a->getPosition().x < b->getPosition().x;
        });
        return found;
    }
};

namespace {

int g_failures = 0;
int g_checks   = 0;

void check(bool condition, const std::string& what) {
    ++g_checks;
    if (condition) {
        std::cout << "  [ ok ] " << what << "\n";
    } else {
        std::cout << "  [FAIL] " << what << "\n";
        ++g_failures;
    }
}

void section(const std::string& title) {
    std::cout << "\n=== " << title << " ===\n";
}

constexpr float FRAME = 1.0f / 60.0f;

void tick(PlayingState& state, int frames) {
    for (int i = 0; i < frames; ++i) state.update(FRAME);
}

// Runs the death sequence out until the loser has actually left the level.
// DEATH_FALL_SECONDS is 1.6s, so 200 frames is comfortably more than enough
// and the loop stops the moment the pointer is dropped.
bool runEliminationToCompletion(PlayingState& state, bool killPlayerTwo) {
    for (int i = 0; i < 240; ++i) {
        state.update(FRAME);
        const bool gone = killPlayerTwo ? (VersusAndAxesTestHooks::playerTwo(state) == nullptr)
                                        : (VersusAndAxesTestHooks::playerOne(state) == nullptr);
        if (gone) {
            // One more frame, so the HUD sync at the end of a full update()
            // has run at least once since the pointer was dropped. Without it
            // the badge fields would be read from before the elimination and
            // the test could not tell a fix from the bug.
            state.update(FRAME);
            return true;
        }
    }
    return false;
}

// The bridge, as chopBridge() itself defines it: every solid tile in an arena
// column that has lava underneath. Counted rather than named, so this agrees
// with the production code by construction.
int bridgeTilesInArena(const PlayingState& state) {
    const TileMap& map = VersusAndAxesTestHooks::tiles(state);
    Boss* boss = VersusAndAxesTestHooks::boss(state);
    if (!boss || !boss->hasArena()) return -1;
    const AABB arena = boss->getArena();

    const int firstX = std::max(0, static_cast<int>(arena.x / Constants::TILE_SIZE));
    const int lastX  = std::min(map.getWidth() - 1,
                                static_cast<int>((arena.x + arena.width) / Constants::TILE_SIZE));
    int solidOverLava = 0;
    for (int x = firstX; x <= lastX; ++x) {
        int topLavaRow = -1;
        for (int y = 0; y < map.getHeight(); ++y) {
            if (map.getTileType(x, y) == TileType::Lava) { topLavaRow = y; break; }
        }
        if (topLavaRow < 0) continue;
        for (int y = 0; y < topLavaRow; ++y) {
            if (TileMap::getInfo(map.getTileType(x, y)).isSolid) ++solidOverLava;
        }
    }
    return solidOverLava;
}

// Level 1-3 is index 2 in the campaign catalogue; asserted rather than assumed,
// because every axe check below reads that level's data.
int bowserLevelIndex() {
    for (int i = 0; i < LevelCatalog::count(); ++i) {
        if (LevelCatalog::pathFor(i).find("level_3.json") != std::string::npos) return i;
    }
    return -1;
}

// ---------------------------------------------------------------------------
// 1. The frozen camera.
// ---------------------------------------------------------------------------
void testTheCameraFollowsTheSurvivor() {
    section("versus  the camera follows whoever is left after an elimination");

    Game::getInstance().setDifficulty("normal");
    PlayingState state(false, false, MapGeneratorConfig(), 0, 0,
                       MatchConfig{GameMode::VersusHuman});
    state.enter();

    Player* one = VersusAndAxesTestHooks::playerOne(state);
    Player* two = VersusAndAxesTestHooks::playerTwo(state);
    check(one != nullptr && two != nullptr, "a versus match starts with two players");
    if (!one || !two) { state.exit(); return; }

    // The survivor needs lives to spare: a second death mid-test would end the
    // run and the camera would have nobody to follow for a legitimate reason.
    two->restoreStats(9, 0, 0);
    // And Player 1 needs to be on their last, so the next death eliminates
    // rather than respawning them.
    one->restoreStats(1, 0, 0);

    VersusAndAxesTestHooks::kill(state, one);
    check(runEliminationToCompletion(state, /*killPlayerTwo=*/false),
          "Player 1's death sequence runs to elimination");
    check(VersusAndAxesTestHooks::playerOne(state) == nullptr,
          "the eliminated player has left the level");
    two = VersusAndAxesTestHooks::playerTwo(state);
    check(two != nullptr, "and the survivor is still in it");
    if (!two) { state.exit(); return; }

    // Now walk the survivor away and watch what the view does. Walking rather
    // than teleporting: a teleport can drop them into terrain that was not
    // there at the spawn, and the defect is about a view that does not track a
    // MOVING player.
    const float cameraBefore = VersusAndAxesTestHooks::cameraCentre(state).x;
    const float playerBefore = two->getPosition().x;
    for (int i = 0; i < 180; ++i) {
        two = VersusAndAxesTestHooks::playerTwo(state);
        if (!two) break;
        two->moveRight();
        state.update(FRAME);
    }
    two = VersusAndAxesTestHooks::playerTwo(state);
    check(two != nullptr, "the survivor survived three seconds of walking");
    if (!two) { state.exit(); return; }

    const float cameraAfter = VersusAndAxesTestHooks::cameraCentre(state).x;
    const float playerAfter = two->getPosition().x;

    check(playerAfter > playerBefore + 100.0f,
          "the survivor actually travelled, so there was something to follow");
    // The bug: the camera sat exactly where updateVersusCamera() last left it.
    check(cameraAfter > cameraBefore + 100.0f,
          "the camera moved after the elimination instead of freezing");
    // And it is framing the survivor, not merely drifting. Three tiles is the
    // slack the lookahead and the follow lag get, chosen from measured numbers
    // rather than taste: the fixed build settles at a ~34px gap, the
    // frozen-camera build at ~156px. A half-screen bound (640px) passed against
    // BOTH and was therefore no guard at all. The [diag] line is printed so the
    // next person can re-derive the bound instead of guessing at it.
    std::cout << "    [diag] camera moved " << (cameraAfter - cameraBefore)
              << "px, player moved " << (playerAfter - playerBefore)
              << "px, gap " << std::abs(cameraAfter - playerAfter) << "px\n";
    check(std::abs(cameraAfter - playerAfter) < 3.0f * Constants::TILE_SIZE,
          "the survivor is inside the frame the camera settled on");

    state.exit();
}

// ---------------------------------------------------------------------------
// 2. The badges.
// ---------------------------------------------------------------------------
void testAnEliminatedPlayerSaysSoOnTheHud() {
    section("versus  an eliminated player's badge reports OUT with the right character");

    Game::getInstance().setDifficulty("normal");

    // --- Player 1 eliminated: the badge used to go stale ---------------------
    {
        PlayingState state(false, false, MapGeneratorConfig(), 0, 0,
                           MatchConfig{GameMode::VersusHuman});
        state.enter();
        Player* one = VersusAndAxesTestHooks::playerOne(state);
        Player* two = VersusAndAxesTestHooks::playerTwo(state);
        check(one != nullptr && two != nullptr && VersusAndAxesTestHooks::hasHud(state),
              "two players and a HUD");
        if (!one || !two) { state.exit(); return; }

        const std::string oneName = one->getCharacterName();
        const std::string twoName = two->getCharacterName();
        check(oneName != twoName, "the two players are different characters, so the badge can be wrong");

        two->restoreStats(9, 0, 0);
        one->restoreStats(1, 7, 4200);
        VersusAndAxesTestHooks::kill(state, one);
        check(runEliminationToCompletion(state, false), "Player 1 is eliminated");

        const HudData& hud = VersusAndAxesTestHooks::hudData(state);
        check(hud.eliminated, "Player 1's badge is marked eliminated");
        check(hud.characterName == oneName,
              "and still names the character who was eliminated, not a default");
        check(hud.lives == 0, "with no lives left rather than a frozen count");
        // setupTestScene()'s mock fallback is 102520/57/9 — the exact values the
        // badge used to freeze on for a real run once getPlayer() went null.
        check(hud.score != 102520 && hud.coins != 57,
              "and NOT the test-scene mock values the stale badge showed");
        check(hud.hasSecondPlayer && !hud.secondEliminated,
              "the survivor's badge is present and not marked out");
        check(hud.secondCharacterName == twoName, "and names the survivor");
        state.exit();
    }

    // --- Player 2 eliminated: the badge used to disappear --------------------
    {
        PlayingState state(false, false, MapGeneratorConfig(), 0, 0,
                           MatchConfig{GameMode::VersusHuman});
        state.enter();
        Player* one = VersusAndAxesTestHooks::playerOne(state);
        Player* two = VersusAndAxesTestHooks::playerTwo(state);
        if (!one || !two) { check(false, "two players and a HUD"); state.exit(); return; }

        const std::string twoName = two->getCharacterName();
        one->restoreStats(9, 0, 0);
        two->restoreStats(1, 3, 900);
        VersusAndAxesTestHooks::kill(state, two);
        check(runEliminationToCompletion(state, /*killPlayerTwo=*/true),
              "Player 2 is eliminated");

        const HudData& hud = VersusAndAxesTestHooks::hudData(state);
        check(hud.hasSecondPlayer,
              "Player 2's badge stays on screen instead of vanishing with the pointer");
        check(hud.secondEliminated, "and is marked eliminated");
        check(hud.secondCharacterName == twoName,
              "and still names Player 2's character, which no live object could answer for");
        check(hud.secondLives == 0, "with no lives left");
        check(!hud.eliminated, "the survivor is not marked out");
        state.exit();
    }
}

// ---------------------------------------------------------------------------
// 3. The axes.
// ---------------------------------------------------------------------------
void testAxeCountScalesWithDifficulty() {
    section("bowser  the axe count is EASY 1 / NORMAL 2 / HARD 3");

    const int levelIndex = bowserLevelIndex();
    check(levelIndex >= 0, "the campaign catalogue still contains level_3.json");
    if (levelIndex < 0) return;

    const struct { const char* tier; int expected; } cases[] = {
        {"easy", 1}, {"normal", 2}, {"hard", 3}
    };

    for (const auto& one : cases) {
        Game::getInstance().setDifficulty(one.tier);
        PlayingState state(false, false, MapGeneratorConfig(), 0, levelIndex);
        state.enter();

        const auto axes = VersusAndAxesTestHooks::axes(state);
        check(static_cast<int>(axes.size()) == one.expected,
              std::string(one.tier) + ": " + std::to_string(one.expected) +
              " axe(s) in the level (found " + std::to_string(axes.size()) + ")");
        check(VersusAndAxesTestHooks::axesTotal(state) == one.expected,
              std::string(one.tier) + ": the counter agrees with the world");
        check(VersusAndAxesTestHooks::axesRemaining(state) == one.expected,
              std::string(one.tier) + ": none reached yet");

        // Every axe that survived has to be somewhere a player can stand.
        Boss* boss = VersusAndAxesTestHooks::boss(state);
        check(boss != nullptr && boss->hasArena(),
              std::string(one.tier) + ": the level still has a boss with an arena");
        if (boss && boss->hasArena()) {
            const AABB arena = boss->getArena();
            const TileMap& map = VersusAndAxesTestHooks::tiles(state);
            bool allReachable = true;
            for (const BridgeAxe* axe : axes) {
                const sf::Vector2f at = axe->getPosition();
                const int gx = static_cast<int>(at.x / Constants::TILE_SIZE);
                const int gy = static_cast<int>(at.y / Constants::TILE_SIZE);
                const bool insideArena = at.x >= arena.x && at.x < arena.x + arena.width;
                // A floor directly under it (a BridgeAxe is an Item with the
                // default gravity multiplier — one placed in mid-air over the
                // trench would fall into the lava before the player arrived),
                // and its own cell clear so it is not buried in brick.
                const bool standsOnSomething =
                    TileMap::getInfo(map.getTileType(gx, gy + 1)).isSolid;
                const bool notBuried = !TileMap::getInfo(map.getTileType(gx, gy)).isSolid;
                if (!insideArena || !standsOnSomething || !notBuried) allReachable = false;
            }
            check(allReachable,
                  std::string(one.tier) + ": every axe is inside the arena, "
                  "clear of solid tiles and standing on a floor");
        }
        state.exit();
    }

    // The generator has to follow the same rule, or a procedural Bowser is a
    // different fight from the authored one.
    Game::getInstance().setDifficulty("hard");
    {
        MapGeneratorConfig config;
        config.bossArena = true;
        config.width = 100;
        config.height = 23;
        config.seed = 24680;
        TileMap map;
        std::vector<std::unique_ptr<Entity>> entities;
        MapGenerator::generate(map, entities, config);
        int generated = 0;
        for (const auto& entity : entities) {
            if (dynamic_cast<BridgeAxe*>(entity.get())) ++generated;
        }
        check(generated == 3,
              "MapGenerator lays out three axes for configureBridgeAxes() to prune");
    }
}

void testTheBridgeWaitsForTheLastAxe() {
    section("bowser  the bridge drops only when every axe has been reached");

    const int levelIndex = bowserLevelIndex();
    if (levelIndex < 0) { check(false, "level_3.json is in the catalogue"); return; }

    Game::getInstance().setDifficulty("hard");
    PlayingState state(false, false, MapGeneratorConfig(), 0, levelIndex);
    state.enter();

    Player* player = VersusAndAxesTestHooks::playerOne(state);
    check(player != nullptr, "the level has a player to hand the axes to");
    auto axes = VersusAndAxesTestHooks::axes(state);
    check(axes.size() == 3, "three axes on Hard");
    if (!player || axes.size() != 3) { state.exit(); return; }

    const int bridgeAtStart = bridgeTilesInArena(state);
    check(bridgeAtStart > 0, "there is a bridge over the lava to begin with");

    // Reached through BridgeAxe::activate(), not by publishing the event by
    // hand: that is the path a player's touch takes, and it is what proves the
    // axe is still wired to the handler at all.
    axes[0]->activate(*player);
    check(bridgeTilesInArena(state) == bridgeAtStart,
          "the first axe does NOT drop the bridge");
    check(VersusAndAxesTestHooks::axesRemaining(state) == 2, "two still to reach");

    axes[1]->activate(*player);
    check(bridgeTilesInArena(state) == bridgeAtStart,
          "nor does the second");
    check(VersusAndAxesTestHooks::axesRemaining(state) == 1, "one still to reach");

    Boss* boss = VersusAndAxesTestHooks::boss(state);
    check(boss != nullptr && !boss->isDefeated(),
          "and Bowser is still standing after two of the three");

    axes[2]->activate(*player);
    check(bridgeTilesInArena(state) == 0, "the LAST axe drops every bridge tile");
    check(VersusAndAxesTestHooks::axesRemaining(state) == 0, "and the counter is spent");
    // The chop no longer DELETES him, and that is the point of the change.
    //
    // This used to assert he was defeated on the same frame as the last axe,
    // which was true when chopBridge() called defeatNow() outright. He now
    // begins a lava death: the floor goes, gravity takes him off the stump, and
    // his health burns down one point at a time in the lava before routing
    // through that same defeatNow(). So the correct assertion is stronger than
    // the old one -- not "he is gone", but "he is visibly dying, and he does
    // then die".
    boss = VersusAndAxesTestHooks::boss(state);
    check(boss != nullptr && !boss->isDefeated() && boss->isDyingInLava(),
          "the last axe starts Bowser's lava death rather than deleting him");

    // LAVA_DRAIN_SECONDS is 2.4s and the fall precedes it, so 5s of frames is
    // comfortably past the end without pinning the test to the exact timing.
    tick(state, 300);
    boss = VersusAndAxesTestHooks::boss(state);
    check(boss == nullptr || boss->isDefeated(),
          "and the drain finishes in the same defeat the single-axe route reached");

    state.exit();
}

// The HUD has to be able to SAY where the player is on that route: an axe that
// silently does nothing is indistinguishable from a broken one.
void testTheHudReportsAxeProgress() {
    section("bowser  the HUD carries the axe tally");

    const int levelIndex = bowserLevelIndex();
    if (levelIndex < 0) { check(false, "level_3.json is in the catalogue"); return; }

    Game::getInstance().setDifficulty("hard");
    PlayingState state(false, false, MapGeneratorConfig(), 0, levelIndex);
    state.enter();

    Player* player = VersusAndAxesTestHooks::playerOne(state);
    auto axes = VersusAndAxesTestHooks::axes(state);
    if (!player || axes.empty()) { check(false, "a player and some axes"); state.exit(); return; }

    axes[0]->activate(*player);
    // The boss HUD block only reports while the arena is locked, so drive the
    // player into it first — the tally is a fight-time readout, not a permanent
    // one.
    Boss* boss = VersusAndAxesTestHooks::boss(state);
    if (boss && boss->hasArena()) {
        const AABB arena = boss->getArena();
        player->setPosition({arena.x + 3.0f * Constants::TILE_SIZE,
                             player->getPosition().y});
        tick(state, 4);
    }

    const HudData& hud = VersusAndAxesTestHooks::hudData(state);
    check(hud.bossActive, "the fight is live, so the boss HUD is up");
    check(hud.bossAxesTotal == 3, "the HUD knows this fight has three axes");
    check(hud.bossAxesReached == 1, "and that one of them has been reached");

    state.exit();
}

} // namespace

int main() {
    TestSaveSandbox sandbox("r21_versus_and_axes");

    std::cout << "=========================================\n";
    std::cout << " R21 — versus survivor camera, elimination badges, bridge axes\n";
    std::cout << "=========================================\n";

    // PlayingState::update() asks ImGui::GetIO() whether a text field owns the
    // keyboard before handing the keys to the player, and ImGui asserts on a
    // missing context. A bare context is enough and needs no window or GL —
    // the same trick verify_r21_debug_cheats uses to drive a real level
    // headlessly, and for the same reason.
    ImGui::CreateContext();
    ImGui::GetIO().DisplaySize = ImVec2(static_cast<float>(Constants::WINDOW_WIDTH),
                                       static_cast<float>(Constants::WINDOW_HEIGHT));

    testTheCameraFollowsTheSurvivor();
    testAnEliminatedPlayerSaysSoOnTheHud();
    testAxeCountScalesWithDifficulty();
    testTheBridgeWaitsForTheLastAxe();
    testTheHudReportsAxeProgress();

    // Leave the singleton on the tier the rest of a ctest process expects.
    Game::getInstance().setDifficulty("normal");
    ImGui::DestroyContext();

    std::cout << "\n----------------------------------------\n";
    std::cout << g_checks - g_failures << " / " << g_checks << " checks passed\n";
    if (g_failures > 0) {
        std::cout << g_failures << " FAILURE(S)\n";
        return 1;
    }
    std::cout << "ALL PASS\n";
    return 0;
}
