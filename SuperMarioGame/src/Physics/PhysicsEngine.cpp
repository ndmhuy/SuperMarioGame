#include "Physics/PhysicsEngine.hpp"
#include "Entities/Entity.hpp"
#include "Entities/Player.hpp"
#include "Utils/TileMap.hpp"
#include "Utils/Constants.hpp"
#include "Core/SoundManager.hpp"
#include "Core/EventBus.hpp"
#include "Graphics/ParticleEmitter.hpp"
#include <cmath>
#include <algorithm>
#include <unordered_set>
#include <unordered_map>
#include <utility>

void PhysicsEngine::applyGravity(Entity& entity, float dt) {
    // Anything resting on something solid is not falling. Characters and Items
    // both answer this through Entity::isOnGround(); everything else says no.
    if (entity.isOnGround()) return;

    // Acceleration: 0.5 px/frame^2 at 60 FPS is 1800 px/s^2
    entity.velocity.y += Constants::GRAVITY * Constants::GRAVITY_SCALE * entity.getGravityMultiplier() * dt;
    if (entity.velocity.y > Constants::TERMINAL_VELOCITY) {
        entity.velocity.y = Constants::TERMINAL_VELOCITY;
    }
}


void PhysicsEngine::integrateVelocity(Entity& entity, float dt) {
    entity.position += entity.velocity * dt;
    entity.boundingBox.x = entity.position.x;
    entity.boundingBox.y = entity.position.y;
}

