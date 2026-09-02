// R21: one asset tree, enforced.
//
// The game shipped with TWO level trees. SuperMarioGame/assets/levels/ was the
// tracked, hand-tuned one; a second, untracked tree sat at the repository root
// because tools/generate_game_levels.cpp deliberately wrote every level twice
// (once to relPath, once to "../" + relPath).
//
// That would have been merely untidy if path resolution had a fixed root, but
// ResourceManager::resolvePath tries the bare relative path FIRST and only then
// falls back to "../" and "SuperMarioGame/". So the tree that won depended
// entirely on the working directory the game was launched from:
//
//   cwd = SuperMarioGame/   -> tracked tree   (correct)
//   cwd = repository root   -> root tree      (stale)
//
// The root tree was raw MapGenerator output frozen at 2026-08-31 12:35, nine
// minutes before the two commits that fixed the defects it still contained:
//   309c1a5  sub-level floors use Ground, not Brick, so a P-Switch cannot
//            convert the floor to coins and drop the player into the void
//   cc6a32d  the sub-level entrance pipe is a full 2-column pipe, not a
//            1-column "half" pipe missing its right side
// It also flattened every level's "theme" to "overworld", losing the
// underground/ice/castle themes that drive BackgroundRenderer, and carried
// only 2-3 enemies per level against the tracked tree's 11-15.
//
// So three separately-reported release defects -- "half pipes", "1-1 Sub spawn
// is broken and the P-Switch drops you", and "level detail differs" -- were one
// bug wearing three hats, and which hat you saw depended on where you typed
// ./SuperMarioGame from.
//
// The dual-write is deleted and /assets/ is gitignored. Neither of those stops
// someone reintroducing the second tree: a .gitignore keeps a stray tree out of
// git, not off the disk, and off the disk is what matters because resolution
// reads the disk. This guard is the part that actually fails a build.
//
// A guard that has not been seen to fail is not a guard. To verify this one
// still bites:  mkdir -p assets/levels && touch assets/levels/level_1.json
// from the repository root, run ctest, and confirm this case FAILS; then
// remove it and confirm it passes.
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {

// ASSET_GUARD_PROJECT_ROOT is the source tree's SuperMarioGame/ directory and
// ASSET_GUARD_REPO_ROOT its parent, both injected by CMakeLists.txt at compile
// time. They are resolved this way -- rather than through ResourceManager --
// deliberately: a guard that asked ResourceManager where assets/ lives would
// inherit the exact path-resolution mistake it exists to catch.
fs::path projectRoot() { return fs::path(ASSET_GUARD_PROJECT_ROOT); }
fs::path repoRoot() { return fs::path(ASSET_GUARD_REPO_ROOT); }

int failures = 0;

void fail(const std::string& what) {
    std::cout << "  [FAIL] " << what << std::endl;
    ++failures;
}

void pass(const std::string& what) {
    std::cout << "  [ OK ] " << what << std::endl;
}

// The canonical tree must exist and hold the seven shipped levels. Without this
// half the guard, deleting the RIGHT tree would leave only the stale one and
// still pass the "no shadow tree" check below.
void checkCanonicalTreeIsIntact() {
    const fs::path levels = projectRoot() / "assets" / "levels";
    if (!fs::is_directory(levels)) {
        fail("canonical level tree missing: " + levels.string());
        return;
    }
    const std::vector<std::string> shipped = {
        "level_1.json",     "level_2.json",     "level_3.json", "bonus_1.json",
        "level_1_sub.json", "level_2_sub.json", "level_3_sub.json"};
    for (const auto& name : shipped) {
        if (!fs::is_regular_file(levels / name)) {
            fail("canonical tree is missing " + name);
        }
    }
    if (failures == 0) {
        pass("canonical tree " + levels.string() + " holds all 7 shipped levels");
    }
}

// Any assets/ directory at the repository root shadows the canonical tree for
// anyone launching from there. Checked as "exists at all" rather than "differs
// from canonical": a root tree that happens to match today still silently wins
// resolution tomorrow, and identical-now is not a property anything maintains.
void checkNoShadowTree() {
    const fs::path shadow = repoRoot() / "assets";
    if (!fs::exists(shadow)) {
        pass("no shadow asset tree at " + shadow.string());
        return;
    }
    fail("a second asset tree exists at " + shadow.string() +
         " -- it shadows " + (projectRoot() / "assets").string() +
         " whenever the game is launched from the repository root. Delete it. "
         "If something recreated it, that writer is the actual bug.");
}

}  // namespace

int main() {
    std::cout << "=== guard_asset_single_source ===" << std::endl;
    checkCanonicalTreeIsIntact();
    checkNoShadowTree();

    if (failures > 0) {
        std::cout << failures << " check(s) FAILED" << std::endl;
        return 1;
    }
    std::cout << "All checks passed." << std::endl;
    return 0;
}
