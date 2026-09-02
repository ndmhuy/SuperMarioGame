// verify_r21_level_data.cpp — invariants over the seven shipped level files.
//
// Every defect in release batch R21 that reached a player reached them through
// LEVEL DATA that no code path had any reason to reject: a one-column pipe run
// that can only be drawn as half a pipe, a spawn point inside the level's own
// entrance pipe, a legacy fake castle the real Castle entity then landed on top
// of, and a moving platform whose authored sweep ends inside the ground.
//
// The existing harnesses all test CLASSES. This one tests the DATA, because
// that is the class of bug that shipped. Every check here reads the files under
// assets/levels/ through the real LevelLoader, so a level that fails here is a
// level that fails in the game.
//
// Run via:  ctest -R r21_level_data --output-on-failure

#include "Entities/Castle.hpp"
#include "Entities/Entity.hpp"
#include "Entities/Flagpole.hpp"
#include "Entities/MovingPlatform.hpp"
#include "Entities/Pipe.hpp"
#include "Utils/Constants.hpp"
#include "Utils/LevelLoader.hpp"
#include "Utils/MapGenerator.hpp"
#include "Utils/TileMap.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

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

// Levels live relative to the source tree; the test binary may run from build/.
// Same list of roots verify_regressions.cpp uses, for the same reason.
std::string levelPath(const std::string& name) {
    const std::vector<std::string> roots = {
        "assets/levels/", "../assets/levels/", "../../assets/levels/",
        "SuperMarioGame/assets/levels/"
    };
    for (const auto& r : roots) {
        if (std::filesystem::exists(r + name)) return r + name;
    }
    return "assets/levels/" + name;
}

// A level file's basename, from the path a Pipe carries to its destination.
std::string basenameOf(const std::string& path) {
    return std::filesystem::path(path).filename().string();
}

const std::vector<std::string>& shippedLevels() {
    static const std::vector<std::string> names = {
        "level_1.json", "level_2.json", "level_3.json", "bonus_1.json",
        "level_1_sub.json", "level_2_sub.json", "level_3_sub.json"
    };
    return names;
}

bool solidTile(const TileMap& map, int x, int y) {
    if (x < 0 || y < 0 || x >= map.getWidth() || y >= map.getHeight()) return false;
    return TileMap::getInfo(map.getTileType(x, y)).isSolid;
}

// The row of the first solid tile at or below `fromRow` in `col`, or -1.
// Mirrors PlayingState::floorBelow, which is what settles the end-of-level
// scenery and what the spawn guard leans on.
int floorRowBelow(const TileMap& map, int col, int fromRow) {
    if (col < 0 || col >= map.getWidth()) return -1;
    for (int y = std::max(0, fromRow); y < map.getHeight(); ++y) {
        if (TileMap::getInfo(map.getTileType(col, y)).isSolid) return y;
    }
    return -1;
}

// Any solid tile whose cell overlaps the box, edges excluded.
bool overlapsSolid(const TileMap& map, const AABB& box) {
    const float eps = 0.5f;
    const int firstX = static_cast<int>(std::floor((box.x + eps) / Constants::TILE_SIZE));
    const int lastX  = static_cast<int>(std::floor((box.x + box.width - eps) / Constants::TILE_SIZE));
    const int firstY = static_cast<int>(std::floor((box.y + eps) / Constants::TILE_SIZE));
    const int lastY  = static_cast<int>(std::floor((box.y + box.height - eps) / Constants::TILE_SIZE));
    for (int y = firstY; y <= lastY; ++y) {
        for (int x = firstX; x <= lastX; ++x) {
            if (solidTile(map, x, y)) return true;
        }
    }
    return false;
}

std::string at(int x, int y) {
    return "(" + std::to_string(x) + "," + std::to_string(y) + ")";
}

bool loadInto(const std::string& name, TileMap& map, LevelData& data) {
    LevelLoader loader;
    const std::string path = levelPath(name);
    if (!std::filesystem::exists(path)) {
        check(false, name + " exists on disk");
        return false;
    }
    if (!loader.loadLevel(path, map, data)) {
        check(false, name + " loads");
        return false;
    }
    return true;
}

