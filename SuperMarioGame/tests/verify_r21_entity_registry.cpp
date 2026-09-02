// verify_r21_entity_registry.cpp — R21 defect 11, the structural half.
//
// The defect: 24 of the map editor's 40 palette buttons placed nothing. The
// palette was generated from EntityCatalogue while PlaceEntityCommand ran its
// own if/else chain over 16 name strings and never called EntityFactory. Four
// hand-maintained lists of entity types existed and they had drifted.
//
// Wiring the palette to the factory fixed the symptom. It left TWO lists — the
// catalogue's table and the factory's 42-case switch — agreeing only because
// testEntityCatalogueIsCompleteAndRoundTrips in verify_regressions noticed when
// they stopped. That test walks the CATALOGUE, so it can only ever check the
// types someone remembered to put in the catalogue; a type added to the enum
// and to nothing else was invisible to it.
//
// This harness walks the ENUM instead. EntityType::Count makes that possible:
// every enumerator from 0 to Count must resolve to a registry entry whose
// creator returns a live entity. That is the assertion the release needed and
// did not have — forgetting to register a new type now fails the suite rather
// than shipping a palette button that silently places nothing.
//
// Window-free and cheap, so CI can run it.
//
// Run via:  ctest -R r21_entity_registry --output-on-failure

#include "Entities/Entity.hpp"
#include "Entities/EntityCatalogue.hpp"
#include "Entities/EntityFactory.hpp"
#include "Entities/Enemy.hpp"
#include "Utils/EntityConfig.hpp"
#include "Utils/SerializationUtils.hpp"
#include "TestSaveSandbox.hpp"

#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace {

int g_failures = 0;
int g_checks   = 0;

void check(bool condition, const std::string& what) {
    ++g_checks;
    if (condition) {
        std::cout << "  [ ok ] " << what << "\n";
    } else {
        std::cout << "  [FAIL] " << what << "\n";
        ++g_failures;
    }
}

void section(const std::string& title) {
    std::cout << "\n=== " << title << " ===\n";
}

// A type's name for a failure message, from the enum value alone — the registry
// is exactly what may be missing, so this cannot go through it.
std::string typeNumber(int value) {
    return "EntityType #" + std::to_string(value);
}

// ---------------------------------------------------------------------------
// 1. Every enumerator is registered and builds.
//
// The point of the whole exercise. Nothing here consults the catalogue's own
// list of entries: it counts up through the enum, so a type the registry has
// never heard of is precisely what it catches.
// ---------------------------------------------------------------------------
void testEveryEnumeratorResolvesToACreator() {
    section("registry  every EntityType enumerator builds a real entity");

    const int typeCount = static_cast<int>(EntityType::Count);
    check(typeCount > 0, "EntityType::Count is the end of the enum, not the start");

    int unregistered = 0, nullCreator = 0, builtNothing = 0, unnamed = 0;
    std::string firstUnregistered, firstNullCreator, firstBuiltNothing, firstUnnamed;

    for (int value = 0; value < typeCount; ++value) {
        const EntityType type = static_cast<EntityType>(value);

        const EntityCatalogue::Entry* entry = EntityCatalogue::findByType(type);
        if (!entry) {
            if (unregistered++ == 0) firstUnregistered = typeNumber(value);
            continue;
        }

        // A registration carrying a null creator would pass every metadata
        // check in verify_regressions and still place nothing.
        if (!entry->create) {
            if (nullCreator++ == 0) firstNullCreator = entry->name;
            continue;
        }

        // Through EntityFactory rather than entry->create directly: the factory
        // is what every caller in the game uses, and it is the layer that
        // applies assets/config/entities.json. If it stopped delegating to the
        // registry, this loop would still be the thing that noticed.
        std::unique_ptr<Entity> made = EntityFactory::create(type, {96.0f, 96.0f});
        if (!made) {
            if (builtNothing++ == 0) firstBuiltNothing = entry->name;
            continue;
        }

        // An entity that cannot say what it is cannot be saved to a level file.
        if (made->getTypeName().empty()) {
            if (unnamed++ == 0) firstUnnamed = entry->name;
        }
    }

    check(unregistered == 0,
          "every EntityType enumerator has a registry entry" +
          (unregistered ? " (" + std::to_string(unregistered) + " missing, first: " +
                          firstUnregistered + ")"
                        : std::string()));
    check(nullCreator == 0,
          "no registration carries a null creator" +
          (nullCreator ? " (first: " + firstNullCreator + ")" : std::string()));
    check(builtNothing == 0,
          "EntityFactory::create returns a live entity for every enumerator" +
          (builtNothing ? " (first: " + firstBuiltNothing + ")" : std::string()));
    check(unnamed == 0,
          "every built entity reports a non-empty type name" +
          (unnamed ? " (first: " + firstUnnamed + ")" : std::string()));

    // The registry must also not contain more entries than the enum has types,
    // which would mean a duplicate or a stale row.
    check(EntityCatalogue::all().size() == static_cast<std::size_t>(typeCount),
          "the registry has exactly one entry per enumerator (" +
          std::to_string(EntityCatalogue::all().size()) + " entries, " +
          std::to_string(typeCount) + " types)");
}

