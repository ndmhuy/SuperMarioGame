#pragma once

#include <SFML/System/Vector2.hpp>
#include <vector>
#include <cstdint>

struct PlayerSnapshot {
    sf::Vector2f position{0.0f, 0.0f};
    sf::Vector2f velocity{0.0f, 0.0f};
    int score = 0;
    int coins = 0;
    int lives = 3;
    bool onGround = false;
};

struct EntitySnapshot {
    // Entity::getId(), not a position in PlayingState::m_entities. Indices shift
    // whenever an entity is pruned or spawned between record and restore, which
    // made rewind assign positions to the wrong entities (audit A-5).
    std::uint32_t id = 0;
    sf::Vector2f position{0.0f, 0.0f};
    sf::Vector2f velocity{0.0f, 0.0f};
    bool active = true;
};

// Payload for EventType::PowerUpRequested — a block asking for an item to be
// spawned. Carries the spawn site so the listener does not have to find the
// block that sent it.
struct PowerUpRequest {
    int itemType = 0;             // matches Player::powerUp() item ids
    sf::Vector2f spawnPosition{}; // world position to spawn at
};

// Payload for EventType::EntitySpawnRequested. Entities have no handle on the
// world's entity list, so anything that needs to create another entity asks for
// it here and PlayingState performs the spawn. This is how Lakitu drops Spinies
// and Hammer Bro throws hammers (audit B-6, B-7).
struct EntitySpawnRequest {
    int type = 0;                 // EntityType, as int to keep this header light
    sf::Vector2f position{};
    sf::Vector2f velocity{};
};

struct GameSnapshot {
    PlayerSnapshot playerState;
    std::vector<EntitySnapshot> entityStates;
    float levelTimer = 300.0f;
    sf::Vector2f cameraCenter{0.0f, 0.0f};
};
