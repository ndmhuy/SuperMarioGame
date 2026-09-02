// verify_r21_pipes.cpp — the pipe art selector, and how a pipe is entered.
//
// Three classes of defect, each of which had shipped:
//
//  1. TWO ROW-TILERS. The tilemap composed its pipe runs from 16x16 halves
//     correctly while the Pipe entity stamped four quadrants at +-size*0.25,
//     which is only geometrically right for a SQUARE box. Growing the entity
//     collider from 2x2 to 2x4 therefore left the tilemap fine and the entity
//     path drawing a stripe with a blob at its base. The fix was to give both
//     paths one selector; these checks are what stops them splitting again.
//
//  2. A FIELD THE LOADER READS AND THE SAVER DROPS. Three of those were fixed
//     in the R21 batch, each having silently reset part of a level on its first
//     save. `entry` is a fourth field on the same entity, so it gets the
//     round-trip check the other three ended up needing.
//
//  3. ART THAT LIES ABOUT THE MECHANIC. `pipe_dark_green_long_l` is an L-BEND
//     whose mouth is at the BOTTOM-LEFT — picking it for a pipe the player is
//     supposed to stand on gave a warp pipe with no rim. The entry geometry is
//     asserted here directly, without a keyboard, via Pipe::isAtEntryPoint.
//
// Run via:  ctest -R r21_pipes --output-on-failure

#include "Entities/Mario.hpp"
#include "Entities/Pipe.hpp"
#include "Graphics/PipeRenderer.hpp"
#include "Graphics/TileMapRenderer.hpp"
#include "Utils/Constants.hpp"
#include "Utils/LevelLoader.hpp"
#include "Utils/SerializationUtils.hpp"
#include "Utils/TileMap.hpp"

#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace {

int g_failures = 0;
int g_checks   = 0;

void check(bool condition, const std::string& what) {
    ++g_checks;
    std::cout << (condition ? "  [ ok ] " : "  [FAIL] ") << what << "\n";
    if (!condition) ++g_failures;
}

void section(const std::string& title) {
    std::cout << "\n=== " << title << " ===\n";
}

std::string levelPath(const std::string& name) {
    const std::vector<std::string> roots = {
        "assets/levels/", "../assets/levels/", "../../assets/levels/",
        "SuperMarioGame/assets/levels/"
    };
    for (const std::string& root : roots) {
        if (std::filesystem::exists(root + name)) return root + name;
    }
    return "assets/levels/" + name;
}

// ---------------------------------------------------------------------------
// 1. One selector, and it can never yield half a rim.
// ---------------------------------------------------------------------------
void testCellArtNeverYieldsHalfARim() {
    section("R21J  the pipe art selector composes whole pipes only");

    // A 2-wide run — every warp pipe and every authored tilemap run — pairs off
    // into one whole pipe: left half then right half, rim row then body rows.
    const auto rimLeft   = PipeRenderer::cellArt(0, 2, true);
    const auto rimRight  = PipeRenderer::cellArt(1, 2, true);
    const auto bodyLeft  = PipeRenderer::cellArt(0, 2, false);
    const auto bodyRight = PipeRenderer::cellArt(1, 2, false);
    check(rimLeft.frame == "pipe_green_head_left",   "2-wide rim, column 0 is the left half of a rim");
    check(rimRight.frame == "pipe_green_head_right", "2-wide rim, column 1 is the right half");
    check(bodyLeft.frame == "pipe_green_body_left",  "2-wide body, column 0 is the left half of a body");
    check(bodyRight.frame == "pipe_green_body_right","2-wide body, column 1 is the right half");

    // A 1-wide run cannot be drawn from halves at all without showing half a
    // rim — the reported "half pipe" (R21-D1, after D22/D23) — so it comes from
    // whole 32px-wide art, sliced into a rim cell and a body cell.
    const auto lone    = PipeRenderer::cellArt(0, 1, true);
    const auto loneBody= PipeRenderer::cellArt(0, 1, false);
    check(lone.frame == "pipe_dark_green_long_up", "1-wide run uses whole narrow art, not a half");
    check(lone.sliceTop == 0 && lone.sliceHeight == 32, "1-wide rim cell slices the rim rows");
    check(loneBody.sliceTop == 32 && loneBody.sliceHeight == 32, "1-wide body cell slices the body rows");

    // An odd run's TRAILING column is the unpaired one; the pairs before it are
    // still halves.
    check(PipeRenderer::cellArt(0, 3, true).frame == "pipe_green_head_left",
          "3-wide run, column 0 is still a paired half");
    check(PipeRenderer::cellArt(1, 3, true).frame == "pipe_green_head_right",
          "3-wide run, column 1 is still a paired half");
    check(PipeRenderer::cellArt(2, 3, true).frame == "pipe_dark_green_long_up",
          "3-wide run, the unpaired trailing column uses whole narrow art");

    // No width and no row ever names a frame that is half a rim standing alone.
    bool anyLoneHalf = false;
    for (int width = 1; width <= 8; ++width) {
        for (int col = 0; col < width; ++col) {
            for (int rim = 0; rim <= 1; ++rim) {
                const auto art = PipeRenderer::cellArt(col, width, rim != 0);
                const bool isHalf = art.frame.find("_head_") != std::string::npos ||
                                    art.frame.find("_body_") != std::string::npos;
                // A half is only legitimate when the column it belongs to has a
                // partner: an even offset must have a column to its right, an
                // odd one a column to its left.
                const bool hasPartner = (col % 2 == 0) ? (col + 1 < width) : true;
                if (isHalf && !hasPartner) anyLoneHalf = true;
            }
        }
    }
    check(!anyLoneHalf, "no run width leaves a pipe half without its partner column");
}

