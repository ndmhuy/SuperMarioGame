#include "Physics/CollisionResolver.hpp"
#include "Entities/Player.hpp"
#include "Entities/Enemy.hpp"
#include "Entities/Item.hpp"
#include "Entities/Block.hpp"
#include "Entities/HiddenBlock.hpp"
#include "Entities/Flagpole.hpp"
#include "Entities/Trampoline.hpp"
#include "Entities/PSwitch.hpp"
#include "Entities/POWBlock.hpp"
#include "Entities/Fireball.hpp"
#include "Entities/Projectile.hpp"
#include "Entities/Hammer.hpp"
#include "Entities/KoopaTroopa.hpp"
#include "Entities/Boss.hpp"
#include "Core/SoundManager.hpp"
#include "Core/InputManager.hpp"
#include "Core/Game.hpp"
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

            // Wall slide: falling slowly while pressed against a wall.
            //
            // Two things here were wrong, and together they made it impossible
            // to jump while walking into a wall — so no character could climb
            // any step it was touching.
            //
            //  1. The airborne test asked onGround, which PhysicsEngine has
            //     already cleared for this frame and does not set again until
            //     the Y pass, *after* this one. It therefore read false for a
            //     character standing firmly on the ground, and the rule fired
            //     every time anyone walked into a wall. wasOnGround is the
            //     state this frame started in, which is the real question.
            //
            //  2. It cancelled *vertical* velocity on a purely *horizontal*
            //     collision. A wall is vertical; touching one says nothing
            //     about upward motion, and moving up alongside a wall is
            //     ordinary platforming. A genuine ceiling is what the Y pass's
            //     normal.y == 1 case is for, and it already handles it.
            //
            // So only the downward cap survives, and only when genuinely
            // airborne. Found by tools/eval_level.cpp: an AI walked into the
            // 3-tile step at column 25 of level_1, jumped correctly every other
            // frame for 145 seconds, and never moved a pixel, because the jump
            // velocity was erased here before it could ever be integrated.
            if (!character->onGround && !character->wasOnGround) {
                if (entity.velocity.y > Constants::WALL_SLIDE_SPEED) {
                    entity.velocity.y = Constants::WALL_SLIDE_SPEED;
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

    // A contact hazard — Shadow Mario — takes part in exactly one pairing: the
    // one where it touches a real player. Anything else passes straight through
    // it, so it cannot farm the level for the human it is chasing.
    if (e1.isContactHazard() || e2.isContactHazard()) {
        const bool bothPlayers = (c1 == EntityCategory::Player && c2 == EntityCategory::Player);
        // Two hazards cannot meaningfully collide, and there is only ever one.
        if (!bothPlayers || (e1.isContactHazard() && e2.isContactHazard())) return;
    }

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

        // Every Projectile answers for itself who it may hurt, so these four
        // cases name no concrete projectile type. They used to static_cast
        // straight to Fireball, which meant a thrown hammer killed enemies.
        case (static_cast<int>(EntityCategory::Projectile) << 8) | static_cast<int>(EntityCategory::Enemy):
            resolveProjectileVsEnemy(static_cast<Projectile&>(e1), static_cast<Enemy&>(e2));
            return;
        case (static_cast<int>(EntityCategory::Enemy) << 8) | static_cast<int>(EntityCategory::Projectile):
            resolveProjectileVsEnemy(static_cast<Projectile&>(e2), static_cast<Enemy&>(e1));
            return;

        case (static_cast<int>(EntityCategory::Enemy) << 8) | static_cast<int>(EntityCategory::Enemy):
            resolveEnemyVsEnemy(static_cast<Enemy&>(e1), static_cast<Enemy&>(e2));
            return;

        case (static_cast<int>(EntityCategory::Projectile) << 8) | static_cast<int>(EntityCategory::Player):
            resolveProjectileVsPlayer(static_cast<Projectile&>(e1), static_cast<Player&>(e2));
            return;
        case (static_cast<int>(EntityCategory::Player) << 8) | static_cast<int>(EntityCategory::Projectile):
            resolveProjectileVsPlayer(static_cast<Projectile&>(e2), static_cast<Player&>(e1));
            return;

        default:
            // Item-vs-Item and Projectile-vs-Block are genuinely no-ops.
            return;
    }
}

