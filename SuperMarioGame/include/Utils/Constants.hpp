#pragma once

#include <string>
#include <SFML/Graphics/Color.hpp>

namespace Constants {
    // Window Constants
    constexpr unsigned int WINDOW_WIDTH = 1280;
    constexpr unsigned int WINDOW_HEIGHT = 720;
    const std::string WINDOW_TITLE = "Super Mario Bros - CS202";

    // Tile & Level Constants
    constexpr float TILE_SIZE = 32.0f;
    constexpr int LEVEL_TILE_WIDTH = 200;
    constexpr float LEVEL_HEIGHT_TILES = 22.5f;

    // Physics Constants (Standard Mario)
    constexpr float GRAVITY = 0.5f; // px/frame^2
    constexpr float GRAVITY_SCALE = 3600.0f; // 60^2 to convert px/frame^2 to px/s^2
    constexpr float WALK_SPEED = 150.0f; // px/s
    constexpr float RUN_SPEED = 300.0f; // px/s
    constexpr float JUMP_HEIGHT = 128.0f; // ~4 tiles
    constexpr float FIXED_TIMESTEP = 1.0f / 60.0f; // 60 FPS fixed step
    constexpr int COYOTE_FRAMES = 6;
    constexpr int JUMP_BUFFER_FRAMES = 6;
    constexpr float WALL_SLIDE_SPEED = 50.0f;
    constexpr float GROUND_POUND_SPEED = 600.0f;
    constexpr float TERMINAL_VELOCITY = 600.0f;
    constexpr float GROUND_CHECK_OFFSET = 2.0f;

    // Environmental Surface & Zone Constants
    constexpr float CONVEYOR_SPEED = 100.0f;
    constexpr float WATER_GRAVITY_MULT = 0.3f;
    constexpr float WATER_TERMINAL_VELOCITY = 60.0f;

    // Collision Resolution Forces
    constexpr float STOMP_BOUNCE_FORCE = 300.0f;
    constexpr float KNOCKBACK_FORCE_X = 150.0f;
    constexpr float KNOCKBACK_FORCE_Y = 100.0f;
    constexpr float PLAYER_BOUNCE_FORCE = 300.0f;
    constexpr float PLAYER_PUSH_DOWN_FORCE = 100.0f;
    constexpr float PLAYER_PUSH_SIDE_FORCE = 50.0f;

    // Luigi Modifiers
    constexpr float LUIGI_JUMP_MULT = 1.2f;
    constexpr float LUIGI_SPEED_MULT = 0.85f;
    constexpr float LUIGI_GRAVITY_MULT = 0.9f;

    // Gameplay Rules
    constexpr float STAR_DURATION = 10.0f; // seconds
    constexpr float LEVEL_TIME = 300.0f; // seconds
    constexpr int INITIAL_LIVES = 3;
    constexpr int COINS_FOR_LIFE = 100;

    // Enemy Constants
    constexpr float ENEMY_GOOMBA_SPEED = 50.0f;
    constexpr float GOOMBA_SQUISH_DURATION = 0.5f;
    constexpr float ENEMY_KOOPA_SPEED = 50.0f;
    constexpr float KOOPA_SHELL_KICK_SPEED = 300.0f;
    constexpr float KOOPA_SHELL_WAKE_TIME = 5.0f;
    constexpr float KOOPA_SHELL_SHAKE_TIME = 1.5f;
    constexpr float BOO_SPEED = 80.0f;
    constexpr float BOO_CHASE_RANGE = 250.0f;
    constexpr float ENEMY_SPINY_SPEED = 50.0f;

    // Block & Platform Constants
    constexpr float FALLING_PLATFORM_SHAKE_TIME = 1.0f;
    constexpr float FALLING_PLATFORM_RESPAWN_TIME = 5.0f;

    // Minimap Constants
    constexpr float ENTITY_DOT_RADIUS = 2.0f;
    const sf::Color MINIMAP_PLAYER_DOT_COLOR = sf::Color::Green;
    const sf::Color MINIMAP_ENEMY_DOT_COLOR = sf::Color::Red;
    const sf::Color MINIMAP_ITEM_DOT_COLOR = sf::Color::Yellow;
    const sf::Color MINIMAP_SOLID_BLOCK_COLOR = sf::Color(128, 128, 128, 150);
}

