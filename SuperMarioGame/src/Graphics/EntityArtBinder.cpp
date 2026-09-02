#include "Graphics/EntityArtBinder.hpp"

#include "Entities/Block.hpp"
#include "Entities/BridgeAxe.hpp"
#include "Entities/Enemy.hpp"
#include "Entities/Entity.hpp"
#include "Entities/Item.hpp"
#include "Entities/Player.hpp"
#include "Entities/Projectile.hpp"
#include "Entities/StarCoin.hpp"

#include <iostream>

void EntityArtBinder::setSheets(const SpriteSheet* player, const SpriteSheet* enemy,
                                const SpriteSheet* item, const SpriteSheet* scenery) {
    m_player = player;
    m_enemy = enemy;
    m_item = item;
    m_scenery = scenery;
}

void EntityArtBinder::bind(Entity* entity) const {
    if (!entity) return;

    if (auto* p = dynamic_cast<Player*>(entity)) {
        if (m_player) p->setupAnimations(m_player);
    } else if (auto* e = dynamic_cast<Enemy*>(entity)) {
        if (m_enemy) e->setupAnimations(m_enemy);
    } else if (auto* proj = dynamic_cast<Projectile*>(entity)) {
        // Hammers and Bowser's fire breath both live in the enemy/projectile
        // atlas. One branch covers every projectile, so a new one is wired by
        // existing.
        if (m_enemy) proj->setupAnimations(m_enemy);
    } else if (dynamic_cast<StarCoin*>(entity) || dynamic_cast<BridgeAxe*>(entity)) {
        // Two Items whose art is in the world/scenery atlas rather than the item
        // one: StarCoin's big_coin_0..2, and BridgeAxe's axe_0..2. Routed by the
        // generic Item branch below they find no frame, and drawSprite bails on a
        // zero-size sprite — so they drew a placeholder rectangle.
        if (auto* sceneryItem = dynamic_cast<Item*>(entity)) {
            if (m_scenery) sceneryItem->setupAnimations(m_scenery);
        }
    } else if (auto* i = dynamic_cast<Item*>(entity)) {
        if (m_item) i->setupAnimations(m_item);
    } else if (auto* b = dynamic_cast<Block*>(entity)) {
        if (m_scenery) b->setupAnimations(m_scenery);
    } else {
        std::cerr << "[EntityArtBinder] Unknown entity type '" << entity->getTypeName()
                  << "', no atlas bound." << std::endl;
    }
}
