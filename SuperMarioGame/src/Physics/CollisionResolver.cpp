#include "Physics/CollisionResolver.hpp"
#include "Entities/Player.hpp"
#include "Entities/Enemy.hpp"
#include "Entities/Item.hpp"
#include "Entities/Block.hpp"
#include "Entities/Flagpole.hpp"
#include "Entities/Trampoline.hpp"
#include "Entities/PSwitch.hpp"
#include "Entities/Fireball.hpp"
#include "Entities/KoopaTroopa.hpp"
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

    // Category answers "is this a Character?" without a cast.
    Character* character = (entity.getCategory() == EntityCategory::Player ||
                            entity.getCategory() == EntityCategory::Enemy)
                         ? static_cast<Character*>(&entity) : nullptr;

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
        if (info.normal.y == -1.0f) {
            if (character) {
                character->onGround = true;
            } else if (entity.getCategory() == EntityCategory::Item) {
                Item* item = static_cast<Item*>(&entity);
                item->setOnGround(true);
            }
        }
    }
}

void CollisionResolver::resolveEntityVsEntity(Entity& e1, Entity& e2, const CollisionInfo& info) {
    if (!info.collided) return;

    // Ask each side what it is, once, instead of running up to twelve
    // dynamic_casts per pair per frame (audit A-10). Ordering the pair halves the
    // number of cases: (Enemy, Player) is handled as (Player, Enemy) with the
    // collision normal flipped, because the normal points from e1 towards e2.
    const EntityCategory c1 = e1.getCategory();
    const EntityCategory c2 = e2.getCategory();

    auto flipped = [&info] {
        CollisionInfo f = info;
        f.normal = -info.normal;
        return f;
    };

    // Category pairs, in a fixed order so each combination appears once.
    auto pair = [](EntityCategory a, EntityCategory b) {
        return (static_cast<int>(a) << 8) | static_cast<int>(b);
    };

    switch (pair(c1, c2)) {
        case (static_cast<int>(EntityCategory::Player) << 8) | static_cast<int>(EntityCategory::Player):
            resolvePlayerVsPlayer(static_cast<Player&>(e1), static_cast<Player&>(e2), info);
            return;

        case (static_cast<int>(EntityCategory::Player) << 8) | static_cast<int>(EntityCategory::Enemy):
            resolvePlayerVsEnemy(static_cast<Player&>(e1), static_cast<Enemy&>(e2), info);
            return;
        case (static_cast<int>(EntityCategory::Enemy) << 8) | static_cast<int>(EntityCategory::Player):
            resolvePlayerVsEnemy(static_cast<Player&>(e2), static_cast<Enemy&>(e1), flipped());
            return;

        case (static_cast<int>(EntityCategory::Player) << 8) | static_cast<int>(EntityCategory::Item):
            resolvePlayerVsItem(static_cast<Player&>(e1), static_cast<Item&>(e2), info);
            return;
        case (static_cast<int>(EntityCategory::Item) << 8) | static_cast<int>(EntityCategory::Player):
            resolvePlayerVsItem(static_cast<Player&>(e2), static_cast<Item&>(e1), flipped());
            return;

        // Enemies are Characters too, so Enemy-vs-Block lands here as well.
        case (static_cast<int>(EntityCategory::Player) << 8) | static_cast<int>(EntityCategory::Block):
        case (static_cast<int>(EntityCategory::Enemy)  << 8) | static_cast<int>(EntityCategory::Block):
            resolveCharacterVsBlock(static_cast<Character&>(e1), static_cast<Block&>(e2), info);
            return;
        case (static_cast<int>(EntityCategory::Block) << 8) | static_cast<int>(EntityCategory::Player):
        case (static_cast<int>(EntityCategory::Block) << 8) | static_cast<int>(EntityCategory::Enemy):
            resolveCharacterVsBlock(static_cast<Character&>(e2), static_cast<Block&>(e1), flipped());
            return;

        case (static_cast<int>(EntityCategory::Item) << 8) | static_cast<int>(EntityCategory::Block):
            resolveItemVsBlock(static_cast<Item&>(e1), static_cast<Block&>(e2), info);
            return;
        case (static_cast<int>(EntityCategory::Block) << 8) | static_cast<int>(EntityCategory::Item):
            resolveItemVsBlock(static_cast<Item&>(e2), static_cast<Block&>(e1), flipped());
            return;

        case (static_cast<int>(EntityCategory::Projectile) << 8) | static_cast<int>(EntityCategory::Enemy):
            resolveFireballVsEnemy(static_cast<Fireball&>(e1), static_cast<Enemy&>(e2), info);
            return;
        case (static_cast<int>(EntityCategory::Enemy) << 8) | static_cast<int>(EntityCategory::Projectile):
            resolveFireballVsEnemy(static_cast<Fireball&>(e2), static_cast<Enemy&>(e1), flipped());
            return;

        default:
            // Enemy-vs-Enemy is deliberately absent, which is why a kicked shell
            // cannot defeat anything (audit B-8, Member B). Item-vs-Item and
            // Projectile-vs-Block are genuinely no-ops.
            return;
    }
}