void CollisionResolver::resolvePlayerVsEnemy(Player& player, Enemy& enemy, const CollisionInfo& info) {
    if (!enemy.isActive() || enemy.isDeadOrDying()) return;
    if (player.getInvincibilityTimer() > 0.0f) return; // Ignore all enemy contact (damage & stomp) during hurt i-frames

    // A cape swing clears whatever it reaches. Checked before the stomp test so
    // a spin connects from any direction, which is the point of having it.
    if (player.isSpinningCape() && !enemy.isDeadOrDying()) {
        enemy.onHitByFireball();   // the existing "knocked out sideways" path
        player.incrementCombo();
        player.addScore(enemy.getScoreValue() * player.getComboCounter());
        return;
    }

    // A stomp is any contact where the player is descending onto the enemy's upper band.
    // The feet-vs-top test is more forgiving than the raw collision normal at high speed.
    float playerFeetY = player.getBoundingBox().y + player.getBoundingBox().height;
    float enemyTopY = enemy.getBoundingBox().y + 10.0f;
    bool isStomp = (player.getVelocity().y > -50.0f && playerFeetY <= enemyTopY) ||
                   (info.normal.y == -1.0f && player.getVelocity().y >= 0.0f);

    // --- Koopa Troopas and their shells ------------------------------------
    //
    // This branch used to pick up ANY unflipped Koopa on any non-stomp contact.
    // Walking into a live, patrolling Koopa handed you the Koopa instead of
    // hurting you, and running into a shell someone had just kicked did the
    // same — so Koopas were the one enemy in the game that could not hurt you
    // at all, and a kicked shell was harmless to its own kicker.
    //
    // The rules below are the ones the series has always used:
    //   walking Koopa   stomp -> shell;      side/below -> damage
    //   idle shell      stomp -> kick;       side  -> kick, or carry if running
    //   sliding shell   stomp -> stop;       side  -> damage
    if (auto koopa = dynamic_cast<KoopaTroopa*>(&enemy)) {
        if (!koopa->isFlipped() && koopa->getState() != KoopaState::ShellHeld) {
            const KoopaState koopaState = koopa->getState();

            if (isStomp) {
                // onStomped() already knows all three cases: shell a walker,
                // kick an idle shell, stop a sliding one.
                player.velocity.y = -Constants::STOMP_BOUNCE_FORCE;
                koopa->onStomped();
                if (koopaState == KoopaState::Walking) {
                    player.incrementCombo();
                    player.addScore(enemy.getScoreValue() * player.getComboCounter());
                }
                return;
            }

            // A shell you just launched slides out from under you. Without
            // this, kicking or throwing one hurt you on the very next frame.
            if (koopaState == KoopaState::ShellKicked && koopa->isHarmlessToKicker()) {
                return;
            }

            if (koopaState == KoopaState::ShellIdle) {
                // Side contact with a resting shell. Holding run picks it up to
                // carry; otherwise it is kicked away, which is what happens if
                // you simply walk into one.
                const float dx = player.getBoundingBox().getCenter().x -
                                 koopa->getBoundingBox().getCenter().x;
                const float away = (dx >= 0.0f) ? -1.0f : 1.0f;

                // Either "B button". The originals use one button for run and
                // fire; this game splits them, and requiring specifically the
                // run key made carrying a shell feel like it did not work.
                InputManager& input = InputManager::getInstance();
                const int pad = player.getPlayerIndex();
                const bool grabHeld = player.isRunRequested() ||
                                      input.isActionHeld("run", pad) ||
                                      input.isActionHeld("fire", pad);
                if (grabHeld && !player.getHeldEntity()) {
                    koopa->pickUp(&player);
                    player.holdEntity(koopa);
                } else {
                    koopa->kick({away * Constants::KOOPA_SHELL_KICK_SPEED,
                                 koopa->getVelocity().y});
                    SoundManager::getInstance().playSound("kick");
                }
                return;
            }

            // A walking Koopa or a shell already sliding: falls through to the
            // ordinary side-contact damage path below, like every other enemy.
        }
    }

    // --- Bosses ------------------------------------------------------------
    //
    // A boss cannot reuse the generic stomp path, which is written as if contact
    // were transient. `isStomp` is a *positional* test, true on every frame the
    // player's feet are near the enemy's top, and the branch never separates the
    // two boxes — so standing on BoomBoom held it true indefinitely. That paid
    // score and combo every frame, landed a real hit every time the boss's
    // one-second i-frames lapsed, and never hurt the player, because takeDamage
    // is only in the else branch. Three seconds of standing still won the fight.
    if (auto* boss = dynamic_cast<Boss*>(&enemy)) {
        const bool descending = player.getVelocity().y > Boss::STOMP_MIN_DESCENT_SPEED;

        // Resting on the boss is not an attack. While its i-frames run, or while
        // the player is not actually falling onto it, contact is contact — it
        // hurts, exactly as touching its side does.
        if (!descending || boss->isInvulnerable()) {
            const float dx = player.getBoundingBox().getCenter().x -
                             boss->getBoundingBox().getCenter().x;
            const float direction = (dx >= 0.0f) ? 1.0f : -1.0f;
            player.velocity.x = direction * Constants::KNOCKBACK_FORCE_X;
            player.velocity.y = -Constants::KNOCKBACK_FORCE_Y;
            player.takeDamage(1);
            return;
        }

        // A genuine descending impact. Pay out only if the hit actually landed —
        // takeHit() reports that, and it was being ignored.
        const bool landed = boss->tryStomp();
        player.velocity.y = -Constants::STOMP_BOUNCE_FORCE;
        // Bounced clear of the boss's box either way, so the next frame is not
        // another contact frame. Without this the player never leaves and the
        // whole cycle repeats.
        const AABB bossBox = boss->getBoundingBox();
        player.position.y = bossBox.y - player.getBoundingBox().height - 1.0f;
        player.boundingBox.y = player.position.y;

        if (landed) {
            player.incrementCombo();
            player.addScore(boss->getScoreValue() * player.getComboCounter());
        }
        return;
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

        if (auto pow = dynamic_cast<POWBlock*>(&item)) {
            // Read the approach BEFORE the displacement below zeroes it: whether
            // this contact is a strike depends on how fast the player was
            // travelling into the block, and by the end of this branch that
            // information is gone.
            const float approachY = player.getVelocity().y;

            // Solid in every direction, exactly like a brick — the POW block is
            // terrain you can stand on. It used to fall through to the generic
            // pickup path below, so walking sideways into it "collected" it.
            player.position.x += info.normal.x * (info.overlap.x + 0.01f);
            player.position.y += info.normal.y * (info.overlap.y + 0.01f);
            if (info.normal.x != 0.0f) player.velocity.x = 0.0f;
            if (info.normal.y != 0.0f) player.velocity.y = 0.0f;
            if (info.normal.y == -1.0f) player.onGround = true;

            // A strike is a hit from below (the arcade original) or a genuine
            // descending stomp onto the top. Brushing past the side does
            // nothing, and neither does resting on it — the descent-speed floor
            // is what separates "landed on it" from "standing on it", the same
            // distinction Boss::STOMP_MIN_DESCENT_SPEED draws.
            constexpr float MIN_STRIKE_DESCENT = 60.0f;
            const bool struckFromBelow = info.normal.y == 1.0f;
            const bool stomped = info.normal.y == -1.0f && approachY >= MIN_STRIKE_DESCENT;
            if (struckFromBelow || stomped) {
                pow->activate(player);
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
    // Shadow Mario. Touching your own past costs you: no pushback, no stomp, no
    // bounce — the shadow is unmoved by the collision because it is replaying a
    // path, and shoving it off that path would desync it from the recording it
    // is driven by.
    if (p1.isContactHazard() != p2.isContactHazard()) {
        Player& hazard = p1.isContactHazard() ? p1 : p2;
        Player& victim = p1.isContactHazard() ? p2 : p1;
        if (victim.getInvincibilityTimer() <= 0.0f) {
            victim.takeDamage(1);
            SoundManager::getInstance().playSound("damage");
            // The hazard is told it was the cause. Whoever reports the death
            // then has a fact to report rather than a guess.
            hazard.onContactWithPlayer();
        }
        return;
    }

    // Co-op: jumping on your partner is a boost, not an attack. The bounce is
    // taller than the versus stomp — it is the mode's traversal mechanic, meant
    // to get a partner onto ledges they cannot reach alone — and the player
    // underneath is not pushed down or stunned.
    if (Game::getInstance().matchConfig().isCoop() && info.normal.y != 0.0f) {
        Player& upper = (info.normal.y == -1.0f) ? p1 : p2;
        Player& lower = (info.normal.y == -1.0f) ? p2 : p1;
        upper.position.y += info.normal.y * info.overlap.y;
        upper.boundingBox.y = upper.position.y;
        upper.velocity.y = -Constants::PLAYER_BOUNCE_FORCE * 1.6f;
        lower.velocity.y = 0.0f;
        SoundManager::getInstance().playSound("boing");
        return;
    }

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

    // An unrevealed hidden block is solid only from underneath. Any other
    // approach passes straight through, so the player never bumps into
    // invisible geometry while running or falling past it.
    if (auto hidden = dynamic_cast<HiddenBlock*>(&block)) {
        if (!hidden->isRevealed() && info.normal.y != 1.0f) return;
    }

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

void CollisionResolver::resolveEnemyVsEnemy(Enemy& a, Enemy& b) {
    // A shell sliding after a kick clears everything it touches. Without this
    // branch the resolver had no enemy-vs-enemy case at all, so shells passed
    // straight through and shell chains were impossible (audit B-8).
    auto slidingShell = [](Enemy& e) -> KoopaTroopa* {
        auto koopa = dynamic_cast<KoopaTroopa*>(&e);
        if (koopa && koopa->getState() == KoopaState::ShellKicked) return koopa;
        return nullptr;
    };

    KoopaTroopa* shellA = slidingShell(a);
    KoopaTroopa* shellB = slidingShell(b);

    // Two shells meeting cancel each other rather than fighting over who wins.
    if (shellA && shellB) {
        shellA->onHitByFireball();
        shellB->onHitByFireball();
        return;
    }

    // onHitByFireball is the existing "knocked out from the side" path: it flips
    // the enemy, awards score and publishes EnemyDefeated.
    if (shellA && !b.isDeadOrDying()) { b.onHitByFireball(); return; }
    if (shellB && !a.isDeadOrDying()) { a.onHitByFireball(); return; }

    // Two ordinary walkers just ignore each other, as in the original games.
}

void CollisionResolver::resolveProjectileVsEnemy(Projectile& projectile, Enemy& enemy) {
    if (!projectile.isActive() || !enemy.isActive()) return;
    if (!projectile.damagesEnemies()) return;   // a thrown hammer passes through
    projectile.onHitEnemy(enemy);
}

void CollisionResolver::resolveProjectileVsPlayer(Projectile& projectile, Player& player) {
    if (!projectile.isActive() || !player.isActive()) return;
    if (!projectile.damagesPlayer()) return;    // the player's own fireball
    projectile.onHitPlayer(player);
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

