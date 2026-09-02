#pragma once

#include "Entities/Projectile.hpp"
#include "Graphics/Animator.hpp"
#include "Graphics/SpriteSheet.hpp"
#include <memory>

class Enemy;

// The player's fire-flower shot: bounces along the ground and kills what it
// touches. Harmless to the player who threw it.
class Fireball : public Projectile {
public:
    bool hasArtwork() const override { return m_animator && m_hasAnimation; }
    sf::Vector2f artworkSize() const override {
        if (!m_animator || !m_hasAnimation) return {0.0f, 0.0f};
        const auto b = m_animator->getSprite().getLocalBounds();
        return {b.size.x, b.size.y};
    }

    Fireball(sf::Vector2f pos, sf::Vector2f vel);
    ~Fireball() override = default;

    std::string getTypeName() const override { return "fireball"; }
    PoolTag poolTag() const override { return PoolTag::Fireball; }

    void update(float dt) override;
    void render(sf::RenderTarget& target) override;
    void setupAnimations(const SpriteSheet* spriteSheet) override;

    bool damagesEnemies() const override { return true; }
    void onHitEnemy(Enemy& enemy) override;
    bool onTileImpact(const CollisionInfo& info) override;

    // A shot that has already burst is spent, and must not hit anything else.
    //
    // It stays alive for the three frames of the burst animation, and it used to
    // stay collidable for all of them — so the broadphase went on pairing it
    // with whatever it had just hit and calling onHitEnemy() again every frame.
    // Against an ordinary enemy that is invisible (the first hit killed it), but
    // Bowser survives fire on purpose and counts the hits: one fireball scored
    // three of the four an opening costs. Observed, frames 0-2 of
    // tests/verify_r21_boss_combat.cpp's repro.
    bool isCollidable() const override { return active && m_impactTimer <= 0.0f; }

    // Nor does a burst move. beginImpact() zeroes the velocity, but gravity
    // belongs to the physics engine and it kept adding to it: the burst slid
    // down the screen over its three frames and could even spend one of the
    // bounces the flying shot had left. It is an animation playing where the
    // shot ended, so it stays where the shot ended.
    bool isPhysicsDriven() const override { return m_impactTimer <= 0.0f; }

    float getLifetime() const { return m_lifetime; }
    int getBouncesLeft() const { return m_bouncesLeft; }
    bool isImpacting() const { return m_impactTimer > 0.0f; }

    void bounce();

    // Puts a recycled fireball back into its just-constructed state. Required by
    // ObjectPool<Fireball>; the pool has no idea what "fresh" means for a
    // fireball, and getting this wrong means a reused shot arrives already spent.
    void resetForPool(sf::Vector2f pos, sf::Vector2f vel);

private:
    // Every way a fireball ends goes through here: it stops moving, plays the
    // burst, and destroys itself once the burst is done. Calling destroy()
    // directly would skip the animation the atlas has always shipped.
    void beginImpact();

    float m_lifetime = 3.0f; // 3 seconds max lifetime
    int m_bouncesLeft = 4;
    float m_animTimer = 0.0f;
    // Counts down through the three-frame burst; zero means "still flying".
    float m_impactTimer = 0.0f;

    std::unique_ptr<Animator> m_animator;
    Animation m_flightAnim;
    Animation m_impactAnim;
    bool m_hasAnimation = false;
};