// The two paths that used to be two row-tilers now answer identically.
void testTileMapAndEntityPathsAgree() {
    section("R21J  the tilemap path and the entity path share one selector");

    // A 2-wide, 3-tall pipe run in an otherwise empty map, exactly the shape a
    // warp pipe's collider is.
    TileMap map;
    map.initialize(10, 10);
    for (int y = 4; y <= 6; ++y) {
        map.setTile(5, y, TileType::Pipe);
        map.setTile(6, y, TileType::Pipe);
    }

    bool allAgree = true;
    for (int y = 4; y <= 6; ++y) {
        for (int x = 5; x <= 6; ++x) {
            const auto fromMap  = TileMapRenderer::pipeTileArtAt(map, x, y);
            const auto fromCell = PipeRenderer::cellArt(x - 5, 2, y == 4);
            if (fromMap.frame != fromCell.frame ||
                fromMap.sliceTop != fromCell.sliceTop ||
                fromMap.sliceHeight != fromCell.sliceHeight) {
                allAgree = false;
                std::cout << "        (" << x << "," << y << ") tilemap=" << fromMap.frame
                          << " entity=" << fromCell.frame << "\n";
            }
        }
    }
    check(allAgree, "every cell of a 2x3 run gets the same art from both callers");

    // And the geometry the entity path derives from a collider matches: a
    // 64x128 box is 2 cells by 4, so its top row is a rim and the rest are not.
    const int cols = static_cast<int>(Pipe::WIDTH_PX / Constants::TILE_SIZE);
    const int rows = static_cast<int>(Pipe::HEIGHT_PX / Constants::TILE_SIZE);
    check(cols == 2, "a warp pipe collider is two cells wide");
    check(rows == 4, "a warp pipe collider is four cells tall");
}

// ---------------------------------------------------------------------------
// 2. EntryMode round-trips through the level schema.
// ---------------------------------------------------------------------------
void testEntryModeNamesRoundTrip() {
    section("R21J  entry-mode names round-trip, and unknown names default to Top");

    const Pipe::EntryMode modes[] = {Pipe::EntryMode::Top,
                                     Pipe::EntryMode::SideLeft,
                                     Pipe::EntryMode::SideRight};
    for (Pipe::EntryMode mode : modes) {
        const std::string name = SerializationUtils::getPipeEntryModeName(mode);
        check(SerializationUtils::parsePipeEntryModeName(name) == mode,
              "'" + name + "' parses back to the mode it was written from");
    }
    // Every level authored before the field existed has no `entry` at all, and
    // a hand-edited one may have a typo. Both must stay playable.
    check(SerializationUtils::parsePipeEntryModeName("") == Pipe::EntryMode::Top,
          "a missing entry mode defaults to Top");
    check(SerializationUtils::parsePipeEntryModeName("sideways") == Pipe::EntryMode::Top,
          "an unrecognised entry mode defaults to Top");
}

