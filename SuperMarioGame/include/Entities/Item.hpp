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

    // Read-only getter
    bool isCollected() const { return collected; }

protected:
    bool collected = false;
    std::unique_ptr<Animator> m_animator;
    Animation m_animation;
    bool m_hasAnimation = false;
    float m_baseScale = 0.0f;
};


