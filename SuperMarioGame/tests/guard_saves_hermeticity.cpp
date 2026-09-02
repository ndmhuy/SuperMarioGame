// R11 (docs/issues/spec_feature_audit_2026-08-31.md): the parity-test
// discipline (g-rule-17) applied to g-rule-13's hermetic-CI requirement.
//
// TestSaveSandbox.hpp redirects every verify_* harness's OWN save I/O into a
// scratch temp dir. That is necessary but not sufficient — it proves each
// harness that uses it behaves, not that every harness actually uses it, and
// not that some code path a harness exercises does not reach the real
// SuperMarioGame/saves/ some other way. This binary is the check that would
// actually have caught the original incident: a ctest run deleted
// saves/highscores.json because a case exercised a path that resolved
// through the developer's real save directory instead of a sandboxed one.
//
// It runs twice, wrapped around every other ctest case via CTest fixtures
// (see CMakeLists.txt's "saves_hermeticity" fixture and add_verify_test()):
//   snapshot -- taken once, before any verify_* case runs.
//   verify   -- taken once, after every verify_* case has finished.
// If the two do not match byte-for-byte -- a file's content changed, a file
// was added, a file was removed, or the directory started or stopped
// existing -- something in the suite touched the real saves/ and this test
// FAILS the run. A guard that has not been seen to fail is not a guard: see
// the mutation-test instructions in the R11 log entry for how this one was
// verified to actually catch the incident it targets.
#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {

// SAVES_HERMETICITY_ROOT is the source tree's SuperMarioGame/ directory,
// injected by CMakeLists.txt at compile time (see the guard_saves_hermeticity
// target). Resolving it this way -- rather than through Serializer, or
// through a relative "saves" the way Serializer itself once did -- means this
// check cannot be fooled by the exact bug it exists to catch: if a future
// change made Serializer's own path resolution wrong, a check that asked
// Serializer where saves/ is would inherit that same mistake.
const fs::path& savesDir() {
    static const fs::path dir = fs::path(SAVES_HERMETICITY_ROOT) / "saves";
    return dir;
}

// A fixed, non-randomized path: the "snapshot" invocation and the "verify"
// invocation are two separate process runs and must agree on where the
// manifest lives. Not inside saves/ itself -- that would make the guard
// perturb the very directory it is watching.
fs::path manifestPath() {
    return fs::temp_directory_path() / "supermario_r11_saves_hermeticity_manifest.txt";
}

// FNV-1a over the raw bytes. std::hash<std::string>'s result is unspecified
// across standard library versions/ABIs, which is fine within one process
// but not guaranteed stable if snapshot and verify ever ran with different
// binaries; FNV-1a is defined purely in terms of the bytes, so it can't drift.
uint64_t fnv1a(const std::string& data) {
    uint64_t hash = 14695981039346656037ull;
    for (unsigned char c : data) {
        hash ^= c;
        hash *= 1099511628211ull;
    }
    return hash;
}

// saves/shots/ is screenshot OUTPUT, not save data.
//
// This guard exists to catch a test writing the developer's real save files --
// the incident it was built for was a ctest run deleting saves/progress.json.
// It was watching the whole tree, screenshots included, and saves/shots/ is
// where every `--script` run drops its PNGs. So any ordinary playtest between
// the snapshot and the verify failed the guard, and it failed for a reason that
// had nothing to do with hermeticity.
//
// That is not a harmless false positive. A guard that cries wolf gets read as
// noise, and this one did: two separate investigations this session blamed a
// test harness for "writing saves/" on its evidence, and the harness turned out
// to be clean -- measured, all four save files byte-identical across a run. The
// real cause was a concurrent playtest writing PNGs. A check that cannot tell
// those apart costs more than it earns.
//
// Narrowed to exclude ONLY this one output directory. Every *.json under
// saves/, slot files included, is still watched byte-for-byte, so the incident
// this guard was written for still fails it. Deliberately a path prefix rather
// than an extension filter: a new kind of save file must be watched by default,
// and only a directory whose whole purpose is generated output is skipped.
bool isOutputArtefact(const std::string& relativePath) {
    return relativePath.rfind("shots/", 0) == 0;
}

