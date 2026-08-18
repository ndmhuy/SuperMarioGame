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
function(add_verify_test name source)
    add_executable(${name} "${source}" ${APP_SOURCES})
    target_include_directories(${name} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_link_libraries(${name} PRIVATE
        SFML::Graphics SFML::Window SFML::System SFML::Audio SFML::Network
        ImGui-SFML::ImGui-SFML
        nlohmann_json::nlohmann_json)
    add_test(NAME ${name} COMMAND ${name})
    # Assets resolve relative to the source tree, not the build directory.
    set_tests_properties(${name} PROPERTIES
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        TIMEOUT 120)
endfunction()

# ---------------------------------------------------------
# 2. Source Files configuration
# ---------------------------------------------------------
file(GLOB_RECURSE SOURCES 
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
)

# ---------------------------------------------------------
# 3. Verification Test Targets
# ---------------------------------------------------------
list(FILTER SOURCES EXCLUDE REGEX ".*main\\.cpp$")

add_executable(verify_all
    ${SOURCES}
    tests/verify_all.cpp
)
target_include_directories(verify_all PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(verify_all PRIVATE
    SFML::Graphics
    SFML::Window
    SFML::System
    SFML::Audio
    SFML::Network
    ImGui-SFML::ImGui-SFML
    nlohmann_json::nlohmann_json
)

add_executable(verify_map_editor 
    ${SOURCES} 
    tests/verify_map_editor.cpp
)
target_include_directories(verify_map_editor PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(verify_map_editor PRIVATE 
    SFML::Graphics 
    SFML::Window 
    SFML::System 
    SFML::Audio 
    SFML::Network
    ImGui-SFML::ImGui-SFML
    nlohmann_json::nlohmann_json
)

add_executable(verify_save_load 
    tests/verify_save_load.cpp
    ${SOURCES}
)
target_include_directories(verify_save_load PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(verify_save_load PRIVATE 
    SFML::Graphics 
    SFML::Window 
    SFML::System 
    SFML::Audio 
    SFML::Network
    ImGui-SFML::ImGui-SFML
    nlohmann_json::nlohmann_json
)

add_executable(verify_enemies 
    "tests/verify_enemies.cpp"
    ${SOURCES}
)
target_include_directories(verify_enemies PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(verify_enemies PRIVATE 
    SFML::Graphics 
    SFML::Window 
    SFML::System 
    SFML::Audio 
    SFML::Network
    ImGui-SFML::ImGui-SFML
    nlohmann_json::nlohmann_json
)

# ---------------------------------------------------------
# 4. Verification Test Target (verify_blocks)
# ---------------------------------------------------------
add_executable(verify_blocks 
    "tests/verify_blocks.cpp"
    ${SOURCES}
)

target_include_directories(verify_blocks PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(verify_blocks PRIVATE 
    SFML::Graphics 
    SFML::Window 
    SFML::System 
    SFML::Audio 
    SFML::Network
    ImGui-SFML::ImGui-SFML
    nlohmann_json::nlohmann_json
)

# ---------------------------------------------------------
# 5. Verification Test Target (verify_enemies_new - Phase 3.9)
# ---------------------------------------------------------
add_executable(verify_enemies_new
    "tests/verify_enemies_new.cpp"
    ${SOURCES}
)

target_include_directories(verify_enemies_new PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(verify_enemies_new PRIVATE 
    SFML::Graphics 
    SFML::Window 
    SFML::System 
    SFML::Audio 
    SFML::Network
    ImGui-SFML::ImGui-SFML
    nlohmann_json::nlohmann_json
)

# ---------------------------------------------------------
# 6. Verification Test Target (verify_blocks_new - Phase 3.13)
# ---------------------------------------------------------
add_executable(verify_blocks_new
    "tests/verify_blocks_new.cpp"
    ${SOURCES}
)

target_include_directories(verify_blocks_new PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(verify_blocks_new PRIVATE 
    SFML::Graphics 
    SFML::Window 
    SFML::System 
    SFML::Audio 
    SFML::Network
    ImGui-SFML::ImGui-SFML
    nlohmann_json::nlohmann_json
)

# ---------------------------------------------------------
# 7. Verification Test Target (verify_graphics)
# ---------------------------------------------------------
add_executable(verify_graphics
    "tests/verify_graphics.cpp"
    "src/Graphics/SpriteSheet.cpp"
    "src/Graphics/Animator.cpp"
    "src/Graphics/AnimationManager.cpp"
    "src/Core/ResourceManager.cpp"
)

target_include_directories(verify_graphics PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(verify_graphics PRIVATE 
    SFML::Graphics 
    SFML::Window 
    SFML::System 
    SFML::Audio 
    SFML::Network
    ImGui-SFML::ImGui-SFML
    nlohmann_json::nlohmann_json
)

# ---------------------------------------------------------
# 8. Verification Test Target (verify_graphics_visual)
# ---------------------------------------------------------
add_executable(verify_graphics_visual
    "tests/verify_graphics_visual.cpp"
    ${APP_SOURCES}
)

target_include_directories(verify_graphics_visual PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(verify_graphics_visual PRIVATE 
    SFML::Graphics 
    SFML::Window 
    SFML::System 
    SFML::Audio 
    SFML::Network
    ImGui-SFML::ImGui-SFML
    nlohmann_json::nlohmann_json
)

# ---------------------------------------------------------
# 9. Verification Test Target (verify_hud_boss)
# ---------------------------------------------------------
add_executable(verify_hud_boss
    "tests/verify_hud_boss.cpp"
    "src/Graphics/HUD.cpp"
    "src/Core/ResourceManager.cpp"
    "src/Core/EventBus.cpp"
)

target_include_directories(verify_hud_boss PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(verify_hud_boss PRIVATE 
    SFML::Graphics 
    SFML::Window 
    SFML::System 
    SFML::Audio
    ImGui-SFML::ImGui-SFML
    nlohmann_json::nlohmann_json
)

# ---------------------------------------------------------
# 13. Verification Test Target (verify_fireball)
# ---------------------------------------------------------
add_executable(verify_fireball
    ${SOURCES}
    tests/verify_fireball.cpp
)
target_include_directories(verify_fireball PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(verify_fireball PRIVATE
    SFML::Graphics
    SFML::Window
    SFML::System
    SFML::Audio
    SFML::Network
    ImGui-SFML::ImGui-SFML
    nlohmann_json::nlohmann_json
)

# ---------------------------------------------------------
# 14. Verification Test Target (verify_flagpole_trampoline)
# ---------------------------------------------------------
add_executable(verify_flagpole_trampoline
    ${SOURCES}
    tests/verify_flagpole_trampoline.cpp
)
target_include_directories(verify_flagpole_trampoline PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(verify_flagpole_trampoline PRIVATE
    SFML::Graphics
    SFML::Window
    SFML::System
    SFML::Audio
    SFML::Network
    ImGui-SFML::ImGui-SFML
    nlohmann_json::nlohmann_json
)

# ---------------------------------------------------------
# 10. Verification Test Target (verify_minimap_visual)
# ---------------------------------------------------------
set(APP_SOURCES ${SOURCES})
list(REMOVE_ITEM APP_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp" "src/main.cpp")

add_executable(verify_minimap_visual
    "tests/verify_minimap_visual.cpp"
    ${APP_SOURCES}
)

target_include_directories(verify_minimap_visual PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(verify_minimap_visual PRIVATE 
    SFML::Graphics 
    SFML::Window 
    SFML::System 
    SFML::Audio 
    SFML::Network
    ImGui-SFML::ImGui-SFML
    nlohmann_json::nlohmann_json
)

# ---------------------------------------------------------
# 11. Verification Test Target (verify_particles_visual)
# ---------------------------------------------------------
add_executable(verify_particles_visual
    "tests/verify_particles_visual.cpp"
    ${APP_SOURCES}
)

target_include_directories(verify_particles_visual PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(verify_particles_visual PRIVATE 
    SFML::Graphics 
    SFML::Window 
    SFML::System 
    SFML::Audio 
    SFML::Network
    ImGui-SFML::ImGui-SFML
    nlohmann_json::nlohmann_json
)

# ---------------------------------------------------------
# 12. Verification Test Target (verify_player_animation_visual)
# ---------------------------------------------------------
add_executable(verify_player_animation_visual
    "tests/verify_player_animation_visual.cpp"
    ${APP_SOURCES}
)

target_include_directories(verify_player_animation_visual PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(verify_player_animation_visual PRIVATE 
    SFML::Graphics 
    SFML::Window 
    SFML::System 
    SFML::Audio 
    SFML::Network
    ImGui-SFML::ImGui-SFML
    nlohmann_json::nlohmann_json
)

# ---------------------------------------------------------
# 13. Verification Test Target (verify_enemies_visual)
# ---------------------------------------------------------
add_executable(verify_enemies_visual
    "tests/verify_enemies_visual.cpp"
    ${APP_SOURCES}
)

target_include_directories(verify_enemies_visual PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(verify_enemies_visual PRIVATE 
    SFML::Graphics 
    SFML::Window 
    SFML::System 
    SFML::Audio 
    SFML::Network
    ImGui-SFML::ImGui-SFML
    nlohmann_json::nlohmann_json
)

# ---------------------------------------------------------
# 14. Verification Test Target (verify_all_entities_visual)
# ---------------------------------------------------------
add_executable(verify_all_entities_visual
    "tests/verify_all_entities_visual.cpp"
    ${APP_SOURCES}
)

target_include_directories(verify_all_entities_visual PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(verify_all_entities_visual PRIVATE 
    SFML::Graphics 
    SFML::Window 
    SFML::System 
    SFML::Audio 
    SFML::Network
    ImGui-SFML::ImGui-SFML
    nlohmann_json::nlohmann_json
)

# ---------------------------------------------------------
# 15. Verification Test Target (verify_map_generator)
# ---------------------------------------------------------
add_executable(verify_map_generator
    "tests/verify_map_generator.cpp"
    ${APP_SOURCES}
)

target_include_directories(verify_map_generator PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(verify_map_generator PRIVATE 
    SFML::Graphics 
    SFML::Window 
    SFML::System 
    SFML::Audio 
    SFML::Network
    ImGui-SFML::ImGui-SFML
    nlohmann_json::nlohmann_json
)

# ---------------------------------------------------------
# 16. Verification Test Target (verify_enemies_behavior)
# ---------------------------------------------------------
add_executable(verify_enemies_behavior
    "tests/verify_enemies_behavior.cpp"
    ${APP_SOURCES}
)

target_include_directories(verify_enemies_behavior PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(verify_enemies_behavior PRIVATE 
    SFML::Graphics 
    SFML::Window 
    SFML::System 
    SFML::Audio 
    SFML::Network
    ImGui-SFML::ImGui-SFML
    nlohmann_json::nlohmann_json
)

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
)







# ---------------------------------------------------------
# 18. Audit Regression Suite (headless, runs under CTest)
# ---------------------------------------------------------
# The August 2026 audit found six subsystems shipped "complete" and inert, and
# three critical defects, none of which any automated check would have caught.
# This target is the guard against that recurring. Keep it headless so CI can
# run it without a display.
add_verify_test(verify_regressions "tests/verify_regressions.cpp")