void testEntryModeSurvivesASaveAndReload() {
    section("R21J  entry mode survives LevelLoader's save/load round trip");

    LevelLoader loader;
    TileMap map;
    LevelData data;
    if (!loader.loadLevel(levelPath("level_1_sub.json"), map, data)) {
        check(false, "level_1_sub.json loads");
        return;
    }

    Pipe* loaded = nullptr;
    for (auto& entity : data.entities) {
        if (auto* pipe = dynamic_cast<Pipe*>(entity.get())) loaded = pipe;
    }
    if (!loaded) {
        check(false, "level_1_sub.json has a pipe entity");
        return;
    }
    check(loaded->getEntryMode() == Pipe::EntryMode::SideLeft,
          "level_1_sub's way out is read from the file as a west-facing side pipe");

    // The shaft rise is DERIVED from the room's ceiling at load time, not
    // authored, so the loader must have filled it in and the file must not
    // carry it.
    check(loaded->getShaftRise() > 0.0f,
          "the up-pipe's shaft is extended to the ceiling by the loader");

    // Round-trip through the saver. Written into the build tree's own temp
    // area, never into saves/ — guard_saves_hermeticity fails the run for that.
    loaded->setEntryMode(Pipe::EntryMode::SideRight);
    const std::filesystem::path out =
        std::filesystem::temp_directory_path() / "r21j_pipe_roundtrip.json";
    if (!loader.saveLevel(out.string(), map, data.entities, data)) {
        check(false, "the round-trip level saves");
        return;
    }

    {
        std::ifstream file(out);
        nlohmann::json j;
        file >> j;
        bool wroteEntry = false;
        std::string written;
        for (const auto& entity : j["entities"]) {
            if (entity.value("type", std::string()) == "pipe") {
                wroteEntry = entity.contains("entry");
                written = entity.value("entry", std::string());
            }
        }
        check(wroteEntry, "saveLevel writes the pipe's `entry` field");
        check(written == "side_right", "saveLevel writes the mode that was set");

        // The derived value must NOT be written: a file carrying its own copy
        // of the shaft rise would be wrong the first time a ceiling moved.
        bool wroteShaft = false;
        for (const auto& entity : j["entities"]) {
            if (entity.value("type", std::string()) == "pipe" && entity.contains("shaftRise")) {
                wroteShaft = true;
            }
        }
        check(!wroteShaft, "saveLevel does not write the derived shaft rise");
    }

    TileMap reloadedMap;
    LevelData reloaded;
    if (!loader.loadLevel(out.string(), reloadedMap, reloaded)) {
        check(false, "the round-trip level reloads");
        return;
    }
    Pipe* back = nullptr;
    for (auto& entity : reloaded.entities) {
        if (auto* pipe = dynamic_cast<Pipe*>(entity.get())) back = pipe;
    }
    check(back != nullptr, "the reloaded level still has its pipe");
    if (back) {
        check(back->getEntryMode() == Pipe::EntryMode::SideRight,
              "the entry mode came back as it was saved");
    }

    std::error_code ignored;
    std::filesystem::remove(out, ignored);
}

// ---------------------------------------------------------------------------
// 3. Entry geometry, without a keyboard.
// ---------------------------------------------------------------------------
void testTopEntryWantsTheRim() {
    section("R21J  a top-entry pipe is entered from its rim and nowhere else");

    Pipe pipe({320.0f, 320.0f}, 1, {0.0f, 0.0f}, "somewhere.json", true);
    pipe.setEntryMode(Pipe::EntryMode::Top);

    // Feet on the rim, centred over it.
    Mario onRim({320.0f + Pipe::WIDTH_PX * 0.5f - 16.0f, 320.0f - 32.0f});
    onRim.setGrounded(true);
    check(pipe.isAtEntryPoint(onRim), "standing on the rim is in position");

    // Beside it on the floor is NOT: a top-entry pipe has no side opening, and
    // accepting one would make every pipe enterable by walking into its wall.
    Mario beside({320.0f - 34.0f, 320.0f + Pipe::HEIGHT_PX - 32.0f});
    beside.setGrounded(true);
    check(!pipe.isAtEntryPoint(beside), "standing beside it is not in position");
}

void testSideEntryWantsTheMouth() {
    section("R21J  a side-entry pipe is entered by walking into its mouth");

    // A pipe whose foot rests on a floor at y = 320 + HEIGHT_PX.
    const sf::Vector2f origin{320.0f, 320.0f};
    const float floorY = origin.y + Pipe::HEIGHT_PX;
    Pipe west(origin, 2, {0.0f, 0.0f}, "somewhere.json", true);
    west.setEntryMode(Pipe::EntryMode::SideLeft);

    // Flush against the west face, standing on that floor — where the collision
    // resolver leaves a player who walked into it. Placed from the player's own
    // box rather than an assumed 32x32: Small Mario is 24x30, so hardcoding a
    // tile left a 2px gap that this check reported as "not in position".
    Mario atMouth({0.0f, 0.0f});
    const AABB marioBox = atMouth.getBoundingBox();
    atMouth.setPosition({origin.x - marioBox.width, floorY - marioBox.height});
    atMouth.setGrounded(true);
    check(west.isAtEntryPoint(atMouth), "flush against the mouth on the floor is in position");

    // Airborne beside the SHAFT, three tiles up, is not. Requiring grounded is
    // the check that keeps a jump past the pipe from warping.
    Mario flying({origin.x - 32.0f, origin.y});
    flying.setGrounded(false);
    check(!west.isAtEntryPoint(flying), "brushing the shaft in mid-air is not in position");

    // Nor is standing on the rim: an up-pipe has no opening on top, which is
    // the whole reason this mode exists.
    Mario onTop({origin.x + Pipe::WIDTH_PX * 0.5f - 16.0f, origin.y - 32.0f});
    onTop.setGrounded(true);
    check(!west.isAtEntryPoint(onTop), "standing on an up-pipe's shaft is not in position");

    // The wrong side does not count either.
    Mario wrongSide({origin.x + Pipe::WIDTH_PX, floorY - marioBox.height});
    wrongSide.setGrounded(true);
    check(!west.isAtEntryPoint(wrongSide),
          "standing at the far end of a west-facing pipe is not in position");

    // And the mirrored mode is mirrored, not merely "a side pipe".
    Pipe east(origin, 2, {0.0f, 0.0f}, "somewhere.json", true);
    east.setEntryMode(Pipe::EntryMode::SideRight);
    check(east.isAtEntryPoint(wrongSide), "an east-facing pipe is entered from its east side");
    check(!east.isAtEntryPoint(atMouth), "an east-facing pipe is not entered from the west");

    check(west.getSideApproachDirection() > 0.0f, "a west-facing mouth is walked into heading east");
    check(east.getSideApproachDirection() < 0.0f, "an east-facing mouth is walked into heading west");
    Pipe top(origin, 1, {0.0f, 0.0f}, "somewhere.json", true);
    check(top.getSideApproachDirection() == 0.0f, "a top-entry pipe has no horizontal approach");
}

