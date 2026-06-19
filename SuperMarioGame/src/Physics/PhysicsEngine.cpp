#include "Physics/PhysicsEngine.hpp"
#include "Entities/Entity.hpp"
#include "Entities/Character.hpp"
#include "Utils/TileMap.hpp"
#include "Utils/Constants.hpp"
#include <cmath>
#include <algorithm>

void PhysicsEngine::applyGravity(Entity& entity, float dt) {
    if (auto character = dynamic_cast<Character*>(&entity)) {
        if (character->onGround) {
            return;
        }
    }
    // Acceleration: 0.5 px/frame^2 at 60 FPS is 1800 px/s^2
    entity.velocity.y += Constants::GRAVITY * 3600.0f * dt;
    if (entity.velocity.y > 600.0f) {
        entity.velocity.y = 600.0f;
    }
}

void PhysicsEngine::integrateVelocity(Entity& entity, float dt) {
    entity.position += entity.velocity * dt;
    entity.boundingBox.x = entity.position.x;
    entity.boundingBox.y = entity.position.y;
}

void PhysicsEngine::update(std::vector<Entity*>& entities, TileMap& tileMap, float dt) {
    // 1. Rebuild spatial hash
    m_spatialHash.clear();
    for (auto entity : entities) {
        if (entity && entity->isActive()) {
            m_spatialHash.insert(entity, entity->getBoundingBox());
        }
    }

    // Reset character states
    for (auto entity : entities) {
        if (!entity || !entity->isActive()) continue;
        if (auto character = dynamic_cast<Character*>(entity)) {
            character->onGround = false;
            character->onWall = false;
        }
    }

    // 2. Apply gravity and environmental forces
    for (auto entity : entities) {
        if (!entity || !entity->isActive()) continue;

        // Check if entity is in water (tile type 7)
        float cx = entity->position.x + entity->boundingBox.width / 2.0f;
        float cy = entity->position.y + entity->boundingBox.height / 2.0f;
        bool inWater = (tileMap.getTileSurfaceType(cx, cy) == TileType::Water);

        if (inWater) {
            entity->velocity.y += Constants::GRAVITY * 3600.0f * 0.3f * dt;
            if (entity->velocity.y > 60.0f) {
                entity->velocity.y = 60.0f; // Water terminal velocity
            }
        } else {
            applyGravity(*entity, dt);
        }

        // Apply conveyor push if standing on conveyor (tile type 6)
        if (auto character = dynamic_cast<Character*>(entity)) {
            if (character->onGround) {
                float feetY = character->position.y + character->boundingBox.height + 2.0f;
                if (tileMap.getTileSurfaceType(cx, feetY) == TileType::Conveyor) {
                    character->position.x += 100.0f * dt;
                    character->boundingBox.x = character->position.x;
                }
            }
        }
    }

    // 3. Integrate X and resolve X collisions with the tile map
    for (auto entity : entities) {
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
    for (auto entity : entities) {
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
    for (auto entity : entities) {
        if (!entity || !entity->isActive()) continue;

        auto candidates = m_spatialHash.query(entity->getBoundingBox());
        for (auto candidate : candidates) {
            if (candidate == entity || !candidate->isActive()) continue;

            auto collision = m_detector.checkEntityVsEntity(*entity, *candidate);
            if (collision.collided) {
                m_resolver.resolveEntityVsEntity(*entity, *candidate, collision);
            }
        }
    }
}
