#pragma once

#include <memory>
#include <SFML/System/Vector2.hpp>

// Forward declarations of entity bases
class Entity;

// Enum defining all concrete entity types
enum class EntityType {
    // Players
    Mario,
    Luigi,
    Toad,
    Peach,

    // Enemies
    Goomba,
    KoopaTroopa,
    KoopaParatroopa,
    Boo,
    PiranhaPlant,
    BulletBill,
    HammerBro,
    Thwomp,
    ChainChomp,
    Lakitu,
    Spiny,
    Hammer,      // Hammer Bro projectile
    Bowser,
    BoomBoom,

    // Items
    Mushroom,
    FireFlower,
    Coin,
    Star,
    OneUpMushroom,
    CapeFeather,
    MegaMushroom,
    MiniMushroom,
    POWBlock,
    PSwitch,
    Trampoline,
    StarCoin,

    // Blocks
    BrickBlock,
    QuestionBlock,
    Pipe,
    Flagpole,
    HiddenBlock,
    MovingPlatform,
    FallingPlatform,
    IceBlock,
    ConveyorBelt
};

// Factory class definition - declarations only (no implementation)
class EntityFactory {
public:
    EntityFactory() = delete;

    // Creation functions declared but not implemented here
    static std::unique_ptr<Entity> create(EntityType type, sf::Vector2f position);
    static std::unique_ptr<Entity> createFireball(sf::Vector2f position, sf::Vector2f velocity);
};