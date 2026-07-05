#include "Physics/PhysicsEngine.hpp"
#include "Entities/Entity.hpp"
#include "Entities/Character.hpp"
#include "Entities/Player.hpp"
#include "Entities/Luigi.hpp"
#include "Entities/Toad.hpp"
#include "Entities/Peach.hpp"
#include "Utils/TileMap.hpp"
#include "Utils/Constants.hpp"
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
    // 1. Reset character states and apply previous frame's conveyor push
    for (const auto& entity : entities) {
        if (!entity || !entity->isActive()) continue;
        if (auto character = dynamic_cast<Character*>(entity.get())) {
            // Apply conveyor push if standing on conveyor (tile type 6) using the ground status from the previous frame
            if (character->onGround) {
                float cx = character->position.x + character->boundingBox.width / 2.0f;
                float feetY = character->position.y + character->boundingBox.height + Constants::GROUND_CHECK_OFFSET;
                if (tileMap.getTileSurfaceType(cx, feetY) == TileType::Conveyor) {
                    character->position.x += Constants::CONVEYOR_SPEED * dt;
                    character->boundingBox.x = character->position.x;
                }
            }
            character->onGround = false;
            character->onWall = false;
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
        }
    }

    // 2. Apply gravity and environmental forces
    for (const auto& entity : entities) {
        if (!entity || !entity->isActive()) continue;

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

    // 3. Integrate X and resolve X collisions with the tile map
    for (const auto& entity : entities) {
        if (!entity || !entity->isActive()) continue;

        entity->position.x += entity->velocity.x * dt;
        entity->boundingBox.x = entity->position.x;

        auto collisions = m_detector.checkEntityVsTileMap(*entity, tileMap);
        for (auto& collision : collisions) {
            if (collision.normal.x != 0.0f) {
                m_resolver.resolveEntityVsTile(*entity, collision);
            }
        }
    }

    // 4. Integrate Y and resolve Y collisions with the tile map
    for (const auto& entity : entities) {
        if (!entity || !entity->isActive()) continue;

        entity->position.y += entity->velocity.y * dt;
        entity->boundingBox.y = entity->position.y;

        auto collisions = m_detector.checkEntityVsTileMap(*entity, tileMap);
        for (auto& collision : collisions) {
            if (collision.normal.y != 0.0f) {
                m_resolver.resolveEntityVsTile(*entity, collision);
            }
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
