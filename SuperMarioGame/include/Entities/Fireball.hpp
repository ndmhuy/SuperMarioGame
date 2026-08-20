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

    void update(float dt) override;
    void render(sf::RenderTarget& target) override;
    void setupAnimations(const SpriteSheet* spriteSheet) override;

    bool damagesEnemies() const override { return true; }
    void onHitEnemy(Enemy& enemy) override;

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
