#include "Physics/CollisionResolver.hpp"
#include "Entities/Player.hpp"
#include "Entities/Enemy.hpp"
#include "Entities/Item.hpp"
#include "Utils/Constants.hpp"
#include <cmath>

void CollisionResolver::resolveEntityVsTile(Entity& entity, const CollisionInfo& info) {
    if (!info.collided) return;

    // Push the entity out of the tile with a tiny epsilon buffer (0.01px) to prevent float precision rounding overlap triggers
    const float EPSILON = 0.01f;
    entity.position.x += info.normal.x * (info.overlap.x + (info.normal.x != 0.0f ? EPSILON : 0.0f));
    entity.position.y += info.normal.y * (info.overlap.y + (info.normal.y != 0.0f ? EPSILON : 0.0f));

    // Sync bounding box coordinates immediately
    entity.boundingBox.x = entity.position.x;
    entity.boundingBox.y = entity.position.y;

    auto character = dynamic_cast<Character*>(&entity);

    // Cancel velocity along the collision normal
    if (info.normal.x != 0.0f) {
        entity.velocity.x = 0.0f;
        if (character) {
            character->onWall = true;

            // Air wall friction & sliding mechanics
            if (!character->onGround) {
                if (entity.velocity.y < 0.0f) {
                    entity.velocity.y = 0.0f; // Eliminate upward momentum on wall impact
                } else if (entity.velocity.y > Constants::WALL_SLIDE_SPEED) {
                    entity.velocity.y = Constants::WALL_SLIDE_SPEED; // Cap downward slide velocity
                }
            }
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
    auto player2 = dynamic_cast<Player*>(&e2);

    // 1. Player vs Player
    if (player1 && player2) {
        resolvePlayerVsPlayer(*player1, *player2, info);
        return;
    }

    // 2. Player vs Enemy
    auto enemy1 = dynamic_cast<Enemy*>(&e1);
    auto enemy2 = dynamic_cast<Enemy*>(&e2);

    if (player1 && enemy2) {
        resolvePlayerVsEnemy(*player1, *enemy2, info);
        return;
    }
    if (enemy1 && player2) {
        CollisionInfo flippedInfo = info;
        flippedInfo.normal = -info.normal;
        resolvePlayerVsEnemy(*player2, *enemy1, flippedInfo);
        return;
    }

    // 3. Player vs Item
    auto item1 = dynamic_cast<Item*>(&e1);
    auto item2 = dynamic_cast<Item*>(&e2);

    if (player1 && item2) {
        resolvePlayerVsItem(*player1, *item2, info);
        return;
    }
    if (item1 && player2) {
        CollisionInfo flippedInfo = info;
        flippedInfo.normal = -info.normal;
        resolvePlayerVsItem(*player2, *item1, flippedInfo);
        return;
    }
}

void CollisionResolver::resolvePlayerVsEnemy(Player& player, Enemy& enemy, const CollisionInfo& info) {
    if (info.normal.y == -1.0f) {
        // Player stomped enemy from above
        player.velocity.y = -Constants::STOMP_BOUNCE_FORCE; // Stomp bounce force
        enemy.onStomped();
        // Reset player combo / handle combos (this will be handled by EventBus / player states in Phase 3)
    } else {
        // Player hit from the side or below -> takes damage (unless invincible/mega)
        // If player is hit, we apply knockback away from the enemy
        float dx = player.getBoundingBox().getCenter().x - enemy.getBoundingBox().getCenter().x;
        float direction = (dx >= 0.0f) ? 1.0f : -1.0f;
        
        player.velocity.x = direction * Constants::KNOCKBACK_FORCE_X;
        player.velocity.y = -Constants::KNOCKBACK_FORCE_Y;
        
        player.takeDamage(1);
    }
}

void CollisionResolver::resolvePlayerVsItem(Player& player, Item& item, const CollisionInfo& info) {
    if (!item.isCollected()) {
        item.activate(player);
        item.collect();
    }
}

void CollisionResolver::resolvePlayerVsPlayer(Player& p1, Player& p2, const CollisionInfo& info) {
    // Both are dynamic characters; resolve collision by pushing both back by 50% of the overlap
    p1.position.x += info.normal.x * info.overlap.x * 0.5f;
    p1.position.y += info.normal.y * info.overlap.y * 0.5f;

    p2.position.x -= info.normal.x * info.overlap.x * 0.5f;
    p2.position.y -= info.normal.y * info.overlap.y * 0.5f;

    // Sync both player bounding boxes
    p1.boundingBox.x = p1.position.x;
    p1.boundingBox.y = p1.position.y;
    p2.boundingBox.x = p2.position.x;
    p2.boundingBox.y = p2.position.y;

    if (info.normal.y == -1.0f) {
        // p1 is above p2 (p1 stomps p2's head)
        p1.velocity.y = -Constants::PLAYER_BOUNCE_FORCE; // p1 bounces up
        p2.velocity.y = Constants::PLAYER_PUSH_DOWN_FORCE;  // p2 is pushed down
    } else if (info.normal.y == 1.0f) {
        // p2 is above p1 (p2 stomps p1's head)
        p2.velocity.y = -Constants::PLAYER_BOUNCE_FORCE; // p2 bounces up
        p1.velocity.y = Constants::PLAYER_PUSH_DOWN_FORCE;  // p1 is pushed down
    } else if (info.normal.x != 0.0f) {
        // Side hit: distribute velocities horizontally to push them apart
        float avgVx = (p1.velocity.x + p2.velocity.x) / 2.0f;
        p1.velocity.x = avgVx + info.normal.x * Constants::PLAYER_PUSH_SIDE_FORCE;
        p2.velocity.x = avgVx - info.normal.x * Constants::PLAYER_PUSH_SIDE_FORCE;
    }
}
