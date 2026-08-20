#pragma once

#include "Entities/Projectile.hpp"
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
class Hammer : public Projectile {
public:
    bool hasArtwork() const override { return m_animator && m_hasAnimation; }
    sf::Vector2f artworkSize() const override {
        if (!m_animator || !m_hasAnimation) return {0.0f, 0.0f};
        const auto b = m_animator->getSprite().getLocalBounds();
        return {b.size.x, b.size.y};
    }

    Hammer(sf::Vector2f position, sf::Vector2f velocity);
    ~Hammer() override = default;

    std::string getTypeName() const override { return "hammer"; }

    // Damages the player only. It used to kill other enemies as well, because
    // the resolver treated it as a Fireball.
    bool damagesPlayer() const override { return true; }
    void onHitPlayer(Player& player) override;

    void update(float dt) override;
    void render(sf::RenderTarget& target) override;
    void setupAnimations(const SpriteSheet* spriteSheet) override;

    // Hammers pass through terrain — they are thrown over gaps and platforms.
    bool collidesWithTiles() const override { return false; }

    // See Fireball::resetForPool.
    void resetForPool(sf::Vector2f pos, sf::Vector2f vel);

private:
    float m_lifetime = 6.0f;
    float m_spin = 0.0f;

    std::unique_ptr<Animator> m_animator;
    Animation m_animation;
    bool m_hasAnimation = false;
};
