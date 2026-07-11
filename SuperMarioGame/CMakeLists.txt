cmake_minimum_required(VERSION 3.15)

project(SuperMarioGame)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

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
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
)
FetchContent_MakeAvailable(json)

# ---------------------------------------------------------
# 2. Source Files configuration
# ---------------------------------------------------------
file(GLOB_RECURSE SOURCES 
    "src/*.cpp"
)

add_executable(${PROJECT_NAME} ${SOURCES}
    src/Entities/EntityFactory.cpp
    include/Graphics/SpriteSheet.hpp
    src/Graphics/AnimationManager.cpp
    include/Graphics/Animator.hpp
    src/Graphics/Animator.cpp
    src/Graphics/HUD.cpp)

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
# 3. Verification Test Target (verify_enemies)
# ---------------------------------------------------------
add_executable(verify_enemies 
    "tests/verify_enemies.cpp"
    "src/Entities/Goomba.cpp"
    "src/Entities/KoopaTroopa.cpp"
    "src/Entities/KoopaParatroopa.cpp"
    "src/Entities/Boo.cpp"
    "src/Entities/Enemy.cpp"
    "src/Entities/PatrolStrategy.cpp"
    "src/Entities/FlyStrategy.cpp"
    "src/Entities/ChaseStrategy.cpp"
    "src/Entities/Entity.cpp"
    "src/Entities/Character.cpp"
    "src/Core/EventBus.cpp"
    "src/Physics/AABB.cpp"
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
)

# ---------------------------------------------------------
# 4. Verification Test Target (verify_blocks)
# ---------------------------------------------------------
add_executable(verify_blocks 
    "tests/verify_blocks.cpp"
    "src/Entities/Block.cpp"
    "src/Entities/BrickBlock.cpp"
    "src/Entities/QuestionBlock.cpp"
    "src/Entities/Pipe.cpp"
    "src/Entities/Flagpole.cpp"
    "src/Entities/Entity.cpp"
    "src/Entities/Character.cpp"
    "src/Entities/Player.cpp"
    "src/Entities/IPlayerState.cpp"
    "src/Entities/Enemy.cpp"
    "src/Entities/Item.cpp"
    "src/Core/EventBus.cpp"
    "src/Physics/AABB.cpp"
    "src/Physics/CollisionResolver.cpp"
    "src/Physics/CollisionDetector.cpp"
    "src/Physics/SpatialHash.cpp"
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
)

# ---------------------------------------------------------
# 5. Verification Test Target (verify_enemies_new - Phase 3.9)
# ---------------------------------------------------------
add_executable(verify_enemies_new
    "tests/verify_enemies_new.cpp"
    "src/Entities/PiranhaPlant.cpp"
    "src/Entities/BulletBill.cpp"
    "src/Entities/HammerBro.cpp"
    "src/Entities/Thwomp.cpp"
    "src/Entities/ChainChomp.cpp"
    "src/Entities/Lakitu.cpp"
    "src/Entities/Spiny.cpp"
    "src/Entities/Enemy.cpp"
    "src/Entities/Entity.cpp"
    "src/Entities/Character.cpp"
    "src/Entities/Player.cpp"
    "src/Entities/IPlayerState.cpp"
    "src/Entities/Item.cpp"
    "src/Entities/PatrolStrategy.cpp"
    "src/Entities/FlyStrategy.cpp"
    "src/Entities/ChaseStrategy.cpp"
    "src/Entities/LinearStrategy.cpp"
    "src/Entities/TetheredChaseStrategy.cpp"
    "src/Entities/ProximityTriggerStrategy.cpp"
    "src/Entities/TimerEmergenceStrategy.cpp"
    "src/Entities/HammerThrowStrategy.cpp"
    "src/Core/EventBus.cpp"
    "src/Physics/AABB.cpp"
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
)

# ---------------------------------------------------------
# 6. Verification Test Target (verify_blocks_new - Phase 3.13)
# ---------------------------------------------------------
add_executable(verify_blocks_new
    "tests/verify_blocks_new.cpp"
    "src/Entities/HiddenBlock.cpp"
    "src/Entities/MovingPlatform.cpp"
    "src/Entities/FallingPlatform.cpp"
    "src/Entities/IceBlock.cpp"
    "src/Entities/ConveyorBelt.cpp"
    "src/Entities/Block.cpp"
    "src/Entities/Entity.cpp"
    "src/Entities/Character.cpp"
    "src/Entities/Player.cpp"
    "src/Entities/IPlayerState.cpp"
    "src/Entities/Item.cpp"
    "src/Core/EventBus.cpp"
    "src/Physics/AABB.cpp"
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
    "src/Graphics/SpriteSheet.cpp"
    "src/Graphics/Animator.cpp"
    "src/Graphics/AnimationManager.cpp"
    "src/Core/ResourceManager.cpp"
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





