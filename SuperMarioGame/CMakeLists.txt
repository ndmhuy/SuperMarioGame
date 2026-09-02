cmake_minimum_required(VERSION 3.15)

project(SuperMarioGame)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(MSVC)
    add_compile_options(/FS)
endif()

# ---------------------------------------------------------
# 1. Fetch Dependencies (SFML & ImGui)
# ---------------------------------------------------------
include(FetchContent)

FetchContent_Declare(
    SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG        3.0.2
)
FetchContent_MakeAvailable(SFML)

FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.91.8
)
FetchContent_MakeAvailable(imgui)

# Avoid SFML and ImGui-SFML find conflicts
set(IMGUI_SFML_FIND_SFML OFF CACHE BOOL "Do not search for installed SFML" FORCE)
set(IMGUI_SFML_IMGUI_DIR ${imgui_SOURCE_DIR} CACHE PATH "Path to ImGui" FORCE)
set(IMGUI_DIR ${imgui_SOURCE_DIR} CACHE PATH "Path to ImGui" FORCE)

FetchContent_Declare(
    imgui-sfml
    GIT_REPOSITORY https://github.com/SFML/imgui-sfml.git
    GIT_TAG        v3.0
)
FetchContent_MakeAvailable(imgui-sfml)

FetchContent_Declare(
    json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
)
FetchContent_MakeAvailable(json)

# ---------------------------------------------------------
# 1b. Warnings & testing
# ---------------------------------------------------------
# Warnings are cheap here: a full -Wall -Wextra pass over the ten largest
# translation units produced seven findings, two of which (m_powerUpSubId,
# m_levelCompleteSubId) were direct evidence of an unwired feature.
if (MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wno-unused-parameter)
endif()

enable_testing()

# Registers a headless test executable and its ctest case in one step, instead
# of repeating the add_executable / include / link block per target.
# Builds a harness and, unless NO_CTEST is given, registers it as a ctest case.
#
# NO_CTEST is for the harnesses that open an sf::RenderWindow and run until it is
# closed — the *_visual ones plus verify_hud_boss and verify_enemies_behavior. They
# wait for a human
# to look at it, so `ctest` hung forever on the first one. They are inspection
# tools, not tests — built by `make`, run by hand.
# Everything `ctest` will try to run, as one target. CI builds this instead of
# naming binaries by hand.
#
# It has to exist because the hand-written list drifted: the workflow built
# SuperMarioGame and verify_regressions, while ctest ran thirteen cases, so the
# other eleven reported "***Not Run" and the job failed on every commit — on
# `dev` too. Every run in this repository's history was red, and the real defects
# underneath were invisible behind that noise. Depending on the registration
# itself means adding a test cannot reintroduce the gap.
add_custom_target(ctest_binaries)

function(add_verify_test name source)
    cmake_parse_arguments(AVT "NO_CTEST" "" "" ${ARGN})
    # Each verify_* target used to compile APP_SOURCES (~133 files) itself, so
    # 22+ targets meant 22+ independent compiles of the whole app — ~2,900
    # object files and multiple GB per build tree (log :4122; R11). All of
    # that now lives in the AppObjectsTest OBJECT library (declared once,
    # below, from the same APP_SOURCES), compiled ONCE with -UNDEBUG. Linking
    # against it here pulls in its object files, its PUBLIC include dirs
    # (include/ and tests/, for the shared TestSaveSandbox helper) and its
    # PUBLIC SFML/ImGui-SFML/json link libraries — this target only needs to
    # additionally compile ${source} itself.
    #
    # Release (see shared-build.cmake) compiles with the stock -O3 -DNDEBUG,
    # which compiles every assert() in these harnesses to nothing: the process
    # runs, prints PASSED, and exits 0 without evaluating a single assertion.
    # Proven in phase R5 — forcing HiddenBlock::onCharacterTouch to always
    # return BlockTouch::Solid still left verify_blocks exiting 0.
    #
    # Undefining NDEBUG for AppObjectsTest, for test targets only, fixes the
    # root cause at the one chokepoint every verify_* target already flows
    # through, so any harness using assert() — today's or tomorrow's — works
    # correctly forever. The alternative, converting ~230 assert() call sites
    # across 10 files to a counting check() helper, only fixes today's
    # instances: a harness written tomorrow with a bare assert() would be
    # silently vacuous again, and the rewrite itself is high-churn mechanical
    # editing across 10 files with real transcription-error risk in exactly
    # the code meant to catch errors. This keeps the correct asymmetry too:
    # the shipped game target links AppObjectsRelease (built WITH NDEBUG —
    # asserts compiled out, as a release build should), while only test
    # targets link AppObjectsTest (built WITHOUT it).
    #
    # Real cost, stated honestly: assert() aborts on first failure, so a
    # failing harness reports one failure rather than a full list. The newest
    # harnesses (verify_regressions.cpp, and R5's new sections in
    # verify_blocks.cpp / verify_enemies.cpp) already use a counting check()
    # helper instead and report every failure; converting the older
    # bare-assert harnesses to that pattern remains a worthwhile follow-up for
    # report quality, but it is not what fixes this defect.
    add_executable(${name} "${source}")
    # ${source} (e.g. tests/verify_blocks.cpp) is compiled directly into this
    # target, NOT into AppObjectsTest — it still needs -UNDEBUG applied here
    # too, or the harness's OWN top-level assert() calls (as opposed to the
    # ones inside the APP_SOURCES classes they exercise) compile away under
    # Release same as before. Caught by the R11 mutation test: without this
    # line, disabling QuestionBlock::onHitFromBelow's m_isEmpty assignment
    # left verify_blocks exiting 0, because assert(qBlock.isEmpty()) at
    # tests/verify_blocks.cpp:147 is itself a bare assert() in this file.
    target_compile_options(${name} PRIVATE -UNDEBUG)
    target_link_libraries(${name} PRIVATE AppObjectsTest)
    if (NOT AVT_NO_CTEST)
        add_test(NAME ${name} COMMAND ${name})
        # Assets resolve relative to the source tree, not the build directory.
        # FIXTURES_REQUIRED wires this case into the saves_hermeticity guard
        # (R11, see guard_saves_hermeticity below): CTest runs the fixture's
        # SETUP test before, and its CLEANUP test after, every test that
        # requires it -- so a snapshot of the real saves/ is taken before this
        # case can touch it and compared again once every such case has run.
        # Every ctest-registered verify_* case gets this automatically and for
        # free; a harness would have to opt OUT (NO_CTEST) to escape the guard.
        set_tests_properties(${name} PROPERTIES
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
            TIMEOUT 120
            FIXTURES_REQUIRED saves_hermeticity)
        add_dependencies(ctest_binaries ${name})
    endif()
