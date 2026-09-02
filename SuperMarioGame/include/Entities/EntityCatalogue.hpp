#pragma once

#include "Entities/EntityFactory.hpp"   // EntityType, and the Entity forward declaration

#include <memory>
#include <string>
#include <vector>

#include <SFML/System/Vector2.hpp>

// The registry: one declaration per entity type, carrying everything the rest
// of the game needs to know about that type — including how to build it.
//
// Why this exists
// ---------------
// The same list of entity types was written out by hand in four places, and
// they had all drifted from each other:
//
//   - EntityFactory::create()          knew 40 types
//   - SerializationUtils::parse...()   knew 40 names
//   - MapEditor's palette              knew 16, none of them enemies
//   - PlaceEntityCommand's if-chain    knew 16 name strings, and built nothing
//
// So the level editor — the feature whose entire purpose is placing entities —
// could not place a Goomba, a Koopa, a pipe, a question block or a flagpole,
// and nothing anywhere failed to say so. Adding a type meant remembering four
// files, and forgetting one was silent.
//
// The first three of those drifts were closed by making the palette and the
// parser read this table. That still left TWO hand-maintained lists — this one
// and EntityFactory's 42-case switch — kept in step only by a regression test.
// A test that catches a drift is a guard on a hazard, not the removal of it, so
// the construction step now lives here too, in Entry::create: the factory
// switch is gone and the table below is the single declaration of a type.
//
// g-rule-22: a list the code already contains must be derived, not hand-synced.
// The parser, the palette and the factory all read this one, and
// verify_r21_entity_registry walks every EntityType enumerator and fails the
// suite for any that has no entry here — so a type added to the enum and
// forgotten cannot become a palette button that silently places nothing.
namespace EntityCatalogue {

// Which drawer of the palette a type belongs in. Ordered as the editor shows
// them, which is roughly the order a level is built in.
enum class Category {
    Player,
    Enemy,
    Item,
    Block,
    Scenery,
    // Projectiles exist as entity types because they are spawned at runtime, but
    // a level file must never contain one. The palette hides this category.
    Projectile
};

// Builds one entity of a given type at a position, before any tuning from
// assets/config/entities.json is applied — EntityFactory::create() owns that
// step, so that construction and configuration stay separable.
//
// A plain function pointer rather than std::function: every creator here is a
// capture-less lambda, so this costs nothing to store or call, and it cannot be
// left in a moved-from or empty state that would fail only at the call site.
// The three constructors that need more than a position — a moving platform's
// travel range, and the launch velocity of the two projectiles — supply it
// inside their own lambda; see the table in EntityCatalogue.cpp.
using Creator = std::unique_ptr<Entity> (*)(sf::Vector2f position);

struct Entry {
    EntityType  type;
    // The canonical serialised name. MUST match the class's getTypeName(), or
    // saving a level and loading it back produces a different entity.
    std::string name;
    // What the editor shows a human.
    std::string label;
    Category    category;
    // Never null. This is the field that makes the table a registry rather than
    // a description of one: there is no second list of types to keep in step.
    Creator     create;
};

// Every type the game can construct, in palette order.
const std::vector<Entry>& all();

// The entries in one category, in palette order.
std::vector<const Entry*> inCategory(Category category);

// Null when the name is not canonical. Historical aliases are NOT accepted
// here — SerializationUtils owns those, because they are a compatibility
// concern rather than a fact about the game's types.
const Entry* findByName(const std::string& name);
const Entry* findByType(EntityType type);

const char* categoryLabel(Category category);

// The categories a level designer can place, in palette order — everything
// except Projectile.
const std::vector<Category>& placeableCategories();

} // namespace EntityCatalogue
