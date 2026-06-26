#pragma once

#include "Entities/Entity.hpp"

class Player;

class Block : public Entity {
public:
    explicit Block(sf::Vector2f position);
    ~Block() override = default;

    // Triggered when hit from below by a player
    virtual void onHitFromBelow(Player& player) = 0;

    // Overrides Entity lifecycle
    void update(float dt) override;
    void render(sf::RenderTarget& target) override = 0;

    bool isBreakable() const { return m_breakable; }

protected:
    bool m_breakable = false;
    bool m_isHit = false;
    float m_bumpTimer = 0.0f;
    sf::Vector2f m_originalPosition;
};