std::string readFile(const fs::path& p) {
    std::ifstream in(p, std::ios::binary);
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

// One line per file: "relative/path<TAB>size<TAB>hash". Sorted so two
// snapshots of the same directory produce identical text regardless of
// directory-iteration order, which differs across filesystems/platforms.
std::vector<std::string> snapshotLines() {
    std::vector<std::string> lines;
    const fs::path dir = savesDir();

    if (!fs::exists(dir)) {
        // A missing saves/ is itself a fact worth pinning: if some test
        // CREATES the directory where none existed, that is exactly as much
        // a hermeticity violation as modifying a file inside it.
        lines.push_back("__ABSENT__");
        return lines;
    }

    for (const auto& entry : fs::recursive_directory_iterator(dir)) {
        if (!entry.is_regular_file()) continue;
        const std::string rel = fs::relative(entry.path(), dir).generic_string();
        if (isOutputArtefact(rel)) continue;
        const std::string content = readFile(entry.path());
        std::ostringstream line;
        line << rel << '\t' << content.size() << '\t' << fnv1a(content);
        lines.push_back(line.str());
    }
    std::sort(lines.begin(), lines.end());
    return lines;
}

} // namespace

int main(int argc, char** argv) {
    if (argc != 2 || (std::string(argv[1]) != "snapshot" && std::string(argv[1]) != "verify")) {
        std::cerr << "usage: guard_saves_hermeticity <snapshot|verify>\n";
        return 2;
    }
    const std::string mode = argv[1];
    const auto lines = snapshotLines();

    if (mode == "snapshot") {
        std::ofstream out(manifestPath(), std::ios::trunc | std::ios::binary);
        for (const auto& l : lines) out << l << '\n';
        if (!out) {
            std::cerr << "[guard_saves_hermeticity] FATAL: could not write manifest to "
                      << manifestPath() << "\n";
            return 2;
        }
        std::cout << "[guard_saves_hermeticity] snapshotted " << lines.size()
                   << " entr" << (lines.size() == 1 ? "y" : "ies") << " under "
                   << savesDir() << "\n";
        return 0;
    }

    // mode == "verify"
    std::ifstream in(manifestPath());
    if (!in) {
        std::cerr << "[guard_saves_hermeticity] FATAL: no snapshot manifest at "
                  << manifestPath()
                  << " -- the setup test (FIXTURES_SETUP saves_hermeticity) did"
                     " not run before this one. Run the full suite, not a single"
                     " test filtered with -R.\n";
        return 2;
    }
    std::vector<std::string> before;
    std::string line;
    while (std::getline(in, line)) before.push_back(line);

    if (before == lines) {
        std::cout << "[guard_saves_hermeticity] PASS: " << savesDir()
                   << " is byte-for-byte unchanged across the full ctest run ("
                   << lines.size() << " entr" << (lines.size() == 1 ? "y" : "ies")
                   << ").\n";
        return 0;
    }

    std::cerr << "[guard_saves_hermeticity] FAIL: " << savesDir()
              << " changed during this ctest run.\n"
              << "g-rule-13 requires CI to be hermetic: no test may read or write\n"
              << "the developer's real save directory. Point the offending harness\n"
              << "at TestSaveSandbox (tests/TestSaveSandbox.hpp), which redirects\n"
              << "Serializer::setSaveDirectory() to a scratch temp dir for the\n"
              << "harness's lifetime -- see the other 23 verify_* mains for the\n"
              << "one-line pattern.\n\n"
              << "--- before (start of suite) ---\n";
    for (const auto& l : before) std::cerr << l << '\n';
    std::cerr << "--- after (end of suite) ---\n";
    for (const auto& l : lines) std::cerr << l << '\n';
    return 1;
}