// ---------------------------------------------------------------------------
// 2. Names are unique, types are unique, and the round trip closes.
//
// A duplicate name means a level file can name something the registry answers
// two ways; a duplicate type means findByType picks a winner arbitrarily. Both
// are silent at runtime.
// ---------------------------------------------------------------------------
void testNamesAndTypesAreUniqueAndRoundTrip() {
    section("registry  names and types are unique and survive the round trip");

    std::set<std::string> names;
    std::set<int>         types;
    std::string firstDuplicateName, firstDuplicateType;
    int duplicateNames = 0, duplicateTypes = 0;

    for (const EntityCatalogue::Entry& entry : EntityCatalogue::all()) {
        if (!names.insert(entry.name).second) {
            if (duplicateNames++ == 0) firstDuplicateName = entry.name;
        }
        if (!types.insert(static_cast<int>(entry.type)).second) {
            if (duplicateTypes++ == 0) firstDuplicateType = entry.name;
        }
    }

    check(duplicateNames == 0,
          "no two entries share a serialised name" +
          (duplicateNames ? " (first: " + firstDuplicateName + ")" : std::string()));
    check(duplicateTypes == 0,
          "no two entries share an EntityType" +
          (duplicateTypes ? " (first: " + firstDuplicateType + ")" : std::string()));

    // name -> type -> name. The parser must resolve through the same registry,
    // or a level saved from the editor loads back as something else.
    int badParse = 0, badName = 0, notFound = 0;
    std::string firstBadParse, firstBadName, firstNotFound;

    for (const EntityCatalogue::Entry& entry : EntityCatalogue::all()) {
        if (SerializationUtils::parseEntityTypeName(entry.name) != entry.type) {
            if (badParse++ == 0) firstBadParse = entry.name;
        }

        const EntityCatalogue::Entry* back = EntityCatalogue::findByName(entry.name);
        if (!back) {
            if (notFound++ == 0) firstNotFound = entry.name;
            continue;
        }
        if (back->type != entry.type || back->name != entry.name) {
            if (badName++ == 0) firstBadName = entry.name;
        }
    }

    check(badParse == 0,
          "every registered name parses back to its own type" +
          (badParse ? " (first: " + firstBadParse + ")" : std::string()));
    check(notFound == 0,
          "every registered name is findable by name" +
          (notFound ? " (first: " + firstNotFound + ")" : std::string()));
    check(badName == 0,
          "name -> type -> name closes on the same entry" +
          (badName ? " (first: " + firstBadName + ")" : std::string()));

    // findByName is the level loader's entry point and must reject rather than
    // guess: an unknown name resolving to a real entry would load silently as
    // the wrong entity.
    check(EntityCatalogue::findByName("no_such_entity") == nullptr,
          "an unknown name resolves to no entry at all");
    check(EntityCatalogue::findByType(EntityType::Count) == nullptr,
          "the Count sentinel is not itself a registered type");
}

