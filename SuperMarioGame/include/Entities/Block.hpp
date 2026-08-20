#pragma once

#include "Entities/Entity.hpp"
#include "Graphics/Animator.hpp"
#include "Graphics/SpriteSheet.hpp"
#include <memory>

class Player;

class Block : public Entity {
public:
    bool hasArtwork() const override { return m_animator && m_hasAnimation; }
    sf::Vector2f artworkSize() const override {
        if (!m_animator || !m_hasAnimation) return {0.0f, 0.0f};
        const auto b = m_animator->getSprite().getLocalBounds();
        return {b.size.x, b.size.y};
    }

    explicit Block(sf::Vector2f position, sf::Vector2f targetSize = {32.0f, 32.0f});
    ~Block() override = default;

    // Triggered when hit from below by a player
    virtual void onHitFromBelow(Player& player) = 0;

    // Overrides Entity lifecycle & physics
    float getGravityMultiplier() const override { return 0.0f; }
    EntityCategory getCategory() const override { return EntityCategory::Block; }
    void update(float dt) override;
    void render(sf::RenderTarget& target) override;
    virtual void setupAnimations(const SpriteSheet* spriteSheet);

    bool isBreakable() const { return m_breakable; }


protected:
    bool m_breakable = false;
    bool m_isHit = false;
    float m_bumpTimer = 0.0f;
    sf::Vector2f m_originalPosition;

    std::unique_ptr<Animator> m_animator;
    Animation m_animation;
    bool m_hasAnimation = false;
};