void CollisionResolver::resolvePlayerVsEnemy(Player& player, Enemy& enemy, const CollisionInfo& info) {
    if (!enemy.isActive() || enemy.isDeadOrDying()) return;
    if (player.getInvincibilityTimer() > 0.0f) return; // Ignore all enemy contact (damage & stomp) during hurt i-frames

    // A stomp is any contact where the player is descending onto the enemy's upper band.
    // The feet-vs-top test is more forgiving than the raw collision normal at high speed.
    float playerFeetY = player.getBoundingBox().y + player.getBoundingBox().height;
    float enemyTopY = enemy.getBoundingBox().y + 10.0f;
    bool isStomp = (player.getVelocity().y > -50.0f && playerFeetY <= enemyTopY) ||
                   (info.normal.y == -1.0f && player.getVelocity().y >= 0.0f);

    // Touching any unflipped Koopa picks it up and prevents damage
    if (auto koopa = dynamic_cast<KoopaTroopa*>(&enemy)) {
        if (!koopa->isFlipped() && koopa->getState() != KoopaState::ShellHeld) {
            if (isStomp && koopa->getState() == KoopaState::Walking) {
                // Stomping a walking Koopa from above executes the standard stomp bounce
                player.velocity.y = -Constants::STOMP_BOUNCE_FORCE;
                koopa->onStomped();
                player.incrementCombo();
                player.addScore(enemy.getScoreValue() * player.getComboCounter());
                return;
            } else {
                // Touching sideways/below (or touching an idle/kicked shell from any angle) picks it up
                koopa->pickUp(&player);
                player.holdEntity(koopa);
                return;
            }
        }
    }

    if (isStomp) {
        // Player stomped enemy from above
        player.velocity.y = -Constants::STOMP_BOUNCE_FORCE;
        enemy.onStomped();
        player.incrementCombo();
        player.addScore(enemy.getScoreValue() * player.getComboCounter());
    } else {
        // Player hit from the side or below -> damage + knockback away from the enemy.
        // The i-frame guard at the top of this function already gated this path.
        float dx = player.getBoundingBox().getCenter().x - enemy.getBoundingBox().getCenter().x;
        float direction = (dx >= 0.0f) ? 1.0f : -1.0f;
        
        player.velocity.x = direction * Constants::KNOCKBACK_FORCE_X;
        player.velocity.y = -Constants::KNOCKBACK_FORCE_Y;
        
        player.takeDamage(1);
    }
}


void CollisionResolver::resolvePlayerVsItem(Player& player, Item& item, const CollisionInfo& info) {
    if (player.getInvincibilityTimer() > 0.0f) return; // Ignore item collisions / collection while hurt invincible

    if (!item.isCollected()) {
        if (auto trampoline = dynamic_cast<Trampoline*>(&item)) {
            // Trampoline only bounces player when landed on from above
            if (info.normal.y == -1.0f || player.getVelocity().y >= 0.0f) {
                trampoline->activate(player);
            } else {
                // Side / bottom collision acts as solid box displacement
                player.position.x += info.normal.x * (info.overlap.x + 0.01f);
                player.position.y += info.normal.y * (info.overlap.y + 0.01f);
                if (info.normal.x != 0.0f) player.velocity.x = 0.0f;
                if (info.normal.y != 0.0f) player.velocity.y = 0.0f;
            }
            return;
        }

        if (auto pswitch = dynamic_cast<PSwitch*>(&item)) {
            pswitch->activate(player);
            pswitch->collect();
            // Solid collision response so player lands on top of squished P-Switch
            player.position.x += info.normal.x * (info.overlap.x + 0.01f);
            player.position.y += info.normal.y * (info.overlap.y + 0.01f);
            if (info.normal.x != 0.0f) player.velocity.x = 0.0f;
            if (info.normal.y != 0.0f) player.velocity.y = 0.0f;
            if (info.normal.y == -1.0f) player.onGround = true;
            return;
        }

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

void CollisionResolver::resolveCharacterVsBlock(Character& character, Block& block, const CollisionInfo& info) {
    if (!info.collided || !block.isActive()) return;

    // Handle flagpole trigger specifically (subclass of Block)
    if (auto flagpole = dynamic_cast<Flagpole*>(&block)) {
        if (character.getCategory() == EntityCategory::Player) {
            Player* player = static_cast<Player*>(&character);
            flagpole->onPlayerCollision(*player, character.position.y);
        }
        return; // Flagpole does not physically block the character
    }

    // Push character out of the block
    character.position.x += info.normal.x * info.overlap.x;
    character.position.y += info.normal.y * info.overlap.y;
    character.boundingBox.x = character.position.x;
    character.boundingBox.y = character.position.y;

    // Cancel velocity along collision normal
    if (info.normal.x != 0.0f) {
        character.velocity.x = 0.0f;
        character.onWall = true;
    }
    if (info.normal.y != 0.0f) {
        character.velocity.y = 0.0f;
        if (info.normal.y == -1.0f) { // Character landed on top of the block
            character.onGround = true;
        }
        if (info.normal.y == 1.0f) { // Character hit ceiling (block from below)
            if (character.getCategory() == EntityCategory::Player) {
                block.onHitFromBelow(static_cast<Player&>(character));
            }
        }
    }
}

void CollisionResolver::resolveFireballVsEnemy(Fireball& fireball, Enemy& enemy, const CollisionInfo& info) {
    if (fireball.isActive() && enemy.isActive()) {
        enemy.onHitByFireball();
        fireball.destroy();
    }
}

void CollisionResolver::resolveItemVsBlock(Item& item, Block& block, const CollisionInfo& info) {
    if (!info.collided || !block.isActive()) return;

    item.position.x += info.normal.x * info.overlap.x;
    item.position.y += info.normal.y * info.overlap.y;
    item.boundingBox.x = item.position.x;
    item.boundingBox.y = item.position.y;

    if (info.normal.x != 0.0f) {
        item.velocity.x = -item.velocity.x;
    }
    if (info.normal.y != 0.0f) {
        item.velocity.y = 0.0f;
        if (info.normal.y == -1.0f) {
            item.setOnGround(true);
        }
    }
}