// ---------------------------------------------------------------------------
// 3. The palette promises only what the factory can keep.
//
// This is defect 11 stated directly: a button in placeableCategories() whose
// type the factory cannot build is a button that places nothing.
// ---------------------------------------------------------------------------
void testPaletteContainsNothingUnbuildable() {
    section("palette  every button places a real entity");

    int placeable = 0, unbuildable = 0, unlabelled = 0;
    std::string firstUnbuildable, firstUnlabelled;
    std::set<int> paletteTypes;

    for (EntityCatalogue::Category category : EntityCatalogue::placeableCategories()) {
        const std::vector<const EntityCatalogue::Entry*> entries =
            EntityCatalogue::inCategory(category);
        for (const EntityCatalogue::Entry* entry : entries) {
            ++placeable;
            paletteTypes.insert(static_cast<int>(entry->type));

            std::unique_ptr<Entity> made = EntityFactory::create(entry->type, {96.0f, 96.0f});
            if (!made) {
                if (unbuildable++ == 0) firstUnbuildable = entry->name;
            }
            // A blank button is unusable even when it works.
            if (entry->label.empty()) {
                if (unlabelled++ == 0) firstUnlabelled = entry->name;
            }
        }
    }

    check(unbuildable == 0,
          "no palette button names a type the factory cannot build" +
          (unbuildable ? " (first: " + firstUnbuildable + ")" : std::string()));
    check(unlabelled == 0,
          "every palette button has a display label" +
          (unlabelled ? " (first: " + firstUnlabelled + ")" : std::string()));

    // The original complaint in numbers: 16 of 40 buttons worked. Rather than
    // asserting a magic count, require the palette to cover every registered
    // type that is not a player or a projectile — so adding an authorable type
    // and leaving it out of a placeable category is itself a failure, and
    // adding one correctly does not have to update a number here.
    int missingFromPalette = 0;
    std::string firstMissing;
    for (const EntityCatalogue::Entry& entry : EntityCatalogue::all()) {
        if (entry.category == EntityCatalogue::Category::Player ||
            entry.category == EntityCatalogue::Category::Projectile) {
            continue;
        }
        if (paletteTypes.count(static_cast<int>(entry.type)) == 0) {
            if (missingFromPalette++ == 0) firstMissing = entry.name;
        }
    }
    check(missingFromPalette == 0,
          "every authorable registered type has a palette button (" +
          std::to_string(placeable) + " buttons)" +
          (missingFromPalette ? " (first missing: " + firstMissing + ")" : std::string()));

    // Projectiles are spawned by their thrower, never authored: one in a level
    // file would appear frozen in mid-air.
    for (const EntityCatalogue::Entry& entry : EntityCatalogue::all()) {
        if (entry.category != EntityCatalogue::Category::Projectile) continue;
        check(paletteTypes.count(static_cast<int>(entry.type)) == 0,
              "the palette does not offer the runtime-only " + entry.name);
    }

    // Players are absent by design: saveLevel turns a player into the level's
    // spawnPoint and writes no entity for it, so a palette player would promise
    // something the format cannot keep.
    bool playersOffered = false;
    for (EntityCatalogue::Category category : EntityCatalogue::placeableCategories()) {
        if (category == EntityCatalogue::Category::Player) playersOffered = true;
    }
    check(!playersOffered, "the palette does not offer players (the Spawn Point tool does)");

    // Every placeable category must actually have buttons in it, or the editor
    // draws an empty drawer.
    for (EntityCatalogue::Category category : EntityCatalogue::placeableCategories()) {
        check(!EntityCatalogue::inCategory(category).empty(),
              std::string("the \"") + EntityCatalogue::categoryLabel(category) +
              "\" drawer is not empty");
    }
}

