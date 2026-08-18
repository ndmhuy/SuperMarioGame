#include "Physics/PhysicsEngine.hpp"
#include "Entities/Entity.hpp"
#include "Entities/Character.hpp"
#include "Entities/Player.hpp"
#include "Entities/Enemy.hpp"
#include "Entities/Luigi.hpp"
#include "Entities/Toad.hpp"
#include "Entities/Peach.hpp"
#include "Entities/Fireball.hpp"
#include "Entities/Item.hpp"
#include "Utils/TileMap.hpp"
#include "Utils/Constants.hpp"
#include "Core/SoundManager.hpp"
#include "Core/EventBus.hpp"
#include "Graphics/ParticleEmitter.hpp"
#include "Utils/MathUtils.hpp"
#include <cmath>
#include <algorithm>
#include <unordered_set>
#include <unordered_map>
#include <utility>

void PhysicsEngine::applyGravity(Entity& entity, float dt) {
    if (auto character = dynamic_cast<Character*>(&entity)) {
        if (character->onGround) {
            return;
        }
    }
    if (auto item = dynamic_cast<Item*>(&entity)) {
        if (item->isOnGround()) {
            return;
        }
    }
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
        if (auto character = dynamic_cast<Character*>(entity.get())) {
            if (character->onGround) {
                float cx = character->position.x + character->boundingBox.width / 2.0f;
                float feetY = character->position.y + character->boundingBox.height + Constants::GROUND_CHECK_OFFSET;
                if (tileMap.getTileSurfaceType(cx, feetY) == TileType::Conveyor) {
                    character->position.x += Constants::CONVEYOR_SPEED * dt;
                    character->boundingBox.x = character->position.x;
                }
            }
        }
    }

    // 1.1. Check non-solid interactive tile pickups (Coin tile collection)
    for (const auto& entity : entities) {
        if (!entity || !entity->isActive()) continue;
        if (auto player = dynamic_cast<Player*>(entity.get())) {
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

    // 1.5. Apply character-specific horizontal acceleration, deceleration, and friction
    for (const auto& entity : entities) {
        if (!entity || !entity->isActive()) continue;
        if (auto character = dynamic_cast<Character*>(entity.get())) {
            float maxSpeed = character->getSpeed();
            
            // Check if player is running to scale their max horizontal speed
            if (auto player = dynamic_cast<Player*>(character)) {
                if (player->isRunRequested()) {
                    maxSpeed = Constants::RUN_SPEED;
                    if (dynamic_cast<Luigi*>(player)) maxSpeed *= Constants::LUIGI_SPEED_MULT;
                    else if (dynamic_cast<Toad*>(player)) maxSpeed = Constants::RUN_SPEED * 1.3f;
                    else if (dynamic_cast<Peach*>(player)) maxSpeed *= 0.9f;
                }
            }

            float accelRate = 1000.0f; // px/s^2 (0 -> 150 px/s in 0.15s)
            float decelRate = character->onGround ? 1000.0f : 300.0f;

            // Ice platform check: reduced friction
            if (character->onGround) {
                float cx = character->position.x + character->boundingBox.width / 2.0f;
                float feetY = character->position.y + character->boundingBox.height + Constants::GROUND_CHECK_OFFSET;
                if (tileMap.getTileSurfaceType(cx, feetY) == TileType::Ice) {
                    decelRate = 250.0f; // Slide further on ice!
                }
            }

            // Check if player is currently in a crouch slide (handled internally by Player::update)
            bool isPlayerCrouchingOrSliding = false;
            if (auto player = dynamic_cast<Player*>(character)) {
                isPlayerCrouchingOrSliding = player->isCrouched() || player->isSliding();
            }

            // Process movement requests
            if (character->isMoveLeftRequested()) {
                character->velocity.x -= accelRate * dt;
                character->velocity.x = MathUtils::clamp(character->velocity.x, -maxSpeed, maxSpeed);
            }
            else if (character->isMoveRightRequested()) {
                character->velocity.x += accelRate * dt;
                character->velocity.x = MathUtils::clamp(character->velocity.x, -maxSpeed, maxSpeed);
            }
            else if (!isPlayerCrouchingOrSliding) {
                // Apply passive friction decay towards 0
                if (character->velocity.x > 0.0f) {
                    character->velocity.x -= decelRate * dt;
                    if (character->velocity.x < 0.0f) character->velocity.x = 0.0f;
                } else if (character->velocity.x < 0.0f) {
                    character->velocity.x += decelRate * dt;
                    if (character->velocity.x > 0.0f) character->velocity.x = 0.0f;
                }
            }

            // Clear intent flags for next frame
            character->clearMovementRequests();

            // Reset ground/wall flags for the new collision detection pass
            character->onGround = false;
            character->onWall = false;
        } else if (auto item = dynamic_cast<Item*>(entity.get())) {
            item->setOnGround(false);
        }
    }


    // 2. Apply gravity and environmental forces
    for (const auto& entity : entities) {
        if (!entity || !entity->isActive() || entity->getGravityMultiplier() <= 0.0f) continue;


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

        entity->position.x += entity->velocity.x * dt;
        entity->boundingBox.x = entity->position.x;

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

        if (maxCollision.collided) {
            if (auto fireball = dynamic_cast<Fireball*>(entity.get())) {
                fireball->destroy();
            } else {
                m_resolver.resolveEntityVsTile(*entity, maxCollision);
            }
        }
    }

    // 4. Integrate Y and resolve Y collisions with the tile map (only resolve max vertical overlap)
    for (const auto& entity : entities) {
        if (!entity || !entity->isActive()) continue;

        float preVelY = entity->velocity.y;
        entity->position.y += entity->velocity.y * dt;
        entity->boundingBox.y = entity->position.y;

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
            if (auto fireball = dynamic_cast<Fireball*>(entity.get())) {
                if (maxCollision.normal.y == -1.0f) {
                    fireball->bounce();
                } else {
                    fireball->destroy();
                }
            } else {
                m_resolver.resolveEntityVsTile(*entity, maxCollision);

                // Head-butt logic for Player hitting ceiling tiles from below
                if (auto player = dynamic_cast<Player*>(entity.get())) {
                    for (const auto& col : collisions) {
                        if (col.tileX != -1 && col.tileY != -1 && (col.normal.y == 1.0f || preVelY < 0.0f)) {
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
                if (dynamic_cast<Enemy*>(entity.get())) {
                    vel.x = -vel.x; // Patrol enemy turns around at left map border
                } else {
                    vel.x = 0.0f;   // Player/Item stops at left map border
                }
            }
            entity->setPosition(pos);
            entity->setVelocity(vel);
        }
        // Right Map Boundary (x = maxMapX)
        else if (pos.x + width > maxMapX && maxMapX > 0.0f) {
            pos.x = maxMapX - width;
            if (vel.x > 0.0f) {
                if (dynamic_cast<Enemy*>(entity.get())) {
                    vel.x = -vel.x; // Patrol enemy turns around at right map border
                } else {
                    vel.x = 0.0f;   // Player/Item stops at right map border
                }
            }
            entity->setPosition(pos);
            entity->setVelocity(vel);
        }
    }

    // 5. Broadphase entity-to-entity collisions via Spatial Hash
    // Rebuild the spatial hash with the updated and resolved bounding boxes
    m_spatialHash.clear();
    for (const auto& entity : entities) {
        if (entity && entity->isActive()) {
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
        if (!entity || !entity->isActive()) continue;

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
}
