#pragma once

#include <string>

#include "Entities/Enemy.hpp"

class Goomba : public Enemy {
public:
    explicit Goomba(sf::Vector2f position, bool isRed = false);
    virtual ~Goomba() override = default;

    std::string getTypeName() const override { return "goomba"; }

    void update(float dt) override;
    void render(sf::RenderTarget& target) override;
    void setupAnimations(const SpriteSheet* spriteSheet) override;

    void onStomped() override;
    void onHitByFireball() override;

    // Getters for state/unit testing
    bool isRed() const { return m_isRed; }
    bool isSquished() const { return m_isSquished; }
    float getSquishTimer() const { return m_squishTimer; }

    const AABB& getBoundingBox() const override;
    bool isDeadOrDying() const override { return Enemy::isDeadOrDying() || m_isSquished; }
    bool collidesWithTiles() const override { return !m_isSquished && Enemy::collidesWithTiles(); }

private:
    bool m_isRed;
    bool m_isSquished = false;
    // isFlipped()/m_isFlipped deliberately NOT redeclared here.
    //
    // This class used to shadow both with its own copy. onHitByFireball() then
    // set the *derived* flag while Enemy::isDeadOrDying() and
    // Enemy::collidesWithTiles() kept reading the base one — so a fireball
    // launched this enemy into the air and it never actually died, and it went
    // on colliding with tiles the whole time. That is why "the fireball must
    // kill the enemy": it hit, it knocked them back, and nothing else happened.
    float m_squishTimer = 0.0f;
    Animation m_squishAnim;
};
