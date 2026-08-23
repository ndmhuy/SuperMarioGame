#pragma once

#include "Utils/Serializer.hpp"

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>

// Redirects every save file into a fresh temporary directory for the lifetime
// of the object, and deletes it afterwards.
//
// Why every harness needs one
// ---------------------------
// The suite used to read and write the developer's real saves/ directory. Two
// things went wrong, and both were observed rather than theorised:
//
//   1. A `ctest` run DELETED saves/progress.json. The campaign-progress case
//      exercises CampaignProgress::reset(), which is a std::filesystem::remove
//      on a path that resolves through Serializer::saveDirectory() — the real
//      one. Actual campaign progress vanished between two game launches.
//
//   2. The high-score case asserted "the best run is at the top" against
//      whatever table was already on disk. It passed under ctest and failed
//      when the same binary was run from build/, because the two have different
//      working directories and the case's own backup path was hardcoded
//      relative to cwd while Serializer resolved somewhere else entirely.
//
// g-rule-13: "never let a test assert against a developer's local paths,
// mounted drives, or machine state — point the base directory at an empty
// scratch dir instead." This is that scratch dir. Declare one at the top of
// main() and the whole harness is sealed off, including code it calls that
// reaches for a save path on its own.
class TestSaveSandbox {
public:
    explicit TestSaveSandbox(const std::string& harnessName) {
        // Named for the harness and stamped, so two suites running at once — or
        // a leftover directory from a crashed run — cannot collide.
        const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
        m_path = std::filesystem::temp_directory_path() /
                 ("supermario_" + harnessName + "_" + std::to_string(stamp));

        std::error_code ec;
        std::filesystem::create_directories(m_path, ec);
        if (ec) {
            // Refuse to fall through to the real directory. A sandbox that
            // silently does not sandbox is worse than no sandbox, because the
            // suite would go on passing while deleting save files.
            std::cerr << "[TestSaveSandbox] FATAL: could not create " << m_path
                      << ": " << ec.message() << "\n";
            std::exit(1);
        }
        Serializer::setSaveDirectory(m_path.string());
    }

    ~TestSaveSandbox() {
        Serializer::setSaveDirectory("");
        std::error_code ec;
        std::filesystem::remove_all(m_path, ec);   // best effort; it is a temp dir
    }

    TestSaveSandbox(const TestSaveSandbox&) = delete;
    TestSaveSandbox& operator=(const TestSaveSandbox&) = delete;

    const std::filesystem::path& path() const { return m_path; }

private:
    std::filesystem::path m_path;
};
