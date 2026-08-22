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
    Hammer,        // Hammer Bro projectile
    BossFireball,  // Bowser's fire breath
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
    BridgeAxe,     // ends the Bowser fight without beating him

    // Blocks
    BrickBlock,
    QuestionBlock,
    Pipe,
    Flagpole,
    HiddenBlock,
    MovingPlatform,
    FallingPlatform,
    IceBlock,
    ConveyorBelt,
    Castle         // end-of-level scenery, drawn from castle_end
};

// Factory class definition - declarations only (no implementation)
class EntityFactory {
public:
    EntityFactory() = delete;

    // Creation functions declared but not implemented here
    // Builds the entity and then applies assets/config/entities.json on top.
    static std::unique_ptr<Entity> create(EntityType type, sf::Vector2f position);

    // The raw construction, before the config file is applied. Exposed so the
    // tests can show what the file actually changed.
    static std::unique_ptr<Entity> createUnconfigured(EntityType type, sf::Vector2f position);
    static std::unique_ptr<Entity> createFireball(sf::Vector2f position, sf::Vector2f velocity);
};