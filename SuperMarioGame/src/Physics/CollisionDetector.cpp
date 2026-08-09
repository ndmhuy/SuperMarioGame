#include "Physics/CollisionDetector.hpp"
#include "Entities/Entity.hpp"
#include "Entities/Player.hpp"
#include "Entities/IPlayerState.hpp"
#include "Utils/TileMap.hpp"
#include "Utils/Constants.hpp"
#include <algorithm>
#include <cmath>

CollisionInfo CollisionDetector::checkEntityVsEntity(Entity& e1, Entity& e2) {
    CollisionInfo info;
    AABB box1 = e1.getBoundingBox();
    AABB box2 = e2.getBoundingBox();

    if (!box1.intersects(box2)) {
        return info;
    }

    AABB overlapBox = box1.getOverlap(box2);
    info.collided = true;
    info.other = &e2;

    sf::Vector2f center1 = box1.getCenter();
    sf::Vector2f center2 = box2.getCenter();

    if (overlapBox.width < overlapBox.height) {
        info.overlap.x = overlapBox.width;
        if (center1.x < center2.x) {
            info.normal.x = -1.0f;
        } else {
            info.normal.x = 1.0f;
        }
    } else {
        info.overlap.y = overlapBox.height;
        if (center1.y < center2.y) {
            info.normal.y = -1.0f;
        } else {
            info.normal.y = 1.0f;
        }
    }

    return info;
}

std::vector<CollisionInfo> CollisionDetector::checkEntityVsTileMap(Entity& entity, TileMap& tileMap) {
    std::vector<CollisionInfo> collisions;
    AABB entityBox = entity.getBoundingBox();

    int startX = static_cast<int>(std::floor(entityBox.x / Constants::TILE_SIZE));
    int endX = static_cast<int>(std::floor((entityBox.x + entityBox.width) / Constants::TILE_SIZE));
    int startY = static_cast<int>(std::floor(entityBox.y / Constants::TILE_SIZE));
    int endY = static_cast<int>(std::floor((entityBox.y + entityBox.height) / Constants::TILE_SIZE));

    for (int y = startY; y <= endY; ++y) {
        for (int x = startX; x <= endX; ++x) {
            TileType tileType = tileMap.getTileType(x, y);
            const TileInfo& tileInfo = TileMap::getInfo(tileType);
            bool isSolid = tileInfo.isSolid;

            // MiniState water walking check
            if (tileType == TileType::Water) {
                if (auto player = dynamic_cast<Player*>(&entity)) {
                    IPlayerState* baseState = player->getCurrentState();
                    while (auto* decorator = dynamic_cast<PlayerStateDecorator*>(baseState)) {
                        baseState = decorator->getWrappedState();
                    }
                    if (dynamic_cast<MiniState*>(baseState)) {
                        float feetY = entity.getPosition().y + entity.getBoundingBox().height;
                        float waterTopY = y * Constants::TILE_SIZE;
                        if (feetY <= waterTopY + 4.0f && entity.getVelocity().y >= 0.0f) {
                            isSolid = true;
                        }
                    }
                }
            }

            if (isSolid) {
                AABB tileBox {
                    x * Constants::TILE_SIZE,
                    y * Constants::TILE_SIZE,
                    Constants::TILE_SIZE,
                    Constants::TILE_SIZE
                };

                if (entityBox.intersects(tileBox)) {
                    CollisionInfo collisionInfo;
                    collisionInfo.collided = true;
                    collisionInfo.tileX = x;
                    collisionInfo.tileY = y;
                    collisionInfo.other = nullptr;

                    AABB overlapBox = entityBox.getOverlap(tileBox);
                    sf::Vector2f center1 = entityBox.getCenter();
                    sf::Vector2f center2 = tileBox.getCenter();

                    if (overlapBox.width < overlapBox.height) {
                        collisionInfo.overlap.x = overlapBox.width;
                        if (center1.x < center2.x) {
                            collisionInfo.normal.x = -1.0f;
                        } else {
                            collisionInfo.normal.x = 1.0f;
                        }

                        // Internal edge mitigation:
                        // If we detected a horizontal collision, but the adjacent tile in the direction
                        // of the collision is also solid, then it is an internal vertical seam.
                        // Convert it to a vertical collision so the entity can slide smoothly!
                        bool isInternal = false;
                        if (collisionInfo.normal.x < 0.0f) { // Entity is on the left
                            if (x > 0) {
                                TileType adjacentTile = tileMap.getTileType(x - 1, y);
                                if (TileMap::getInfo(adjacentTile).isSolid) {
                                    isInternal = true;
                                }
                            }
                        } else { // Entity is on the right
                            if (x < tileMap.getWidth() - 1) {
                                TileType adjacentTile = tileMap.getTileType(x + 1, y);
                                if (TileMap::getInfo(adjacentTile).isSolid) {
                                    isInternal = true;
                                }
                            }
                        }

                        if (isInternal) {
                            collisionInfo.normal.x = 0.0f;
                            collisionInfo.normal.y = (center1.y < center2.y) ? -1.0f : 1.0f;
                            collisionInfo.overlap.x = 0.0f;
                            collisionInfo.overlap.y = overlapBox.height;
                        }
                    } else {
                        collisionInfo.overlap.y = overlapBox.height;
                        if (center1.y < center2.y) {
                            collisionInfo.normal.y = -1.0f;
                        } else {
                            collisionInfo.normal.y = 1.0f;
                        }
                    }
                    collisions.push_back(collisionInfo);
                }
            }
        }
    }

    return collisions;
}