void PhysicsEngine::update(const std::vector<std::unique_ptr<Entity>>& entities, TileMap& tileMap, float dt) {
    // 1. Apply previous frame's conveyor push using the ground status from the previous frame
    for (const auto& entity : entities) {
        if (!entity || !entity->isActive()) continue;
        if (!entity->ridesConveyors() || !entity->isOnGround()) continue;

        float cx = entity->position.x + entity->boundingBox.width / 2.0f;
        float feetY = entity->position.y + entity->boundingBox.height + Constants::GROUND_CHECK_OFFSET;
        if (tileMap.getTileSurfaceType(cx, feetY) == TileType::Conveyor) {
            entity->position.x += Constants::CONVEYOR_SPEED * dt;
            entity->boundingBox.x = entity->position.x;
        }
    }

    // 1.1. Check non-solid interactive tile pickups (Coin tile collection)
    for (const auto& entity : entities) {
        if (!entity || !entity->isActive()) continue;
        // Only a player collects coins. The category is the exact answer — it is
        // overridden once, by Player, for its whole subtree — and it costs a
        // virtual call rather than a walk of the RTTI hierarchy (audit A-10 / D8).
        if (entity->getCategory() == EntityCategory::Player) {
            Player* player = static_cast<Player*>(entity.get());
            AABB pBox = player->getBoundingBox();
            int startX = static_cast<int>(std::floor(pBox.x / Constants::TILE_SIZE));
            int endX = static_cast<int>(std::floor((pBox.x + pBox.width) / Constants::TILE_SIZE));
            int startY = static_cast<int>(std::floor(pBox.y / Constants::TILE_SIZE));
            int endY = static_cast<int>(std::floor((pBox.y + pBox.height) / Constants::TILE_SIZE));

            for (int y = startY; y <= endY; ++y) {
                for (int x = startX; x <= endX; ++x) {
                    if (tileMap.getTileType(x, y) == TileType::Coin) {
                        tileMap.setTile(x, y, TileType::Empty);
                        player->addCoins(1);
                        player->addScore(200);
                        SoundManager::getInstance().playSound("coin");
                    }
                }
            }
        }
    }



    // 1.5. Let each entity apply its own horizontal acceleration, braking and
    // friction, then clear its per-frame contact flags.
    //
    // This loop used to be a dynamic_cast<Character*> wrapping fifty lines of
    // locomotion, with three more casts inside it to find out which player
    // character was running and whether it was mid-crouch-slide (audit A-10 /
    // D8). The engine now owns only the part that is genuinely the
    // environment's — how hard the surface underfoot brakes — and hands that
    // number to the entity.
    for (const auto& entity : entities) {
        if (!entity || !entity->isActive()) continue;

        // Airborne control is weaker than ground friction, and ice is slippery.
        float groundDecel = entity->isOnGround() ? 1000.0f : 300.0f;
        if (entity->isOnGround()) {
            float cx = entity->position.x + entity->boundingBox.width / 2.0f;
            float feetY = entity->position.y + entity->boundingBox.height + Constants::GROUND_CHECK_OFFSET;
            if (tileMap.getTileSurfaceType(cx, feetY) == TileType::Ice) {
                groundDecel = 250.0f; // Slide further on ice!
            }
        }

        entity->applyHorizontalControl(dt, groundDecel);

        // Intent flags are NOT cleared here. They describe what the player
        // asked for this frame, and collision resolution at the end of this
        // same update() reads them: "hold run to carry a shell instead of
        // kicking it" could never fire, because run had already been wiped
        // by the time the resolver looked. Cleared at the end of update()
        // instead, once everything that consumes them has run.
        //
        // The *contact* flags do get reset here, ready for this frame's
        // collision passes — after applyHorizontalControl(), which reads them.
        entity->beginPhysicsFrame();
    }

    // 2. Apply gravity and environmental forces
    for (const auto& entity : entities) {
        if (!entity || !entity->isActive()) continue;
        // Zero-gravity entities (blocks, flying/scripted enemies) opt out entirely.
        if (entity->getGravityMultiplier() <= 0.0f) continue;
        // Dead or held enemies are driven by their own death/carry animation, not physics.
        if (!entity->isPhysicsDriven()) continue;

        // Check if entity is in water (tile type 7)
        float cx = entity->position.x + entity->boundingBox.width / 2.0f;
        float cy = entity->position.y + entity->boundingBox.height / 2.0f;
        bool inWater = (tileMap.getTileSurfaceType(cx, cy) == TileType::Water);

        if (inWater) {
            entity->velocity.y += Constants::GRAVITY * Constants::GRAVITY_SCALE * Constants::WATER_GRAVITY_MULT * dt;
            if (entity->velocity.y > Constants::WATER_TERMINAL_VELOCITY) {
                entity->velocity.y = Constants::WATER_TERMINAL_VELOCITY; // Water terminal velocity
            }
        } else {
            applyGravity(*entity, dt);
        }
    }

    // 3. Integrate X and resolve X collisions with the tile map (only resolve max horizontal overlap)
    for (const auto& entity : entities) {
        if (!entity || !entity->isActive()) continue;
        if (!entity->isPhysicsDriven()) continue;

        entity->position.x += entity->velocity.x * dt;
        entity->boundingBox.x = entity->position.x;

        if (!entity->collidesWithTiles()) continue;

        auto collisions = m_detector.checkEntityVsTileMap(*entity, tileMap);
        CollisionInfo maxCollision;
        maxCollision.collided = false;

        for (const auto& collision : collisions) {
            if (collision.normal.x != 0.0f) {
                if (!maxCollision.collided || collision.overlap.x > maxCollision.overlap.x) {
                    maxCollision = collision;
                }
            }
        }

        if (maxCollision.collided && !entity->onTileImpact(maxCollision)) {
            m_resolver.resolveEntityVsTile(*entity, maxCollision);
        }
    }

    // 4. Integrate Y and resolve Y collisions with the tile map (only resolve max vertical overlap)
    for (const auto& entity : entities) {
        if (!entity || !entity->isActive()) continue;
        if (!entity->isPhysicsDriven()) continue;

        float preVelY = entity->velocity.y;
        entity->position.y += entity->velocity.y * dt;
        entity->boundingBox.y = entity->position.y;

        if (!entity->collidesWithTiles()) continue;

        auto collisions = m_detector.checkEntityVsTileMap(*entity, tileMap);
        CollisionInfo maxCollision;
        maxCollision.collided = false;

        for (const auto& collision : collisions) {
            if (collision.normal.y != 0.0f) {
                if (!maxCollision.collided || collision.overlap.y > maxCollision.overlap.y) {
                    maxCollision = collision;
                }
            }
        }

        if (maxCollision.collided) {
            // The entity gets first refusal on the impact: a fireball bursts or
            // bounces instead of being pushed out of the tile.
            if (!entity->onTileImpact(maxCollision)) {
                m_resolver.resolveEntityVsTile(*entity, maxCollision);

                // Head-butt logic for Player hitting ceiling tiles from below.
                // Shadow Mario is excluded: it replays the player's path, so it
                // would punch out every question block and brick the player had
                // already jumped under, three seconds behind them.
                if (entity->getCategory() == EntityCategory::Player &&
                    !entity->isContactHazard()) {
                    Player* player = static_cast<Player*>(entity.get());
                    for (const auto& col : collisions) {
                        // Only a ceiling contact counts as a head-butt. The old
                        // condition also accepted `preVelY < 0.0f`, which is true for
                        // *horizontal* collisions while rising — so jumping up a
                        // 1-tile shaft destroyed the bricks either side of the player
                        // along with the one overhead (audit A-6).
                        const bool hitFromBelow = (col.normal.y == 1.0f) && (preVelY < 0.0f);
                        if (col.tileX != -1 && col.tileY != -1 && hitFromBelow) {
                            TileType hitTile = tileMap.getTileType(col.tileX, col.tileY);
                            
                            if (hitTile == TileType::Brick) {
                                // If player is Super+ (height > 32px), break block
                                if (player->getBoundingBox().height > 32.0f) {
                                    tileMap.setTile(col.tileX, col.tileY, TileType::Empty);
                                    player->addScore(100);
                                    SoundManager::getInstance().playSound("break_block");
                                    EventBus::getInstance().publish({EventType::BlockBroken, 100});
                                } else {
                                    // Small player just bumps it
                                    SoundManager::getInstance().playSound("bump");
                                }
                            } else if (hitTile == TileType::Question) {
                                // Yield coin and convert to used block
                                tileMap.setTile(col.tileX, col.tileY, TileType::Used);
                                player->addCoins(1);
                                player->addScore(200);
                                SoundManager::getInstance().playSound("coin");
                            } else if (hitTile == TileType::Coin) {
                                // Collect coin tile
                                tileMap.setTile(col.tileX, col.tileY, TileType::Empty);
                                player->addCoins(1);
                                player->addScore(200);
                                SoundManager::getInstance().playSound("coin");
                            }
                        }
                    }
                }
            }
        }
    }

    // 4.5. Enforce Level Map Boundaries (x = 0 and x = mapWidth * TILE_SIZE) for all entities
    float maxMapX = tileMap.getWidth() * Constants::TILE_SIZE;
    for (const auto& entity : entities) {
        if (!entity || !entity->isActive()) continue;

        sf::Vector2f pos = entity->getPosition();
        sf::Vector2f vel = entity->getVelocity();
        float width = entity->getBoundingBox().width;

        // Left Map Boundary (x = 0)
        if (pos.x < 0.0f) {
            pos.x = 0.0f;
            if (vel.x < 0.0f) {
                // Patrol enemies turn around; players and items stop dead.
                vel.x = entity->reversesAtLevelEdge() ? -vel.x : 0.0f;
            }
            entity->setPosition(pos);
            entity->setVelocity(vel);
        }
        // Right Map Boundary (x = maxMapX)
        else if (pos.x + width > maxMapX && maxMapX > 0.0f) {
            pos.x = maxMapX - width;
            if (vel.x > 0.0f) {
                vel.x = entity->reversesAtLevelEdge() ? -vel.x : 0.0f;
            }
            entity->setPosition(pos);
            entity->setVelocity(vel);
        }
    }

    // 5. Broadphase entity-to-entity collisions via Spatial Hash
    // Rebuild the spatial hash with the updated and resolved bounding boxes
    m_spatialHash.clear();
    for (const auto& entity : entities) {
        // Non-collidable entities are left out of the hash entirely rather than
        // inserted at a fake position (audit B-14).
        if (entity && entity->isActive() && entity->isCollidable()) {
            m_spatialHash.insert(entity.get(), entity->getBoundingBox());
        }
    }

    struct PairHash {
        std::size_t operator()(const std::pair<Entity*, Entity*>& p) const {
            return std::hash<void*>()(p.first) ^ (std::hash<void*>()(p.second) << 1);
        }
    };
    std::unordered_set<std::pair<Entity*, Entity*>, PairHash> resolvedPairs;

    for (const auto& entity : entities) {
        if (!entity || !entity->isActive() || !entity->isCollidable()) continue;

        auto candidates = m_spatialHash.query(entity->getBoundingBox());
        for (auto candidate : candidates) {
            if (candidate == entity.get() || !candidate->isActive()) continue;

            // Ensure unique pair ordering to de-duplicate A-B and B-A resolutions
            auto first = std::min(entity.get(), candidate);
            auto second = std::max(entity.get(), candidate);
            std::pair<Entity*, Entity*> pair(first, second);

            if (resolvedPairs.count(pair) > 0) continue;
            resolvedPairs.insert(pair);

            auto collision = m_detector.checkEntityVsEntity(*entity, *candidate);
            if (collision.collided) {
                m_resolver.resolveEntityVsEntity(*entity, *candidate, collision);
            }
        }
    }

    // 6. Intent flags last, now that acceleration and collision resolution have
    // both had their look at them.
    for (const auto& entity : entities) {
        if (!entity || !entity->isActive()) continue;
        entity->clearMovementRequests();
    }
}
