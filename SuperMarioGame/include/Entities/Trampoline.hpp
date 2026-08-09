#pragma once

#include "Entities/Item.hpp"

class Trampoline : public Item {
public:
    explicit Trampoline(sf::Vector2f pos);
    ~Trampoline() override = default;

    void update(float dt) override;
    void render(sf::RenderTarget& target) override;
    void activate(Player& player) override;
    void collect() override;
    void setupAnimations(const SpriteSheet* spriteSheet) override;
private:
    Animation m_idleAnim;
    Animation m_squishAnim;
    Animation m_extendAnim;
    float m_bounceTimer = 0.0f;
    bool m_isBouncing = false;
};
