// Submission sweep, Phase 2 lane 2B (plan §6, row 2B): two items.
//
//  1. The plan reported level_3.json's third HARD-difficulty bridge axe (tile
//     190,19) as unreachable "behind" the boss arena's right enclosure post
//     (189,19), which would matter because configureBridgeAxes() requires
//     EVERY axe to be reached before Bowser's bridge drops on Hard
//     (axeQuotaForDifficulty() == 3, and the BridgeChopped handler only calls
//     chopBridge() once m_axesRemaining <= 1).
//
//     Investigated rather than taken on faith, per this lane's brief: driving
//     a real Player through real PlayingState::update() frames (not a
//     hand-rolled model of the physics) shows the axe IS reached by ordinary
//     play — "hold Right, mash Jump" clears the one-tile post and lands
//     squarely on it, at both walk and run speed, with the post either
//     present or (tried during investigation) removed entirely. What
//     genuinely blocks a crossing that never jumps at all is Bowser's own
//     patrol on the bridge — by design, per BridgeAxe.hpp: "you do not have
//     to beat Bowser, you have to get past him" — which is a combat-timing
//     concern, not a level-data defect, and is already this lane's boundary
//     to leave alone (AGENTS.md restricts this lane to level_3.json and
//     MapGenerator.cpp/.hpp; Boss.cpp/PlayingState.cpp belong to other
//     lanes). No change was made to level_3.json's axe or post: there was no
//     reproducible defect there to fix, and rule g-rule-2 (zero superficial
//     fixes) cuts against editing level data to satisfy a claim the physics
//     did not bear out. This is flagged plainly in the session log instead of
//     silently shipped as "fixed".
//
//     What ships here instead is a protective regression guard: a real
//     physics crossing confirming the third axe (found dynamically as the
//     rightmost of the three, never hard-coded) stays reachable by ordinary
//     "hold Right, mash Jump" play. It is mutation-tested the other
//     direction too — see the log — by temporarily moving the axe onto the
//     post's own solid tile, which the guard does catch.
//
//  2. MapGenerator::generateSubLevel()'s exit pipe — the only way out of a
//     procedurally generated bonus vault — never called setEntryMode(), so it
//     kept the Pipe default of EntryMode::Top. Every hand-authored sub-level
//     (level_1_sub.json, level_2_sub.json, level_3_sub.json) authors this
//     exact pipe as `side_left`: SPEC's up/down pipe pairing is that going
//     DOWN into a vault is a Top entry (stand on the rim, press Down) and
//     coming back UP out of one is a side entry (the L-bend art is the only
//     art in the game that reads as "this shaft goes somewhere else"). A
//     generated vault's exit therefore looked and behaved like a second
//     entrance instead of the way out — inconsistent with every authored
//     level using the same generator code path (MapGenerator.cpp's own
//     comment: "the same rule and the same code path level_3.json goes
//     through").
//
// Run via:  ctest -R verify_hard_axe_and_vault_pipe --output-on-failure

#include "Core/Game.hpp"
#include "Core/PlayingState.hpp"
#include "Entities/Boss.hpp"
#include "Entities/BridgeAxe.hpp"
#include "Entities/Pipe.hpp"
#include "Entities/Player.hpp"
#include "Utils/Constants.hpp"
#include "Utils/LevelCatalog.hpp"
#include "Utils/MapGenerator.hpp"
#include "Utils/TileMap.hpp"

#include "TestSaveSandbox.hpp"

#include <imgui.h>

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

// Declared in the global namespace to match the `friend class
// VersusAndAxesTestHooks;` declarations already in PlayingState.hpp and
// Hud.hpp. Every private member reached below is one that grant already
// covers (tests/verify_r21_versus_and_axes.cpp reaches the identical set for
// the identical reason: a live PlayingState is the only thing that can prove
// these two defects, and none of this is worth a public getter that only a
// test would ever call). Defining the class again here is not an ODR
// violation — each verify_* target is its own executable, compiled and linked
// on its own, so this translation unit's definition never meets that file's.
class VersusAndAxesTestHooks {
public:
    static Player* playerOne(const PlayingState& state) { return state.m_player; }
    static Boss*   boss(const PlayingState& state)      { return state.m_activeBoss; }

    // Every axe still in the level, left to right — collected axes are
    // destroyed and swept out of m_entities, so this list shrinks as the
    // simulation below reaches them.
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

