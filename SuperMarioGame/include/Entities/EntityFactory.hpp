#pragma once

#include <memory>
#include <SFML/System/Vector2.hpp>

// Forward declarations of entity bases
class Entity;

// Enum defining all concrete entity types.
//
// Do NOT give these enumerators explicit values, and only ever APPEND new ones
// before Count: the values are contiguous from zero, which is what lets
// verify_r21_entity_registry walk 0..Count and prove that every type has a
// registry entry. PlayingState::spawnProjectile also passes a type across an
// EventBus payload as an int, so renumbering would silently repoint old events.
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
    Castle,        // end-of-level scenery, drawn from castle_end

    // Not a type: the number of types above it, and the only reason it exists.
    // A test can iterate 0..Count and assert that each value resolves to a
    // registry entry the factory can build, so adding an enumerator and
    // forgetting to register it fails the suite instead of producing a palette
    // button that silently places nothing (R21 defect 11). Every switch over
    // EntityType in the codebase has a default label, so this label needs no
    // case of its own.
    Count
};

// The one place the game constructs an entity from a type.
//
// The factory no longer KNOWS the types: EntityCatalogue is the registry, and
// each of its entries carries the creator for its own type. What is left here is
// the policy the factory owns — building, then applying the tuning file — which
// is why it is still the call every caller makes rather than the registry
// directly.
class EntityFactory {
public:
    EntityFactory() = delete;

    // Builds the entity via its EntityCatalogue registration, then applies
    // assets/config/entities.json on top. Null for a type with no registration.
    static std::unique_ptr<Entity> create(EntityType type, sf::Vector2f position);

    // The raw construction, before the config file is applied. Exposed so the
    // tests can show what the file actually changed.
    static std::unique_ptr<Entity> createUnconfigured(EntityType type, sf::Vector2f position);
    static std::unique_ptr<Entity> createFireball(sf::Vector2f position, sf::Vector2f velocity);
};