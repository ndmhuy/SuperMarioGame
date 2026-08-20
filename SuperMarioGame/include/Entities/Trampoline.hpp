#pragma once

#include <string>

#include "Entities/Item.hpp"

class Trampoline : public Item {
public:
    explicit Trampoline(sf::Vector2f pos);
    ~Trampoline() override = default;

    std::string getTypeName() const override { return "trampoline"; }

    void update(float dt) override;
    void render(sf::RenderTarget& target) override;
    void activate(Player& player) override;
    void collect() override;
    void setupAnimations(const SpriteSheet* spriteSheet) override;

    // Mid-bounce, i.e. compressed and springing back. The harness used to call
    // isCompressed(), which disappeared when this became a timed bounce.
    bool isBouncing() const { return m_isBouncing; }
private:
    Animation m_idleAnim;
    Animation m_squishAnim;
    Animation m_extendAnim;
    float m_bounceTimer = 0.0f;
    bool m_isBouncing = false;
};