    static int axesRemaining(const PlayingState& state) { return state.m_axesRemaining; }
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

// Level 1-3 is the campaign's only Bowser fight; asserted rather than assumed,
// because the axe check below reads that level's own shipped data.
int bowserLevelIndex() {
    for (int i = 0; i < LevelCatalog::count(); ++i) {
        if (LevelCatalog::pathFor(i).find("level_3.json") != std::string::npos) return i;
    }
    return -1;
}

// Drives a fresh Hard-difficulty level_3.json PlayingState, teleports the
// player onto the middle bridge axe, then holds Right and mashes Jump toward
// the third axe for up to `frames` ticks — the ordinary way a player crosses
// a bump they cannot see the top of (Character::jump() is a no-op unless
// grounded, so calling it every frame is "held down", not infinite jumps).
// Returns whether the third axe was actually collected.
bool attemptToCrossToTheThirdAxe(int frames) {
    Game::getInstance().setDifficulty("hard");
    const int levelIndex = bowserLevelIndex();
    if (levelIndex < 0) return false;

    PlayingState state(false, false, MapGeneratorConfig(), 0, levelIndex);
    state.enter();

    Player* player = VersusAndAxesTestHooks::playerOne(state);
    if (!player) { state.exit(); return false; }

    auto axes = VersusAndAxesTestHooks::axes(state);
    if (axes.size() != 3) { state.exit(); return false; }

    // Found, not assumed: the rightmost of the three is "the far ledge past
    // both posts" axe (PlayingState::configureBridgeAxes keeps this one on
    // every difficulty tier — it is the axe the arena was built around).
    // Recorded by VALUE, not by pointer: once collected, activate()/collect()
    // destroy()s the axe and PlayingState's end-of-frame sweep erases it from
    // m_entities, so a raw BridgeAxe* held past that point would dangle.
    const float thirdAxeX = axes.back()->getPosition().x;
    const float midAxeX   = axes[1]->getPosition().x;

    // Start from the middle axe rather than the level's own spawn: the middle
    // axe's own reachability is already covered by
    // verify_r21_versus_and_axes.cpp's "every axe is inside the arena, clear
    // of solid tiles and standing on a floor" check, and walking the entire
    // level from spawn to the arena would spend hundreds of frames proving
    // something else. A few tiles above it, so gravity settles the player
    // onto the walkway exactly the way arriving from the bridge would.
    player->setPosition({midAxeX, axes[1]->getPosition().y - 3.0f * Constants::TILE_SIZE});

    for (int i = 0; i < frames; ++i) {
        Player* p = VersusAndAxesTestHooks::playerOne(state);
        if (!p) { state.exit(); return false; }
        p->moveRight();
        p->jump();
        state.update(FRAME);
    }

    const auto axesAfter = VersusAndAxesTestHooks::axes(state);
    const bool thirdStillPresent = std::any_of(axesAfter.begin(), axesAfter.end(),
        [thirdAxeX](const BridgeAxe* a) {
            // A full tile of slack: an axe resting on solid ground can settle
            // a few pixels from its authored spot on the very first physics
            // frame (collision correction, gravity), and axes are placed many
            // tiles apart, so this cannot false-match a DIFFERENT axe.
            return std::abs(a->getPosition().x - thirdAxeX) < Constants::TILE_SIZE;
        });

    state.exit();
    Game::getInstance().setDifficulty("normal");
    return !thirdStillPresent;
}

// ---------------------------------------------------------------------------
// 1. The third HARD axe stays reachable by ordinary play. Protective guard —
//    see the file header for why no level_3.json change was made.
// ---------------------------------------------------------------------------
void testTheThirdHardAxeIsReachable() {
    section("bowser  the third Hard-difficulty axe is reachable by ordinary play");

    check(bowserLevelIndex() >= 0, "the campaign catalogue still contains level_3.json");

    // 4 seconds at 60fps: generously more than the crossing needs.
    check(attemptToCrossToTheThirdAxe(240),
          "hold Right, mash Jump reaches the third axe from the middle one");
}

// ---------------------------------------------------------------------------
// 2. MapGenerator's generated vault exit pipe matches the hand-authored
//    up/down pairing.
// ---------------------------------------------------------------------------

// Mirrors verify_r21_level_data.cpp's own helper of the same name and
// contract: the first solid row at or below `fromRow` in column `col`, or -1
// if the column has no floor. Duplicated rather than shared because that
// file's helper is file-local (anonymous namespace) and out of scope for this
// lane to touch.
int floorRowBelow(const TileMap& map, int col, int fromRow) {
    if (col < 0 || col >= map.getWidth()) return -1;
    for (int y = std::max(0, fromRow); y < map.getHeight(); ++y) {
        if (TileMap::getInfo(map.getTileType(col, y)).isSolid) return y;
    }
    return -1;
}

void testGeneratedVaultExitPipeIsASideEntryGoingUp() {
    section("mapgen  a generated vault's exit pipe is a side-entry pipe, like every authored one");

    TileMap subMap;
    std::vector<std::unique_ptr<Entity>> subEntities;
    const std::string returnLevel = "assets/levels/level_1.json";
    MapGenerator::generateSubLevel(subMap, subEntities, MapTheme::Overworld,
                                   MapDifficulty::Medium, returnLevel, {96.0f, 608.0f});

    // The exit pipe is the entrance pipe whose target is the level the vault
    // was entered from — the ONLY entity in a generated vault with that
    // target, so this identifies it without hard-coding its tile position.
    const Pipe* exitPipe = nullptr;
    for (const auto& entity : subEntities) {
        const auto* pipe = dynamic_cast<const Pipe*>(entity.get());
        if (pipe && pipe->isEntrance() && pipe->getTargetLevel() == returnLevel) {
            exitPipe = pipe;
            break;
        }
    }
    check(exitPipe != nullptr, "the generated vault has an exit pipe targeting the return level");
    if (!exitPipe) return;

    check(exitPipe->isSideEntry(),
          "the exit pipe is a side entry (an ascending pipe), not the Top-entry default");
    check(exitPipe->getEntryMode() == Pipe::EntryMode::SideLeft,
          "specifically side_left, matching every hand-authored sub-level's exit pipe");

    // The same geometric invariant verify_r21_level_data.cpp's
    // checkSideEntryMouths() holds every shipped side-entry pipe to: the
    // collider's foot sits FLUSH on the floor, not embedded a row into it —
    // a Top-entry pipe can be embedded (its rim just needs to stay within jump
    // reach), but a side-entry pipe's mouth is measured from the FOOT of the
    // collider, so embedding it buries the mouth's lower half in the ground.
    const AABB box = exitPipe->getBoundingBox();
    const int leftCol  = static_cast<int>(box.x / Constants::TILE_SIZE);
    const int rimRow   = static_cast<int>(box.y / Constants::TILE_SIZE);
    const int bottomRow = static_cast<int>((box.y + box.height) / Constants::TILE_SIZE);
    const int floorRow = floorRowBelow(subMap, leftCol, rimRow);
    check(floorRow >= 0, "the exit pipe's column has a floor");
    if (floorRow >= 0) {
        check(bottomRow == floorRow,
              "the exit pipe's foot sits flush on the floor (foot row " +
              std::to_string(bottomRow) + ", floor row " + std::to_string(floorRow) + ")");
    }

    // And the shaft has a ceiling to visibly leave through — the whole reason
    // the side-entry art exists (Pipe.hpp's EntryMode comment).
    const int rightCol = static_cast<int>((box.x + box.width - 1.0f) / Constants::TILE_SIZE);
    const int shaftCol = (exitPipe->getEntryMode() == Pipe::EntryMode::SideLeft) ? rightCol : leftCol;
    bool hasCeiling = false;
    for (int row = rimRow - 1; row >= 0; --row) {
        if (TileMap::getInfo(subMap.getTileType(shaftCol, row)).isSolid) { hasCeiling = true; break; }
    }
    check(hasCeiling, "the shaft column has a ceiling to leave through");
}

} // namespace

int main() {
    TestSaveSandbox sandbox("hard_axe_and_vault_pipe");

    std::cout << "=========================================\n";
    std::cout << " Lane 2B — the third Hard axe, and the generated vault's exit pipe\n";
    std::cout << "=========================================\n";

    // PlayingState::update() asks ImGui::GetIO() whether a text field owns the
    // keyboard before handing input to the player, and ImGui asserts on a
    // missing context — the same setup verify_r21_versus_and_axes.cpp uses to
    // drive a real level headlessly.
    ImGui::CreateContext();
    ImGui::GetIO().DisplaySize = ImVec2(static_cast<float>(Constants::WINDOW_WIDTH),
                                       static_cast<float>(Constants::WINDOW_HEIGHT));

    testTheThirdHardAxeIsReachable();
    testGeneratedVaultExitPipeIsASideEntryGoingUp();

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