void testMouthGeometryMatchesTheArt() {
    section("R21J  the enterable mouth is the drawn mouth");

    // Not a tautology in the way it looks: these were two independently written
    // 64s until the constant moved into PipeRenderer, and the trigger drifting
    // from the art is how you get a pipe you can see into but not enter.
    Pipe west({320.0f, 320.0f}, 2, {0.0f, 0.0f}, "somewhere.json", true);
    west.setEntryMode(Pipe::EntryMode::SideLeft);
    check(west.mouthHeight() ==
              Pipe::HEIGHT_PX * PipeRenderer::L_BEND_MOUTH_HEIGHT_FRAC,
          "Pipe's mouth band is the arm fraction PipeRenderer draws");
    check(west.mouthHeight() == Constants::TILE_SIZE,
          "the shipped 4-tile collider gives a one-tile mouth, as the atlas draws it");
    const sf::Vector2f mouth = west.getMouthCenter();
    check(mouth.x == 320.0f, "a west-facing mouth is on the pipe's west face");
    check(mouth.y == 320.0f + Pipe::HEIGHT_PX - west.mouthHeight() * 0.5f,
          "the mouth is centred in the arm, at the bottom of the collider");

    Pipe top({320.0f, 320.0f}, 1, {0.0f, 0.0f}, "somewhere.json", true);
    const sf::Vector2f rim = top.getMouthCenter();
    check(rim.x == 320.0f + Pipe::WIDTH_PX * 0.5f && rim.y == 320.0f,
          "a top-entry pipe's opening is the centre of its rim");
}

// A pipe that is not an entrance is never enterable, whatever its mode or the
// player's position — the decorative runs and bonus_1's scenery pipe rely on it.
void testDecorativePipesAreNeverWarps() {
    section("R21J  a pipe that is not an entrance never warps");

    Pipe scenery({320.0f, 320.0f}, 0, {0.0f, 0.0f}, "", false);
    Mario onRim({320.0f + Pipe::WIDTH_PX * 0.5f - 16.0f, 320.0f - 32.0f});
    onRim.setGrounded(true);
    // isAtEntryPoint answers geometry only; checkWarp is the one that must
    // refuse, and it refuses before it ever looks at the keyboard.
    check(!scenery.checkWarp(onRim), "a non-entrance pipe does not warp from its rim");
    scenery.setEntryMode(Pipe::EntryMode::SideLeft);
    Mario atMouth({0.0f, 0.0f});
    atMouth.setPosition({320.0f - atMouth.getBoundingBox().width,
                         320.0f + Pipe::HEIGHT_PX - atMouth.getBoundingBox().height});
    atMouth.setGrounded(true);
    check(!scenery.checkWarp(atMouth), "a non-entrance side pipe does not warp from its mouth");
}

} // namespace

int main() {
    std::cout << "=== verify_r21_pipes ===\n";

    testCellArtNeverYieldsHalfARim();
    testTileMapAndEntityPathsAgree();
    testEntryModeNamesRoundTrip();
    testEntryModeSurvivesASaveAndReload();
    testTopEntryWantsTheRim();
    testSideEntryWantsTheMouth();
    testMouthGeometryMatchesTheArt();
    testDecorativePipesAreNeverWarps();

    std::cout << "\n" << (g_checks - g_failures) << "/" << g_checks << " checks passed\n";
    if (g_failures > 0) {
        std::cout << g_failures << " FAILED\n";
        return 1;
    }
    std::cout << "verify_r21_pipes PASSED\n";
    return 0;
}