// The raw JSON, for fields no entity exposes back (a moving platform's travel
// range). Read through the same path resolution as the loader.
bool loadJson(const std::string& name, nlohmann::json& j) {
    std::ifstream file(levelPath(name));
    if (!file.is_open()) return false;
    try {
        file >> j;
    } catch (const std::exception&) {
        return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// D22/D23/R21-D1 — half pipes.
//
// The tilemap pipe sprites are 16x16 HALVES of a pipe stretched to a full tile.
// A pipe run only composes into a whole pipe if it is at least two columns
// wide, and only reads as a pipe rather than a rim if it is at least two rows
// tall. PlayingState::pipeTileArtAt now draws whole narrow art for a run that
// is not, but authored data that needs that rescue is authored data that meant
// something else.
// ---------------------------------------------------------------------------
void checkPipeRuns(const TileMap& map, const std::string& label) {
    int narrow = 0;
    int shallow = 0;

    for (int y = 0; y < map.getHeight(); ++y) {
        for (int x = 0; x < map.getWidth(); ) {
            if (map.getTileType(x, y) != TileType::Pipe) { ++x; continue; }
            const int start = x;
            while (x < map.getWidth() && map.getTileType(x, y) == TileType::Pipe) ++x;
            if (x - start < 2) {
                ++narrow;
                std::cout << "        " << label << ": 1-column pipe run at "
                          << at(start, y) << "\n";
            }
        }
    }

    for (int x = 0; x < map.getWidth(); ++x) {
        for (int y = 0; y < map.getHeight(); ) {
            if (map.getTileType(x, y) != TileType::Pipe) { ++y; continue; }
            const int start = y;
            while (y < map.getHeight() && map.getTileType(x, y) == TileType::Pipe) ++y;
            if (y - start < 2) {
                ++shallow;
                std::cout << "        " << label << ": 1-row pipe run at "
                          << at(x, start) << "\n";
            }
        }
    }

    check(narrow == 0, label + ": every pipe tile run is >= 2 columns wide");
    check(shallow == 0, label + ": every pipe tile run is >= 2 rows tall");
}

void testEveryPipeRunIsAWholePipe() {
    section("R21-D1  every pipe tile run is at least 2x2");

    for (const std::string& name : shippedLevels()) {
        TileMap map;
        LevelData data;
        if (!loadInto(name, map, data)) continue;
        checkPipeRuns(map, name);
    }
}

// ---------------------------------------------------------------------------
// R21-D1(c) — a warp pipe entity stands on the floor and its rim is reachable.
//
// Pipe's collider went from 2x2 to 2x4 tiles so PipeRenderer picks the tall
// whole-pipe art and a warp pipe stops looking like scenery. That moves every
// authored pipe's top-left corner, and getting it wrong leaves a pipe floating
// or its rim above the player's jump.
// ---------------------------------------------------------------------------
void checkPipeEntities(const TileMap& map,
                       const std::vector<std::unique_ptr<Entity>>& entities,
                       const std::string& label) {
    for (const auto& entity : entities) {
        const auto* pipe = dynamic_cast<const Pipe*>(entity.get());
        if (!pipe) continue;

        const AABB box = pipe->getBoundingBox();
        const int col  = static_cast<int>(box.x / Constants::TILE_SIZE);
        const int rimRow    = static_cast<int>(box.y / Constants::TILE_SIZE);
        const int bottomRow = static_cast<int>((box.y + box.height) / Constants::TILE_SIZE);
        const std::string who = label + ": pipe at " + at(col, rimRow);

        const int floorRow = floorRowBelow(map, col, rimRow);
        check(floorRow >= 0, who + " has a floor in its column");
        if (floorRow < 0) continue;

        // Not floating: the pipe's foot reaches the floor it stands on.
        check(bottomRow >= floorRow, who + " reaches its floor (foot row " +
                                         std::to_string(bottomRow) + ", floor row " +
                                         std::to_string(floorRow) + ")");

        // And its rim is jumpable from that floor. JUMP_HEIGHT is exactly four
        // tiles, so a rim four tiles up sits on the apex of the arc and is not
        // reliably reachable — and a sub-level's exit pipe is the only way out
        // of the room, so "usually" is not good enough.
        const float rise = (floorRow - rimRow) * Constants::TILE_SIZE;
        check(rise < Constants::JUMP_HEIGHT,
              who + " rim is " + std::to_string(static_cast<int>(rise)) +
                  "px above its floor, under the " +
                  std::to_string(static_cast<int>(Constants::JUMP_HEIGHT)) + "px jump");
    }
}

void testWarpPipesStandOnTheFloorAndCanBeMounted() {
    section("R21-D1  warp pipes are seated, and their rims are within a jump");

    for (const std::string& name : shippedLevels()) {
        TileMap map;
        LevelData data;
        if (!loadInto(name, map, data)) continue;
        checkPipeEntities(map, data.entities, name);
    }
}

// The generator is held to the same two invariants as the files.
//
// cc6a32d fixed the one-column checkpoint pipe in generateSubLevel and missed
// the identical line in generate(), so every procedurally generated overworld
// went on emitting a half pipe with nothing to notice it. Data invariants only
// help if the code that MAKES data is held to them too.
void testGeneratedLevelsAlsoGetWholePipes() {
    section("R21-D1  procedurally generated levels get whole, seated pipes");

    struct Case { const char* label; MapTheme theme; unsigned int seed; };
    const Case cases[] = {
        {"generated overworld", MapTheme::Overworld, 20250901u},
        {"generated ice",       MapTheme::Ice,       31337u},
        {"generated castle",    MapTheme::Castle,    9001u},
    };

    for (const Case& c : cases) {
        TileMap map;
        std::vector<std::unique_ptr<Entity>> entities;
        MapGeneratorConfig config;
        config.theme = c.theme;
        config.seed  = c.seed;
        MapGenerator::generate(map, entities, config);

        checkPipeRuns(map, c.label);
        checkPipeEntities(map, entities, c.label);
    }

    // And the sub-level vault, whose exit pipe is the only way back out.
    TileMap subMap;
    std::vector<std::unique_ptr<Entity>> subEntities;
    MapGenerator::generateSubLevel(subMap, subEntities, MapTheme::Overworld,
                                   MapDifficulty::Medium,
                                   "assets/levels/level_1.json", {96.0f, 608.0f});
    checkPipeRuns(subMap, "generated sub-level");
    checkPipeEntities(subMap, subEntities, "generated sub-level");
}

// ---------------------------------------------------------------------------
// R21-D2 — a spawn point inside solid geometry.
//
// All three sub-levels shipped spawnPoint (3,16) while their own entrance pipe
// occupied cols 3-4, rows 16-17. TileType::Pipe is solid, so the player's whole
// box was inside a wall. It was masked because every route in overrode the
// spawn, and the guard that should have caught it scanned downwards FROM the
// spawn point and hit that same pipe on its first sample.
// ---------------------------------------------------------------------------
void testEverySpawnPointIsStandable() {
    section("R21-D2  every spawn point is inside the map, clear, and has a floor");

    // A small player is 32x32 (Mario's constructor); the smallest form is the
    // weakest claim worth checking.
    constexpr float BOX = 32.0f;

    for (const std::string& name : shippedLevels()) {
        TileMap map;
        LevelData data;
        if (!loadInto(name, map, data)) continue;

        const sf::Vector2f p = data.spawnPoint;
        const float mapRight  = map.getWidth()  * Constants::TILE_SIZE;
        const float mapBottom = map.getHeight() * Constants::TILE_SIZE;

        const bool inside = p.x >= 0.0f && p.x + BOX <= mapRight &&
                            p.y >= 0.0f && p.y + BOX <= mapBottom;
        check(inside, name + ": spawnPoint is inside the map");
        if (!inside) continue;

        check(!overlapsSolid(map, AABB{p.x, p.y, BOX, BOX}),
              name + ": spawnPoint is not inside a solid tile");

        const int col = static_cast<int>((p.x + BOX * 0.5f) / Constants::TILE_SIZE);
        const int row = static_cast<int>((p.y + BOX) / Constants::TILE_SIZE);
        check(floorRowBelow(map, col, row) >= 0,
              name + ": spawnPoint has a floor beneath it");
    }
}

// A pipe's exit names a tile in ANOTHER level, so it is only checkable against
// that destination. This is the check that (104,20) in a 65-wide room failed.
void testEveryPipeExitIsStandableInItsDestination() {
    section("R21-D2  every warp exit lands somewhere standable in its destination");

    constexpr float BOX = 32.0f;

    for (const std::string& name : shippedLevels()) {
        TileMap map;
        LevelData data;
        if (!loadInto(name, map, data)) continue;

        for (const auto& entity : data.entities) {
            const auto* pipe = dynamic_cast<const Pipe*>(entity.get());
            if (!pipe || !pipe->isEntrance() || pipe->getTargetLevel().empty()) continue;

            TileMap dest;
            LevelData destData;
            const std::string destName = basenameOf(pipe->getTargetLevel());
            if (!loadInto(destName, dest, destData)) continue;

            const sf::Vector2f p = pipe->getExitPosition();
            const std::string who = name + " -> " + destName + " exit " +
                at(static_cast<int>(p.x / Constants::TILE_SIZE),
                   static_cast<int>(p.y / Constants::TILE_SIZE));

            const float mapRight  = dest.getWidth()  * Constants::TILE_SIZE;
            const float mapBottom = dest.getHeight() * Constants::TILE_SIZE;
            const bool inside = p.x >= 0.0f && p.x + BOX <= mapRight &&
                                p.y >= 0.0f && p.y + BOX <= mapBottom;
            check(inside, who + " is inside the destination");
            if (!inside) continue;

            check(!overlapsSolid(dest, AABB{p.x, p.y, BOX, BOX}),
                  who + " is not inside a solid tile");

            const int col = static_cast<int>((p.x + BOX * 0.5f) / Constants::TILE_SIZE);
            const int row = static_cast<int>((p.y + BOX) / Constants::TILE_SIZE);
            check(floorRowBelow(dest, col, row) >= 0, who + " has a floor beneath it");
        }
    }
}

// ---------------------------------------------------------------------------
// D21 generalised — a P-Switch must not delete the level.
//
// The switch swaps every Brick tile for a Coin and every Coin for a Brick.
// Coins are not solid, so a column whose only solid tiles are Brick loses its
// floor for the duration. The sub-levels shipped with Brick floors once; this
// is the invariant that says so out loud instead of relying on a comment.
// ---------------------------------------------------------------------------
void testAPSwitchNeverDeletesAColumnsFloor() {
    section("D21  a P-Switch leaves every solid column with something solid in it");

    for (const std::string& name : shippedLevels()) {
        TileMap map;
        LevelData data;
        if (!loadInto(name, map, data)) continue;

        bool hasSwitch = false;
        for (const auto& entity : data.entities) {
            if (entity && entity->getTypeName() == "pswitch") { hasSwitch = true; break; }
        }
        if (!hasSwitch) continue;

        std::vector<int> stranded;
        for (int x = 0; x < map.getWidth(); ++x) {
            bool solidBefore = false;
            bool solidAfter  = false;
            for (int y = 0; y < map.getHeight(); ++y) {
                const TileType t = map.getTileType(x, y);
                TileType swapped = t;
                if (t == TileType::Brick)     swapped = TileType::Coin;
                else if (t == TileType::Coin) swapped = TileType::Brick;
                if (TileMap::getInfo(t).isSolid)       solidBefore = true;
                if (TileMap::getInfo(swapped).isSolid) solidAfter  = true;
            }
            if (solidBefore && !solidAfter) stranded.push_back(x);
        }

        for (int x : stranded) {
            std::cout << "        " << name << ": column " << x
                      << " has no solid tile left once the P-Switch fires\n";
        }
        check(stranded.empty(),
              name + " has a P-Switch and no column it would empty");
    }
}

// ---------------------------------------------------------------------------
// R21-D3 — the castle.
//
// level_1 kept a legacy 5x5 slab of solid `ground` where the real Castle entity
// stands. PlayingState::settleEndOfLevelScenery drops the castle onto the first
// solid tile beneath it, so the castle landed on the slab's roof five rows above
// the floor its own flagpole settled onto, and the end-of-level walk pushed the
// player into the slab's wall for the full three seconds.
//
// Both halves are checked from the settled positions, because settling is what
// decides them — the y in the file is only a hint.
// ---------------------------------------------------------------------------
void testTheCastleStandsWhereTheFlagpoleDoes() {
    section("R21-D3  the castle is clear of terrain and shares the flagpole's floor");

    for (const std::string& name : {"level_1.json", "level_2.json",
                                    "level_3.json", "bonus_1.json"}) {
        TileMap map;
        LevelData data;
        if (!loadInto(name, map, data)) continue;

        const Flagpole* pole = nullptr;
        const Castle* castle = nullptr;
        for (const auto& entity : data.entities) {
            if (auto* f = dynamic_cast<Flagpole*>(entity.get())) pole = f;
            if (auto* c = dynamic_cast<Castle*>(entity.get()))   castle = c;
        }
        if (!pole)   { check(false, name + " has a flagpole"); continue; }
        if (!castle) { check(false, name + " has a castle");   continue; }

        // Both probes mirror settleEndOfLevelScenery exactly: from the middle
        // of the sprite, starting at the entity's own row.
        const sf::Vector2f poleAt   = pole->getPosition();
        const sf::Vector2f castleAt = castle->getPosition();
        const int poleCol   = static_cast<int>((poleAt.x + 12.0f) / Constants::TILE_SIZE);
        const int castleCol = static_cast<int>(
            (castleAt.x + castle->getBoundingBox().width * 0.5f) / Constants::TILE_SIZE);

        const int poleFloor   = floorRowBelow(map, poleCol,
                                              static_cast<int>(poleAt.y / Constants::TILE_SIZE));
        const int castleFloor = floorRowBelow(map, castleCol,
                                              static_cast<int>(castleAt.y / Constants::TILE_SIZE));

        check(poleFloor >= 0,   name + ": the flagpole has a floor to settle onto");
        check(castleFloor >= 0, name + ": the castle has a floor to settle onto");
        if (poleFloor < 0 || castleFloor < 0) continue;

        check(poleFloor == castleFloor,
              name + ": the castle settles onto the flagpole's floor (pole row " +
                  std::to_string(poleFloor) + ", castle row " +
                  std::to_string(castleFloor) + ")");

        // The settled footprint. A castle is scenery the player walks in front
        // of, so terrain inside it is either something they will collide with
        // (the level_1 slab) or something poking through the brickwork.
        const float floorTop  = castleFloor * Constants::TILE_SIZE;
        const float castleH   = Castle::HEIGHT_TILES * Constants::TILE_SIZE;
        const AABB footprint{castleAt.x, floorTop - castleH,
                             castle->getBoundingBox().width, castleH};
        check(!overlapsSolid(map, footprint),
              name + ": the settled castle's footprint is clear of solid tiles");
    }
}

// ---------------------------------------------------------------------------
// R21-D5 (data half) — an authored sweep that runs into terrain.
//
// EntityFactory gives every platform the same hardcoded four tiles to the
// right, and the schema had no way to say otherwise, so level 1-1's platform at
// (90,20) ended its travel inside the ground at cols 94-95. The loader now
// reads "rangeX"/"rangeY" in tiles; this is the check that says a level used
// them correctly.
// ---------------------------------------------------------------------------
void testNoMovingPlatformSweepsIntoTerrain() {
    section("R21-D5  no moving platform's swept box touches a solid tile");

    for (const std::string& name : shippedLevels()) {
        TileMap map;
        LevelData data;
        if (!loadInto(name, map, data)) continue;

        nlohmann::json j;
        if (!loadJson(name, j)) { check(false, name + ": readable as JSON"); continue; }
        if (!j.contains("entities") || !j["entities"].is_array()) continue;

        for (const auto& entityJson : j["entities"]) {
            if (entityJson.value("type", std::string()) != "moving_platform") continue;

            const float tileX  = entityJson.value("x", 0.0f);
            const float tileY  = entityJson.value("y", 0.0f);
            // Defaults match LevelLoader's, which match what EntityFactory has
            // always done, so a level that names no range is checked as it runs.
            const float rangeX = entityJson.value("rangeX", 4.0f);
            const float rangeY = entityJson.value("rangeY", 0.0f);

            const sf::Vector2f pos{tileX * Constants::TILE_SIZE, tileY * Constants::TILE_SIZE};
            // Built rather than hardcoded so the platform's own size stays the
            // single source of truth for how big a platform is.
            const MovingPlatform probe(pos, {0.0f, 0.0f});
            const AABB box = probe.getBoundingBox();

            const float travelX = rangeX * Constants::TILE_SIZE;
            const float travelY = rangeY * Constants::TILE_SIZE;
            const AABB swept{
                pos.x + std::min(0.0f, travelX),
                pos.y + std::min(0.0f, travelY),
                box.width  + std::abs(travelX),
                box.height + std::abs(travelY)
            };

            check(!overlapsSolid(map, swept),
                  name + ": the platform at " + at(static_cast<int>(tileX),
                                                   static_cast<int>(tileY)) +
                      " sweeps " + std::to_string(static_cast<int>(rangeX)) +
                      " tiles without touching terrain");
        }
    }
}

} // namespace

int main() {
    std::cout << "=========================================\n";
    std::cout << " R21 shipped level data invariants\n";
    std::cout << "=========================================\n";

    testEveryPipeRunIsAWholePipe();
    testWarpPipesStandOnTheFloorAndCanBeMounted();
    testGeneratedLevelsAlsoGetWholePipes();
    testEverySpawnPointIsStandable();
    testEveryPipeExitIsStandableInItsDestination();
    testAPSwitchNeverDeletesAColumnsFloor();
    testTheCastleStandsWhereTheFlagpoleDoes();
    testNoMovingPlatformSweepsIntoTerrain();

    std::cout << "\n----------------------------------------\n";
    std::cout << g_checks - g_failures << " / " << g_checks << " checks passed\n";
    if (g_failures > 0) {
        std::cout << g_failures << " FAILURE(S)\n";
        return 1;
    }
    std::cout << "ALL PASS\n";
    return 0;
}
