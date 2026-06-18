#include "Physics/CollisionResolver.hpp"
#include "Entities/Player.hpp"
#include "Entities/Enemy.hpp"
#include "Entities/Item.hpp"
#include "Utils/Constants.hpp"
#include <cmath>

void CollisionResolver::resolveEntityVsTile(Entity& entity, const CollisionInfo& info) {
    if (!info.collided) return;

    // Push the entity out of the tile
    entity.position.x += info.normal.x * info.overlap.x;
    entity.position.y += info.normal.y * info.overlap.y;

    auto character = dynamic_cast<Character*>(&entity);

    // Cancel velocity along the collision normal
    if (info.normal.x != 0.0f) {
        entity.velocity.x = 0.0f;
        if (character) {
            character->onWall = true;
        }
    }
    if (info.normal.y != 0.0f) {
        entity.velocity.y = 0.0f;
        if (info.normal.y == -1.0f && character) {
            character->onGround = true;
        }
    }
}

void CollisionResolver::resolveEntityVsEntity(Entity& e1, Entity& e2, const CollisionInfo& info) {
    if (!info.collided) return;

    auto player1 = dynamic_cast<Player*>(&e1);
    auto enemy2 = dynamic_cast<Enemy*>(&e2);
    if (player1 && enemy2) {
        resolvePlayerVsEnemy(*player1, *enemy2, info);
        return;
    }

    auto enemy1 = dynamic_cast<Enemy*>(&e1);
    auto player2 = dynamic_cast<Player*>(&e2);
    if (enemy1 && player2) {
        CollisionInfo flippedInfo = info;
        flippedInfo.normal = -info.normal;
        resolvePlayerVsEnemy(*player2, *enemy1, flippedInfo);
        return;
    }

    auto playerVsItem1 = dynamic_cast<Player*>(&e1);
    auto item2 = dynamic_cast<Item*>(&e2);
    if (playerVsItem1 && item2) {
        resolvePlayerVsItem(*playerVsItem1, *item2, info);
        return;
    }

    auto item1 = dynamic_cast<Item*>(&e1);
    auto playerVsItem2 = dynamic_cast<Player*>(&e2);
    if (item1 && playerVsItem2) {
        CollisionInfo flippedInfo = info;
        flippedInfo.normal = -info.normal;
        resolvePlayerVsItem(*playerVsItem2, *item1, flippedInfo);
        return;
    }
}

void CollisionResolver::resolvePlayerVsEnemy(Player& player, Enemy& enemy, const CollisionInfo& info) {
    if (info.normal.y == -1.0f) {
        // Player stomped enemy from above
        player.velocity.y = -300.0f; // Stomp bounce force
        enemy.onStomped();
        // Reset player combo / handle combos (this will be handled by EventBus / player states in Phase 3)
    } else {
        // Player hit from the side or below -> takes damage (unless invincible/mega)
        // If player is hit, we apply knockback away from the enemy
        float dx = player.getBoundingBox().getCenter().x - enemy.getBoundingBox().getCenter().x;
        float direction = (dx >= 0.0f) ? 1.0f : -1.0f;
        
        player.velocity.x = direction * 150.0f;
        player.velocity.y = -100.0f;
        
        player.takeDamage(1);
    }
}

void CollisionResolver::resolvePlayerVsItem(Player& player, Item& item, const CollisionInfo& info) {
    if (!item.collected) {
        item.activate(player);
        item.collect();
    }
}
