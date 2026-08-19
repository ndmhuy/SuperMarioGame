#pragma once

#include "Entities/Entity.hpp"
#include "Graphics/Animator.hpp"
#include "Graphics/SpriteSheet.hpp"
#include <memory>
#include <string>

class Player;

// The projectile Hammer Bro throws.
//
// HammerThrowStrategy has always had the throw timer and a callback hook, but
// nothing ever set the callback and no projectile class existed, so Hammer Bro
// stood on its platform and never attacked (audit B-6).
//
// Arcs under gravity, spins while airborne, and despawns once it falls out of
// the world. Damages the player on contact; harmless to other enemies.
class Hammer : public Entity {
public:
    Hammer(sf::Vector2f position, sf::Vector2f velocity);
    ~Hammer() override = default;

    std::string getTypeName() const override { return "hammer"; }
    EntityCategory getCategory() const override { return EntityCategory::Projectile; }

    void update(float dt) override;
    void render(sf::RenderTarget& target) override;
    void setupAnimations(const SpriteSheet* spriteSheet);

    // Hammers pass through terrain — they are thrown over gaps and platforms.
    bool collidesWithTiles() const override { return false; }

    void onHitPlayer(Player& player);

private:
    float m_lifetime = 6.0f;
    float m_spin = 0.0f;

    std::unique_ptr<Animator> m_animator;
    Animation m_animation;
    bool m_hasAnimation = false;
};
