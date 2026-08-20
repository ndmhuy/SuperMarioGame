#pragma once

#include <string>

#include "Entities/Enemy.hpp"

class Spiny : public Enemy {
public:
    explicit Spiny(sf::Vector2f position, bool isEgg = false);
    ~Spiny() override = default;

    std::string getTypeName() const override { return "spiny"; }

    void onStomped() override;
    // Spikes: onStomped() damages the player.
    bool isStompSafe() const override { return false; }
    void onHitByFireball() override;
    void update(float dt) override;
    void setupAnimations(const SpriteSheet* spriteSheet) override;
    bool isCollidable() const override;

    bool isEgg() const { return m_isEgg; }
    void setEgg(bool isEgg) { m_isEgg = isEgg; }

private:
    // isFlipped()/m_isFlipped deliberately NOT redeclared here.
    //
    // This class used to shadow both with its own copy. onHitByFireball() then
    // set the *derived* flag while Enemy::isDeadOrDying() and
    // Enemy::collidesWithTiles() kept reading the base one — so a fireball
    // launched this enemy into the air and it never actually died, and it went
    // on colliding with tiles the whole time. That is why "the fireball must
    // kill the enemy": it hit, it knocked them back, and nothing else happened.
    bool m_isEgg = false;
    Animation m_eggAnim;
};
