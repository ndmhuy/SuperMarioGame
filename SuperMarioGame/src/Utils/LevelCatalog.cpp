#include "Utils/LevelCatalog.hpp"
#include "Core/ResourceManager.hpp"

#include <nlohmann/json.hpp>
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>

const std::vector<LevelEntry>& LevelCatalog::levels() {
    // Main path only. The *_sub levels are side rooms reached by a warp pipe and
    // left by the pipe at their far end — they carry no flagpole, so nothing in
    // them can ever publish LevelComplete. Listing them here put them in the
    // linear campaign anyway: finishing 1-1 advanced the player into "1-1 Sub",
    // which had no exit and no way to progress. They are still fully playable;
    // they are reached the way the level design intends, through the pipe.
    static const std::vector<LevelEntry> kLevels = {
        {"assets/levels/level_1.json", "1-1",     "World 1-1: Grassland Overworld"},
        {"assets/levels/level_2.json", "1-2",     "World 1-2: Ice Cavern Path"},
        {"assets/levels/level_3.json", "1-3",     "World 1-3: Bowser's Castle Fortress"},
        {"assets/levels/bonus_1.json", "Bonus 1", "Bonus Stage 1: Coin Paradise"}
    };
    return kLevels;
}

int LevelCatalog::count() {
    return static_cast<int>(levels().size());
}

bool LevelCatalog::isValidIndex(int index) {
    return index >= 0 && index < count();
}

const std::string& LevelCatalog::pathFor(int index) {
    return isValidIndex(index) ? levels()[static_cast<std::size_t>(index)].path
                               : levels().front().path;
}

const std::string& LevelCatalog::nameFor(int index) {
    static const std::string kUnknown = "?";
    return isValidIndex(index) ? levels()[static_cast<std::size_t>(index)].displayName
                               : kUnknown;
}

const std::string& LevelCatalog::longNameFor(int index) {
    static const std::string kUnknown = "?";
    return isValidIndex(index) ? levels()[static_cast<std::size_t>(index)].longName
                               : kUnknown;
}

const std::vector<const char*>& LevelCatalog::longNameItems() {
    // Backed by levels(), which is a function-local static built once, so these
    // pointers stay valid for the life of the process.
    static const std::vector<const char*> kItems = [] {
        std::vector<const char*> items;
        items.reserve(levels().size());
        for (const LevelEntry& entry : levels()) items.push_back(entry.longName.c_str());
        return items;
    }();
    return kItems;
}

// --- Author-created levels -----------------------------------------------

namespace {

// The scan result. A function-local static rather than a member because
// LevelCatalog is a namespace of statics with no instance to hang it on.
std::vector<LevelEntry>& customStore() {
    static std::vector<LevelEntry> store;
    return store;
}

bool& customScanned() {
    static bool scanned = false;
    return scanned;
}

} // namespace

const std::string& LevelCatalog::customDirectory() {
    // Beside the shipped levels, NOT in saves/. saves/ is per-player progress
    // that is gitignored and wiped by the test sandbox; a level someone authored
    // is content, and content lives under assets/.
    static const std::string kDir = "assets/levels/custom";
    return kDir;
}

std::string LevelCatalog::resolvedCustomDirectory() {
    // resolvePath only finds paths that already exist, so resolve the levels
    // directory (which always does) and hang "custom" off it. Resolving
    // "assets/levels/custom" directly would fall through to the bare relative
    // path on a first run and create the directory under whatever the working
    // directory happened to be — the same build/saves split that made two
    // config.json files.
    const std::string levelsDir = ResourceManager::resolvePath("assets/levels");
    std::filesystem::path dir = std::filesystem::path(levelsDir) / "custom";
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    if (ec) {
        std::cerr << "[LevelCatalog] Could not create " << dir.string() << ": "
                  << ec.message() << std::endl;
        return std::string();
    }
    // Absolute, so "where did my level go" has an answer that does not depend on
    // knowing which directory the game was launched from.
    std::error_code absErr;
    const std::filesystem::path absolute = std::filesystem::absolute(dir, absErr);
    return absErr ? dir.string() : absolute.lexically_normal().string();
}

std::string LevelCatalog::toFileStem(const std::string& levelName) {
    std::string stem;
    stem.reserve(levelName.size());
    for (unsigned char c : levelName) {
        if (std::isalnum(c)) {
            stem.push_back(static_cast<char>(std::tolower(c)));
        } else if (c == '-' || c == '_' || c == ' ') {
            // Spaces become underscores rather than being dropped, so "My Level"
            // and "MyLevel" stay distinct files.
            stem.push_back('_');
        }
    }
    // Trailing underscores make an ugly file name and an empty name would make
    // ".json", which is a hidden file on every platform this builds for.
    while (!stem.empty() && stem.back() == '_') stem.pop_back();
    if (stem.empty()) stem = "untitled_level";
    return stem;
}

std::string LevelCatalog::customPathFor(const std::string& levelName) {
    const std::string dir = resolvedCustomDirectory();
    if (dir.empty()) return std::string();
    return (std::filesystem::path(dir) / (toFileStem(levelName) + ".json")).string();
}

bool LevelCatalog::isBuiltIn(const std::string& path) {
    // Compared by file name, because the same level reaches this from an
    // asset-relative path, a resolved absolute one and a build-tree copy.
    const std::string name = std::filesystem::path(path).filename().string();
    if (name.empty()) return false;
    for (const LevelEntry& entry : levels()) {
        if (std::filesystem::path(entry.path).filename().string() == name) return true;
    }
    // The sub-levels are not in levels() (they are reached by pipe, never by the
    // campaign walk) but they ship with the game and must be protected too.
    return name.rfind("level_", 0) == 0 || name.rfind("bonus_", 0) == 0;
}

void LevelCatalog::refreshCustomLevels() {
    std::vector<LevelEntry>& store = customStore();
    store.clear();
    customScanned() = true;

    const std::string dir = resolvedCustomDirectory();
    if (dir.empty()) return;

    std::error_code ec;
    std::vector<std::filesystem::path> files;
    for (const auto& entry : std::filesystem::directory_iterator(dir, ec)) {
        if (ec) break;
        if (!entry.is_regular_file()) continue;
        if (entry.path().extension() != ".json") continue;
        files.push_back(entry.path());
    }
    std::sort(files.begin(), files.end());

    for (const std::filesystem::path& file : files) {
        const std::string stem = file.stem().string();
        // The level's own "name", so the list reads as the author titled it
        // rather than as a file stem. A file too broken to parse is still
        // listed — under its stem — because hiding it is how a level someone
        // spent an hour on disappears without a word.
        std::string display = stem;
        std::ifstream in(file);
        if (in.is_open()) {
            try {
                nlohmann::json j;
                in >> j;
                display = j.value("name", stem);
            } catch (const std::exception&) {
                display = stem + " (unreadable)";
            }
        }
        store.push_back(LevelEntry{file.string(), display, display + "  [" + stem + ".json]"});
    }
}

const std::vector<LevelEntry>& LevelCatalog::customLevels() {
    if (!customScanned()) refreshCustomLevels();
    return customStore();
}
