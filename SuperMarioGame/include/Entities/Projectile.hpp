#pragma once

#include "Entities/Entity.hpp"

class SpriteSheet;

class Enemy;
class Player;

// Common base for everything that flies and hits something.
//
// It exists because CollisionResolver used to static_cast *every* Projectile to
// Fireball: a Hammer Bro's hammer touching another enemy was cast to a type it
// is not, and then killed that enemy — which no thrown hammer should do. Adding
// a third projectile (Bowser's fire breath) would have ridden the same path.
//
// Each projectile declares who it is allowed to hurt and what contact does, so
// the resolver dispatches without naming any concrete type. Adding a fourth
// projectile now needs no change to the resolver at all (OCP).
class Projectile : public Entity {
public:
    explicit Projectile(sf::Vector2f pos = {0.0f, 0.0f}, sf::Vector2f targetSize = {12.0f, 12.0f});
    ~Projectile() override = default;

    // final, because the resolver's static_cast from Entity& to Projectile& is
    // only sound while this category means "is a Projectile" and nothing else.
    EntityCategory getCategory() const final { return EntityCategory::Projectile; }

    // Who this projectile is allowed to damage. Both default to false: a new
    // projectile is inert until it says otherwise.
    virtual bool damagesEnemies() const { return false; }
    virtual bool damagesPlayer() const { return false; }

    // Called only when the matching damages*() returned true.
    virtual void onHitEnemy(Enemy& enemy) {}
    virtual void onHitPlayer(Player& player) {}

    // Declared here so the animation dispatcher can wire any projectile without
    // naming it. Hammer had this method already and nothing ever called it: the
    // dispatcher tested Player/Enemy/Item/Block and hammers fell through to the
    // "unknown entity type" branch, so a thrown hammer drew its placeholder.
    virtual void setupAnimations(const SpriteSheet* spriteSheet) {}
};
