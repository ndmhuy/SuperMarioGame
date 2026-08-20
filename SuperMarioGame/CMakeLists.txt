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
function(add_verify_test name source)
    cmake_parse_arguments(AVT "NO_CTEST" "" "" ${ARGN})
    add_executable(${name} "${source}" ${APP_SOURCES})
    target_include_directories(${name} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_link_libraries(${name} PRIVATE
        SFML::Graphics SFML::Window SFML::System SFML::Audio SFML::Network
        ImGui-SFML::ImGui-SFML
        nlohmann_json::nlohmann_json
    nn)
    if (NOT AVT_NO_CTEST)
        add_test(NAME ${name} COMMAND ${name})
        # Assets resolve relative to the source tree, not the build directory.
        set_tests_properties(${name} PROPERTIES
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
            TIMEOUT 120)
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

add_executable(${PROJECT_NAME} ${SOURCES}
    src/Entities/EntityFactory.cpp
    include/Graphics/SpriteSheet.hpp
    src/Graphics/AnimationManager.cpp
    include/Graphics/Animator.hpp
    src/Graphics/Animator.cpp
    src/Graphics/HUD.cpp
    include/Graphics/Minimap.hpp
    include/Graphics/ParticleSystem.hpp
    src/Graphics/ParticleSystem.cpp
    include/Graphics/ParticleEmitter.hpp)

target_include_directories(${PROJECT_NAME} PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(${PROJECT_NAME} PRIVATE 
    SFML::Graphics 
    SFML::Window 
    SFML::System 
    SFML::Audio 
    SFML::Network
    ImGui-SFML::ImGui-SFML
    nlohmann_json::nlohmann_json
    nn
)

# ---------------------------------------------------------
# 3. Verification Test Targets
# ---------------------------------------------------------
list(FILTER SOURCES EXCLUDE REGEX ".*main\\.cpp$")

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
set(APP_SOURCES ${SOURCES})
list(REMOVE_ITEM APP_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp" "src/main.cpp")

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
# 17. Level Generation Utility Target (generate_game_levels)
# ---------------------------------------------------------
add_executable(generate_game_levels
    "tools/generate_game_levels.cpp"
    ${APP_SOURCES}
)

target_include_directories(generate_game_levels PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(generate_game_levels PRIVATE
    SFML::Graphics
    SFML::Window
    SFML::System
    SFML::Audio
    SFML::Network
    ImGui-SFML::ImGui-SFML
    nlohmann_json::nlohmann_json
    nn
)

# ---------------------------------------------------------
# 16b. Vendored neural network framework (nn::)
# ---------------------------------------------------------
# The author's own from-scratch deep learning library, copied in from CS200-Cpp
# rather than referenced by path so the repository stays self-contained (see
# third_party/nn/README.md for what was copied and the one patch applied).
#
# Built as C++20 IN ISOLATION: the framework uses concepts, while this project
# is C++17 by AGENTS.md directive 5 and stays that way. NeuralPolicy.hpp uses
# PIMPL so no nn/ header ever reaches C++17 game code.
file(GLOB_RECURSE NN_SOURCES "third_party/nn/src/*.cpp")
add_library(nn STATIC ${NN_SOURCES})
target_include_directories(nn PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/third_party/nn/include)
set_target_properties(nn PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)

# Optimise the math library even in a Debug build of the game.
#
# This is measured, not assumed. One training step of the actual policy network
# (1899 -> 64 -> 32 -> 7, forward + backward + SGD):
#
#     no flags (Debug, as the game builds)   12.77 ms   = 77% of a 60Hz frame
#     -O2                                     0.97 ms   =  6% of a frame
#     -O3 -mcpu=native                        0.30 ms   =  1.8% of a frame
#
# 42x. Unoptimised, live in-game training is impossible — one step would eat
# three quarters of the frame budget the renderer needs. It also explains the
# 0.01x-vs-PyTorch figures in upstream's benchmark_results.md: NEON intrinsics
# and expression templates depend entirely on inlining, and a -O0 build gets
# none of it. Upstream sets no optimisation flags and no default build type.
#
# Only this target is affected. Game code stays at the project's build type so
# it remains debuggable; nothing about float semantics is relaxed (no
# -ffast-math), so results stay reproducible.
target_compile_options(nn PRIVATE -O3)
include(CheckCXXCompilerFlag)
check_cxx_compiler_flag("-mcpu=native" NN_HAS_MCPU_NATIVE)
if(NN_HAS_MCPU_NATIVE)
    # Lets the scheduler target this machine's NEON pipeline. Guarded because
    # -mcpu=native is not accepted by every compiler/architecture combination,
    # and a hard failure here would break the build for a grader on x86.
    target_compile_options(nn PRIVATE -mcpu=native)
endif()

# The single game translation unit that talks to nn:: must also be C++20.
# Everything else in APP_SOURCES stays C++17.
set_source_files_properties(
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Entities/NeuralPolicy.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Entities/PolicyTrainer.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Entities/PolicyLosses.cpp"
    PROPERTIES COMPILE_FLAGS "-std=c++20"
)

# ---------------------------------------------------------
# 17c. Headless Policy Training Target (train_policy)
# ---------------------------------------------------------
# The same training loop as the in-game TrainingState, without a window. A
# window costs frame budget, pins the run to a visible desktop session, and caps
# the simulation at whatever the renderer keeps up with — all wrong for a long
# run, however right they are for watching one.
add_executable(render_genome "tools/render_genome.cpp" ${APP_SOURCES})
target_include_directories(render_genome PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(render_genome PRIVATE
    SFML::Graphics SFML::Window SFML::System SFML::Audio SFML::Network
    ImGui-SFML::ImGui-SFML nlohmann_json::nlohmann_json nn)

add_executable(train_policy "tools/train_policy.cpp" ${APP_SOURCES})
target_include_directories(train_policy PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(train_policy PRIVATE
    SFML::Graphics SFML::Window SFML::System SFML::Audio SFML::Network
    ImGui-SFML::ImGui-SFML
    nlohmann_json::nlohmann_json
    nn)

# ---------------------------------------------------------
# 17b. Headless Level Evaluation Target (eval_level)
# ---------------------------------------------------------
# Side project, off the main line — see docs/mapgen_gan_rl_plan.md. Plays a
# level with an AI and writes a JSON report; no window, no GL, no ImGui. Built
# like generate_game_levels because it is the same kind of thing: a windowless
# main() over the game's own sources. Not registered with CTest — it takes a
# level path and reports on it rather than asserting anything.
add_executable(eval_level
    "tools/eval_level.cpp"
    ${APP_SOURCES}
)

target_include_directories(eval_level PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(eval_level PRIVATE
    SFML::Graphics
    SFML::Window
    SFML::System
    SFML::Audio
    SFML::Network
    ImGui-SFML::ImGui-SFML
    nlohmann_json::nlohmann_json
    nn
)







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

# The reinforcement-learning seam: observation encoding, the neural policy's
# drop-in contract, the reward function and the transition log.
add_verify_test(verify_rl_policy "tests/verify_rl_policy.cpp")

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
