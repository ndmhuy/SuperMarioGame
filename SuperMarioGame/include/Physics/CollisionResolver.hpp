#pragma once

#include "Physics/CollisionDetector.hpp"

class Entity;
class Character;
class Enemy;
class Player;
class Item;

class CollisionResolver {
public:
    CollisionResolver() = default;
    ~CollisionResolver() = default;

    // Direct resolution methods
    void resolveEntityVsTile(Entity& entity, const CollisionInfo& info);
    void resolveEntityVsEntity(Entity& e1, Entity& e2, const CollisionInfo& info);

    // Specific entity class resolutions
    void resolvePlayerVsEnemy(Player& player, Enemy& enemy, const CollisionInfo& info);
    void resolvePlayerVsItem(Player& player, Item& item, const CollisionInfo& info);
    void resolvePlayerVsPlayer(Player& p1, Player& p2, const CollisionInfo& info);
};
