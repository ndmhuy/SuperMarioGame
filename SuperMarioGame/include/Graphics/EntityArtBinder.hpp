#pragma once

class Entity;
class SpriteSheet;

// Routes an entity to the atlas its class draws from.
//
// setupAnimations() lives on Player, Enemy, Item and Block rather than on
// Entity, and which of the four atlases a given class wants is knowledge that
// existed in exactly one place: PlayingState::wireEntityAnimations(). The level
// editor screen has to make the same decision for entities it places, and an
// entity that never gets its sheet has no sprite at all — Entity::render falls
// through to drawPlaceholder() and draws a flat coloured box. That is why every
// entity dropped by the editor looked broken.
//
// This is that dispatcher, so there is one copy of it (g-rule-22) and both
// screens are wrong or right together.
class EntityArtBinder {
public:
    // Non-owning. The sheets belong to the state that loaded them and must
    // outlive this binder; null sheets are tolerated (the harnesses run without
    // an atlas) and simply leave the matching entities unbound.
    void setSheets(const SpriteSheet* player, const SpriteSheet* enemy,
                   const SpriteSheet* item, const SpriteSheet* scenery);

    // Gives `entity` its animations. Safe on null. Complains on stderr for a
    // type that matches none of the four branches, because silence there is how
    // a new entity category ships invisible.
    void bind(Entity* entity) const;

private:
    const SpriteSheet* m_player = nullptr;
    const SpriteSheet* m_enemy = nullptr;
    const SpriteSheet* m_item = nullptr;
    const SpriteSheet* m_scenery = nullptr;
};
