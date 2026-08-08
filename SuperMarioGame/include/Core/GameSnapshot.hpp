#pragma once

#include <SFML/System/Vector2.hpp>
#include <vector>

struct PlayerSnapshot {
    sf::Vector2f position{0.0f, 0.0f};
    sf::Vector2f velocity{0.0f, 0.0f};
    int score = 0;
    int coins = 0;
    int lives = 3;
    bool onGround = false;
};

struct EntitySnapshot {
    sf::Vector2f position{0.0f, 0.0f};
    sf::Vector2f velocity{0.0f, 0.0f};
    bool active = true;
};

struct GameSnapshot {
    PlayerSnapshot playerState;
    std::vector<EntitySnapshot> entityStates;
    float levelTimer = 300.0f;
    sf::Vector2f cameraCenter{0.0f, 0.0f};
};
