#pragma once

#include "Entities/Projectile.hpp"
#include "Graphics/Animator.hpp"
#include "Graphics/SpriteSheet.hpp"
#include <memory>
#include <string>

class Player;

// Bowser's fire breath.
//
// Deliberately not a Fireball: the player's fireball arcs under gravity, bounces
// four times and kills enemies. This travels flat, ignores terrain and hurts
// only the player. Sharing one class would have meant a flag on Fireball read
// by everything that touches it.
class BossFireball : public Projectile {
public:
    bool hasArtwork() const override { return m_animator && m_hasAnimation; }
    sf::Vector2f artworkSize() const override {
        if (!m_animator || !m_hasAnimation) return {0.0f, 0.0f};
        const auto b = m_animator->getSprite().getLocalBounds();
        return {b.size.x, b.size.y};
    }

    BossFireball(sf::Vector2f position, sf::Vector2f velocity);
    ~BossFireball() override = default;

    std::string getTypeName() const override { return "boss_fireball"; }
    PoolTag poolTag() const override { return PoolTag::BossFireball; }

    void update(float dt) override;
    void render(sf::RenderTarget& target) override;
    void setupAnimations(const SpriteSheet* spriteSheet) override;

    bool damagesPlayer() const override { return true; }
    void onHitPlayer(Player& player) override;

    // Flies level through the arena; walls would stop it before it crossed.
    bool collidesWithTiles() const override { return false; }
    float getGravityMultiplier() const override { return 0.0f; }

    // See Fireball::resetForPool.
    void resetForPool(sf::Vector2f pos, sf::Vector2f vel);

private:
    // Projectile derives from Entity, which has no facing — Character owns that.
    // The atlas ships a directional pair, so the travel direction is kept here.
    bool m_travellingRight = true;
    float m_lifetime = 4.0f;

    std::unique_ptr<Animator> m_animator;
    Animation m_animation;
    bool m_hasAnimation = false;
};