// ---------------------------------------------------------------------------
// 4. The factory still applies assets/config/entities.json.
//
// Moving construction into the registry must not take the tuning pass with it.
// create() and createUnconfigured() differ only by that pass, so comparing them
// is what proves it still runs.
// ---------------------------------------------------------------------------
void testTuningFileStillApplies() {
    section("factory  entities.json tuning survives the registry move");

    int compared = 0;
    for (const EntityCatalogue::Entry& entry : EntityCatalogue::all()) {
        std::unique_ptr<Entity> raw    = EntityFactory::createUnconfigured(entry.type, {96.0f, 96.0f});
        std::unique_ptr<Entity> tuned  = EntityFactory::create(entry.type, {96.0f, 96.0f});
        if (!raw || !tuned) continue;
        ++compared;

        // Both paths must produce the same KIND of thing — the tuning pass
        // adjusts an entity, it never substitutes one.
        if (raw->getTypeName() != tuned->getTypeName()) {
            check(false, "create and createUnconfigured disagree about " + entry.name);
            return;
        }
    }
    check(compared == static_cast<int>(EntityCatalogue::all().size()),
          "both construction paths build every registered type (" +
          std::to_string(compared) + ")");

    // The tuning file must still be reaching entities built through the
    // registry. Score is the observable half — Enemy exposes getScoreValue()
    // but deliberately exposes no speed getter, so speed can only be set, not
    // read back.
    check(EntityConfig::entryCount() > 0,
          "assets/config/entities.json is being read at all (" +
          std::to_string(EntityConfig::entryCount()) + " entries)");

    int covered = 0, disagreed = 0;
    std::string firstDisagreement;
    for (const EntityCatalogue::Entry& entry : EntityCatalogue::all()) {
        std::unique_ptr<Entity> made = EntityFactory::create(entry.type, {96.0f, 96.0f});
        auto* enemy = dynamic_cast<Enemy*>(made.get());
        if (!enemy) continue;   // the file only tunes enemies today

        const EntityConfigEntry* config = EntityConfig::find(entry.name);
        if (!config || config->score < 0) continue;
        ++covered;

        if (enemy->getScoreValue() != config->score) {
            if (disagreed++ == 0) {
                firstDisagreement = entry.name + " is worth " +
                                    std::to_string(enemy->getScoreValue()) +
                                    ", the file says " + std::to_string(config->score);
            }
        }
    }

    check(covered > 0, "the file covers at least one registered enemy (" +
                       std::to_string(covered) + ")");
    check(disagreed == 0,
          "every tuned enemy leaves the factory with the file's score" +
          (disagreed ? " (" + firstDisagreement + ")" : std::string()));

    // Honest about what the check above can and cannot prove: the shipped file
    // was seeded from the constructors' own values, so an entity built without
    // the tuning pass agrees with it by coincidence. The assertion catches the
    // file and the code diverging — it would NOT catch applyConfig being
    // deleted while the file still matches. Deliberately not solved by writing
    // a doctored entities.json from here: the suite must not edit assets.
    std::cout << "         (note: file values are seeded from the constructors,"
              << " so this proves agreement, not that the pass mutated anything)\n";
}

} // namespace

int main() {
    // Every harness gets its own scratch save directory: a ctest run once
    // deleted the real saves/highscores.json (g-rule-13). Nothing here writes a
    // save, but the guard costs nothing and cannot be forgotten later.
    TestSaveSandbox sandbox("verify_r21_entity_registry");

    std::cout << "=========================================\n";
    std::cout << " R21 entity registry — one list, checked against the enum\n";
    std::cout << "=========================================\n";

    testEveryEnumeratorResolvesToACreator();
    testNamesAndTypesAreUniqueAndRoundTrip();
    testPaletteContainsNothingUnbuildable();
    testTuningFileStillApplies();

    std::cout << "\n----------------------------------------\n";
    std::cout << g_checks - g_failures << " / " << g_checks << " checks passed\n";
    if (g_failures > 0) {
        std::cout << g_failures << " FAILURE(S)\n";
        return 1;
    }
    std::cout << "ALL PASS\n";
    return 0;
}
