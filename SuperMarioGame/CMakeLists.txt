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

# ---------------------------------------------------------
# 2. Source Files configuration
# ---------------------------------------------------------
file(GLOB_RECURSE SOURCES 
    "src/*.cpp"
)

add_executable(${PROJECT_NAME} ${SOURCES})

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

