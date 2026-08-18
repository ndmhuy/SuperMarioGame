// verify_regressions.cpp — headless regression suite for the August 2026 audit.
//
// Every case here corresponds to a defect that shipped, was marked complete in
// TASK_DIVISION.md, and survived because nothing ran automatically. Each one is
// deliberately cheap and window-free so CI can run it.
//
// Run via:  ctest -R regressions --output-on-failure
//
// Add a case here whenever a bug escapes to the audit stage again.

#include "Utils/LevelLoader.hpp"
#include "Utils/TileMap.hpp"
#include "Utils/SerializationUtils.hpp"
#include "Utils/Constants.hpp"
#include "Entities/Mario.hpp"
#include "Entities/IPlayerState.hpp"

#include <filesystem>
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

int countTiles(const TileMap& map, TileType type) {
    int n = 0;
    for (int y = 0; y < map.getHeight(); ++y) {
        for (int x = 0; x < map.getWidth(); ++x) {
            if (map.getTileType(x, y) == type) ++n;
        }
    }
    return n;
}

// ---------------------------------------------------------------------------
// A-1 — the loader dropped every "coin_tile" because it re-implemented the
// string->TileType mapping instead of using SerializationUtils.
// ---------------------------------------------------------------------------
void testCoinTilesLoad() {
    section("A-1  coin tiles survive loading");

    const std::vector<std::string> levels = {
        "level_1.json", "level_2.json", "level_3.json", "bonus_1.json",
        "level_1_sub.json", "level_2_sub.json", "level_3_sub.json"
    };

    for (const auto& name : levels) {
        const std::string path = levelPath(name);
        if (!std::filesystem::exists(path)) {
            check(false, name + " exists on disk");
            continue;
        }
        TileMap map;
        LevelData data;
        LevelLoader loader;
        if (!loader.loadLevel(path, map, data)) {
            check(false, name + " loads");
            continue;
        }
        const int coins = countTiles(map, TileType::Coin);
        check(coins > 0, name + " has coin tiles after load (found " + std::to_string(coins) + ")");
    }
}

// The round-trip is the actual invariant: whatever getTileTypeName emits,
// parseTileTypeName must accept. A drift here is what caused A-1.
void testTileNameRoundTrip() {
    section("A-1  tile name round-trip is total");

    const TileType all[] = {
        TileType::Empty, TileType::Ground, TileType::Brick, TileType::Question,
        TileType::Pipe,  TileType::Ice,    TileType::Conveyor,
        TileType::Water, TileType::Coin,   TileType::Used
    };
    for (TileType t : all) {
        const std::string name = SerializationUtils::getTileTypeName(t);
        const TileType back = SerializationUtils::parseTileTypeName(name);
        check(back == t, "round-trip \"" + name + "\" -> " +
                         std::to_string(static_cast<int>(t)));
    }

    // Legacy aliases must keep loading older hand-written level JSON.
    check(SerializationUtils::parseTileTypeName("coin")     == TileType::Coin,     "alias \"coin\"");
    check(SerializationUtils::parseTileTypeName("question") == TileType::Question, "alias \"question\"");
    check(SerializationUtils::parseTileTypeName("nonsense") == TileType::Empty,    "unknown -> Empty");
}

// ---------------------------------------------------------------------------
// A-7 — Star/Mega decorators called player.changeState() from inside their own
// update(), freeing themselves mid-call. Run under ASan this crashed.
// ---------------------------------------------------------------------------
void testStarDecoratorExpiry() {
    section("A-7  Star decorator expires without destroying itself mid-update");

    Mario mario({100.0f, 100.0f});
    check(dynamic_cast<SmallState*>(mario.getCurrentState()) != nullptr,
          "starts in SmallState");

    mario.powerUp(4); // Star
    check(dynamic_cast<StarDecorator*>(mario.getCurrentState()) != nullptr,
          "Star wraps the base state");

    // Step past STAR_DURATION at the fixed timestep.
    const float dt = Constants::FIXED_TIMESTEP;
    const int steps = static_cast<int>(Constants::STAR_DURATION / dt) + 10;
    for (int i = 0; i < steps; ++i) {
        mario.update(dt);
    }

    check(dynamic_cast<PlayerStateDecorator*>(mario.getCurrentState()) == nullptr,
          "decorator is gone after the timer lapses");
    check(dynamic_cast<SmallState*>(mario.getCurrentState()) != nullptr,
          "the wrapped SmallState survived the unwrap");
}

void testMegaDecoratorExpiry() {
    section("A-7  Mega decorator expires cleanly too");

    Mario mario({100.0f, 100.0f});
    mario.powerUp(0); // Super
    check(dynamic_cast<SuperState*>(mario.getCurrentState()) != nullptr, "reached SuperState");

    mario.powerUp(5); // Mega
    check(dynamic_cast<MegaDecorator*>(mario.getCurrentState()) != nullptr, "Mega wraps SuperState");

    const float dt = Constants::FIXED_TIMESTEP;
    for (int i = 0; i < static_cast<int>(12.0f / dt); ++i) {
        mario.update(dt);
    }

    check(dynamic_cast<PlayerStateDecorator*>(mario.getCurrentState()) == nullptr,
          "Mega decorator retired");
    check(dynamic_cast<SuperState*>(mario.getCurrentState()) != nullptr,
          "SuperState survived");
}

// ---------------------------------------------------------------------------
// A-3 — every path that swaps the player must keep the observers consistent.
// PlayingState::adoptPlayer needs a window, so the headless half of the
// invariant is checked here: carrying stats across a swap must not fire events.
// ---------------------------------------------------------------------------
void testRestoreStatsIsSilent() {
    section("A-3  restoreStats carries stats without side effects");

    Mario mario({0.0f, 0.0f});
    mario.restoreStats(5, 250, 12345);

    check(mario.getLives() == 5,     "lives restored exactly (no gainLife loop)");
    check(mario.getCoins() == 250,   "coins restored exactly, not wrapped by the 100-coin 1-UP rule");
    check(mario.getScore() == 12345, "score restored exactly");

    // addCoins(250) would have granted two extra lives and left coins at 50.
    check(mario.getLives() != 7, "restoreStats did not trigger the 1-UP rule");
}

} // namespace

int main() {
    std::cout << "Audit regression suite\n";

    testCoinTilesLoad();
    testTileNameRoundTrip();
    testStarDecoratorExpiry();
    testMegaDecoratorExpiry();
    testRestoreStatsIsSilent();

    std::cout << "\n----------------------------------------\n";
    std::cout << g_checks - g_failures << " / " << g_checks << " checks passed\n";
    if (g_failures > 0) {
        std::cout << g_failures << " FAILURE(S)\n";
        return 1;
    }
    std::cout << "ALL PASS\n";
    return 0;
}
