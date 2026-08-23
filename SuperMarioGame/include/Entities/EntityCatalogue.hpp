#pragma once

#include "Entities/EntityFactory.hpp"

#include <string>
#include <vector>

// The one table that knows every entity type by name.
//
// Why this exists
// ---------------
// The same list of entity types was written out by hand in three places, and
// they had all drifted from each other:
//
//   - EntityFactory::create()          knew 40 types
//   - SerializationUtils::parse...()   knew 40 names
//   - MapEditor's palette              knew 16, none of them enemies
//
// So the level editor — the feature whose entire purpose is placing entities —
// could not place a Goomba, a Koopa, a pipe, a question block or a flagpole,
// and nothing anywhere failed to say so. Adding a type meant remembering three
// files, and forgetting the third was silent.
//
// g-rule-22: a list the code already contains must be derived, not hand-synced.
// This is that list. The parser and the palette both read it, and
// verify_regressions asserts that every entry is creatable by the factory and
// round-trips through its own name — so a type added here and nowhere else
// fails the build rather than quietly disappearing from the editor.
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

struct Entry {
    EntityType  type;
    // The canonical serialised name. MUST match the class's getTypeName(), or
    // saving a level and loading it back produces a different entity.
    std::string name;
    // What the editor shows a human.
    std::string label;
    Category    category;
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
