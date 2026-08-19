#pragma once

#include "Entities/Entity.hpp"
#include "Graphics/Animator.hpp"
#include "Graphics/SpriteSheet.hpp"
#include <memory>

class Player;

class Item : public Entity {
public:
    explicit Item(sf::Vector2f pos, sf::Vector2f targetSize = {32.0f, 32.0f});
    ~Item() override = default;

    // Collect/Apply powerup callbacks
    virtual void activate(Player& player);
    virtual void collect();
    virtual void setupAnimations(const SpriteSheet* spriteSheet);

    void update(float dt) override;
    void render(sf::RenderTarget& target) override;
    EntityCategory getCategory() const override { return EntityCategory::Item; }

    // Ground status & read-only getter
    bool isCollected() const { return collected; }
    bool isOnGround() const { return m_onGround; }
    void setOnGround(bool grounded) { m_onGround = grounded; }

protected:
    bool collected = false;
    bool m_onGround = false;

    std::unique_ptr<Animator> m_animator;
    Animation m_animation;
    bool m_hasAnimation = false;
    float m_baseScale = 0.0f;
    const SpriteSheet* m_spriteSheet = nullptr;
};


