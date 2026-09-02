#include "Entities/EntityFactory.hpp"
#include "Entities/EntityCatalogue.hpp"

// Only three headers now, where there used to be forty-five. Everything this
// file used to include, it included in order to CONSTRUCT — and construction is
// the registry's job (see EntityCatalogue::Entry::create). What is left here is
// what the factory actually owns: the tuning pass, and the one pooled type that
// is built with a velocity rather than a position alone.
#include "Entities/Enemy.hpp"
#include "Entities/Fireball.hpp"
#include "Utils/EntityConfig.hpp"

namespace {

// Applies whatever assets/config/entities.json has to say about an entity, on
// top of what its constructor set (task 10.2). Values the file omits are left
// alone, so a partial entry cannot zero out something the code knew.
void applyConfig(Entity* entity) {
    auto* enemy = dynamic_cast<Enemy*>(entity);
    if (!enemy) return;   // only enemies are tuned from the file today

    const EntityConfigEntry* config = EntityConfig::find(enemy->getTypeName());
    if (!config) return;

    if (config->speed > 0.0f) enemy->setSpeed(config->speed);
    if (config->score >= 0)   enemy->setScoreValue(config->score);
}

} // namespace

std::unique_ptr<Entity> EntityFactory::create(EntityType type, sf::Vector2f position) {
    std::unique_ptr<Entity> entity = createUnconfigured(type, position);
    applyConfig(entity.get());
    return entity;
}

std::unique_ptr<Entity> EntityFactory::createUnconfigured(EntityType type, sf::Vector2f position) {
    // This was a 42-case switch that had to be edited in lockstep with
    // EntityCatalogue's 42-row table. The two agreed only because a regression
    // test noticed when they did not; now there is one list, and this function
    // is the two-line consequence of that.
    //
    // A type with no registry entry yields nullptr rather than an assert: level
    // files are hand-edited and callers already handle a null (PlaceEntityCommand
    // and LevelLoader both check). verify_r21_entity_registry is what makes the
    // omission loud, by walking every EntityType enumerator.
    const EntityCatalogue::Entry* entry = EntityCatalogue::findByType(type);
    if (!entry || !entry->create) return nullptr;
    return entry->create(position);
}

std::unique_ptr<Entity> EntityFactory::createFireball(sf::Vector2f position, sf::Vector2f velocity) {
    return std::make_unique<Fireball>(position, velocity);
}