endfunction()

# ---------------------------------------------------------
# 2. Source Files configuration
# ---------------------------------------------------------
# CONFIGURE_DEPENDS re-globs on build, so adding a new .cpp does not require a
# manual reconfigure. Without it a new source silently fails to link.
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    "src/*.cpp"
)

set(APP_SOURCES ${SOURCES})
list(FILTER APP_SOURCES EXCLUDE REGEX ".*main\\.cpp$")

# ---------------------------------------------------------
# 2b. APP_SOURCES OBJECT libraries (R11 — compile once, link everywhere)
# ---------------------------------------------------------
# Before this, every verify_* target (and generate_game_levels) named
# ${APP_SOURCES} directly in its own add_executable(), so each of the 22+
# targets independently compiled all ~133 app sources — ~2,900 object files
# and several GB per build tree (log :4122). Two OBJECT libraries compile
# APP_SOURCES exactly twice instead:
#   - AppObjectsRelease: stock Release flags (-O3 -DNDEBUG per shared-build.cmake)
#     — linked by the shipped ${PROJECT_NAME} game target.
#   - AppObjectsTest: the same sources with -UNDEBUG added back — linked by
#     every verify_* target via add_verify_test(). This is the asymmetry
#     documented in that function: the game ships with asserts compiled out,
#     the tests need them live. A single shared OBJECT library cannot serve
#     both, since it compiles each source once with one set of flags — hence
#     two libraries, not one.
add_library(AppObjectsRelease OBJECT ${APP_SOURCES})
target_include_directories(AppObjectsRelease PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(AppObjectsRelease PUBLIC
    SFML::Graphics SFML::Window SFML::System SFML::Audio SFML::Network
    ImGui-SFML::ImGui-SFML
    nlohmann_json::nlohmann_json)

add_library(AppObjectsTest OBJECT ${APP_SOURCES})
target_include_directories(AppObjectsTest PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/tests)
target_link_libraries(AppObjectsTest PUBLIC
    SFML::Graphics SFML::Window SFML::System SFML::Audio SFML::Network
    ImGui-SFML::ImGui-SFML
    nlohmann_json::nlohmann_json)
# See add_verify_test()'s comment for why this exists: Release's stock
# -DNDEBUG compiles every assert() in the test harnesses to nothing.
target_compile_options(AppObjectsTest PRIVATE -UNDEBUG)

# ${SOURCES} already globs every src/*.cpp, so nothing below needs to be named
# here — the .cpp entries are duplicates CMake de-duplicates and the .hpp entries
# do not compile at all. They are kept only so the headers show up as target
# sources in IDEs.
#
# Do not hand-type a source path here. `src/Graphics/HUD.cpp` sat in this list
# for months while the file is `Hud.cpp`: macOS resolves that, Linux does not, so
# every CI run failed at the generate step with "Cannot find source file" while
# the local build was fine. If you must add a path, copy it from `git ls-files`.
#
# Only main.cpp is compiled directly here — every other app source comes from
# AppObjectsRelease (built WITH NDEBUG, above). The header-only entries below
# are kept only so they show up as target sources in IDEs; they still don't
# compile.
add_executable(${PROJECT_NAME} src/main.cpp
    include/Graphics/SpriteSheet.hpp
    include/Graphics/Animator.hpp
    include/Graphics/Minimap.hpp
    include/Graphics/ParticleSystem.hpp
    include/Graphics/ParticleEmitter.hpp)

target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(${PROJECT_NAME} PRIVATE AppObjectsRelease)

# ---------------------------------------------------------
# 3. Verification Test Targets
# ---------------------------------------------------------

# R11 hermeticity guard (docs/issues/spec_feature_audit_2026-08-31.md,
# g-rule-13). TestSaveSandbox.hpp gives every harness its OWN scratch save
# directory, but that only proves a harness that uses it behaves -- it does
# not prove every harness uses it, or that some path through the app code a
# harness exercises does not reach the real SuperMarioGame/saves/ some other
# way. This is the check that would actually have caught the original
# incident: a ctest run deleted saves/highscores.json.
#
# guard_saves_hermeticity (tests/guard_saves_hermeticity.cpp) snapshots the
# real saves/ directory content (or its absence) and can also re-snapshot and
# diff against a prior snapshot. Two ctest cases wrap the whole rest of the
# suite in a CTest fixture named "saves_hermeticity":
#   - the SETUP case snapshots saves/ before anything else in the fixture runs
#   - the CLEANUP case re-snapshots and compares after everything that
#     requires the fixture has finished, and FAILS the run if they differ.
# add_verify_test() above attaches FIXTURES_REQUIRED saves_hermeticity to
# every ctest-registered case automatically, so this needs no per-test
# opt-in and cannot be forgotten by a future test the way a convention could.
add_executable(guard_saves_hermeticity "tests/guard_saves_hermeticity.cpp")
target_compile_definitions(guard_saves_hermeticity PRIVATE
    SAVES_HERMETICITY_ROOT="${CMAKE_CURRENT_SOURCE_DIR}")
add_dependencies(ctest_binaries guard_saves_hermeticity)

add_test(NAME guard_saves_hermeticity_setup
         COMMAND guard_saves_hermeticity snapshot)
set_tests_properties(guard_saves_hermeticity_setup PROPERTIES
    FIXTURES_SETUP saves_hermeticity
    TIMEOUT 30)

add_test(NAME guard_saves_hermeticity_check
         COMMAND guard_saves_hermeticity verify)
set_tests_properties(guard_saves_hermeticity_check PROPERTIES
    FIXTURES_CLEANUP saves_hermeticity
    TIMEOUT 30)

# R21: one asset tree, enforced. tools/generate_game_levels.cpp used to write
# every level twice -- once relative to the cwd, once to "../" + path -- which
# created a second, untracked levels tree at the repository root. Because
# ResourceManager::resolvePath tries the bare relative path first, that stale
# tree won whenever the game was launched from the repo root, silently serving
# levels that predated the D21/D22/D23 fixes. The dual-write is gone and
# /assets/ is gitignored, but neither keeps a recreated tree OFF THE DISK, and
# the disk is what path resolution reads. This guard is what actually fails.
add_executable(guard_asset_single_source "tests/guard_asset_single_source.cpp")
target_compile_definitions(guard_asset_single_source PRIVATE
    ASSET_GUARD_PROJECT_ROOT="${CMAKE_CURRENT_SOURCE_DIR}"
    ASSET_GUARD_REPO_ROOT="${CMAKE_CURRENT_SOURCE_DIR}/..")
add_dependencies(ctest_binaries guard_asset_single_source)

add_test(NAME guard_asset_single_source COMMAND guard_asset_single_source)
set_tests_properties(guard_asset_single_source PROPERTIES TIMEOUT 30)

add_verify_test(verify_all "tests/verify_all.cpp")

add_verify_test(verify_map_editor "tests/verify_map_editor.cpp" NO_CTEST)

add_verify_test(verify_save_load "tests/verify_save_load.cpp")

add_verify_test(verify_enemies "tests/verify_enemies.cpp")

# ---------------------------------------------------------
# 4. Verification Test Target (verify_blocks)
# ---------------------------------------------------------
add_verify_test(verify_blocks "tests/verify_blocks.cpp")

# ---------------------------------------------------------
# 5. Verification Test Target (verify_enemies_new - Phase 3.9)
# ---------------------------------------------------------
add_verify_test(verify_enemies_new "tests/verify_enemies_new.cpp")

# ---------------------------------------------------------
# 6. Verification Test Target (verify_blocks_new - Phase 3.13)
# ---------------------------------------------------------
add_verify_test(verify_blocks_new "tests/verify_blocks_new.cpp")

# ---------------------------------------------------------
# 7. Verification Test Target (verify_graphics)
# ---------------------------------------------------------
add_verify_test(verify_graphics "tests/verify_graphics.cpp")

# ---------------------------------------------------------
# 8. Verification Test Target (verify_graphics_visual)
# ---------------------------------------------------------
add_verify_test(verify_graphics_visual "tests/verify_graphics_visual.cpp" NO_CTEST)

# ---------------------------------------------------------
# 9. Verification Test Target (verify_hud_boss)
# ---------------------------------------------------------
add_verify_test(verify_hud_boss "tests/verify_hud_boss.cpp" NO_CTEST)

# ---------------------------------------------------------
# 13. Verification Test Target (verify_fireball)
# ---------------------------------------------------------
add_verify_test(verify_fireball "tests/verify_fireball.cpp")

# ---------------------------------------------------------
# 14. Verification Test Target (verify_flagpole_trampoline)
# ---------------------------------------------------------
add_verify_test(verify_flagpole_trampoline "tests/verify_flagpole_trampoline.cpp")

# ---------------------------------------------------------
# 10. Verification Test Target (verify_minimap_visual)
# ---------------------------------------------------------
add_verify_test(verify_minimap_visual "tests/verify_minimap_visual.cpp" NO_CTEST)

# ---------------------------------------------------------
# 11. Verification Test Target (verify_particles_visual)
# ---------------------------------------------------------
add_verify_test(verify_particles_visual "tests/verify_particles_visual.cpp" NO_CTEST)

# ---------------------------------------------------------
# 12. Verification Test Target (verify_player_animation_visual)
# ---------------------------------------------------------
add_verify_test(verify_player_animation_visual "tests/verify_player_animation_visual.cpp" NO_CTEST)

# ---------------------------------------------------------
# 13. Verification Test Target (verify_enemies_visual)
# ---------------------------------------------------------
add_verify_test(verify_enemies_visual "tests/verify_enemies_visual.cpp" NO_CTEST)

# ---------------------------------------------------------
# 14. Verification Test Target (verify_all_entities_visual)
# ---------------------------------------------------------
add_verify_test(verify_all_entities_visual "tests/verify_all_entities_visual.cpp" NO_CTEST)

# ---------------------------------------------------------
# 15. Verification Test Target (verify_map_generator)
# ---------------------------------------------------------
add_verify_test(verify_map_generator "tests/verify_map_generator.cpp")

# ---------------------------------------------------------
# 16. Verification Test Target (verify_enemies_behavior)
# ---------------------------------------------------------
add_verify_test(verify_enemies_behavior "tests/verify_enemies_behavior.cpp" NO_CTEST)

# ---------------------------------------------------------
# 16.1 Verification Test Target (verify_sound_visual)
# ---------------------------------------------------------
add_verify_test(verify_sound_visual "tests/verify_sound_visual.cpp" NO_CTEST)

# ---------------------------------------------------------
# 17. Level Generation Utility Target (generate_game_levels)
# ---------------------------------------------------------
# Links AppObjectsRelease (see section 2b) instead of naming APP_SOURCES
# directly, so this tool no longer adds a third independent compile of the
# whole app — it's a release-flags tool, not a test, so it shares the game's
# object library rather than the test one.
add_executable(generate_game_levels
    "tools/generate_game_levels.cpp"
)

target_include_directories(generate_game_levels PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(generate_game_levels PRIVATE AppObjectsRelease)







# ---------------------------------------------------------
# 18. Audit Regression Suite (headless, runs under CTest)
# ---------------------------------------------------------
# The August 2026 audit found six subsystems shipped "complete" and inert, and
# three critical defects, none of which any automated check would have caught.
# This target is the guard against that recurring. Keep it headless so CI can
# run it without a display.
add_verify_test(verify_regressions "tests/verify_regressions.cpp")
add_verify_test(verify_frontend_states "tests/verify_frontend_states.cpp")

# Two-player modes, the CPU opponent and Shadow Mario. Headless: it drives the
# policy and the controller directly against a hand-built TileMap, so it needs
# no window and no assets.
add_verify_test(verify_multiplayer_ai "tests/verify_multiplayer_ai.cpp")

# ---------------------------------------------------------
# 19. Asset synchronisation
# ---------------------------------------------------------
# The executable runs with the build directory as its working directory and
# resolves "assets/..." relative to it. Without this step build/assets was a
# stale hand-made copy holding only levels/: the game read out-of-date level
# data, and audio and textures only loaded because ResourceManager::resolvePath
# walks back up into the source tree. A packaged build would have been silent
# and textureless.
#
# Copying after every build keeps one source of truth in the source tree.
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/assets"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/assets"
    COMMENT "Syncing assets/ into the build directory")
