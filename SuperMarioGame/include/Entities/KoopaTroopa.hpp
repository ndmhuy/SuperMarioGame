#pragma once

#include <string>

#include "Entities/Enemy.hpp"

class Player;

enum class KoopaState {
    Walking,
    ShellIdle,
    ShellHeld,
    ShellKicked
};

class KoopaTroopa : public Enemy {
public:
    explicit KoopaTroopa(sf::Vector2f position, bool isRed = false);
    virtual ~KoopaTroopa() override = default;

    std::string getTypeName() const override { return "koopa_troopa"; }

    void update(float dt) override;
    void render(sf::RenderTarget& target) override;
    void setupAnimations(const SpriteSheet* spriteSheet) override;

    void onStomped() override;
    void onHitByFireball() override;

    void kick(sf::Vector2f velocity);
    void pickUp(Player* holder);
    void throwShell(float speed = 420.0f, float angleDeg = 8.0f);

    // Put down rather than thrown — the carrier was hurt or died. The shell goes
    // back to resting on the spot, with its wake-up timer restarted.
    void release();

    // Getters for state/unit testing
    bool isRed() const { return m_isRed; }
    KoopaState getState() const { return m_state; }
    float getShellTimer() const { return m_shellTimer; }

    bool isCollidable() const override;
    bool isDeadOrDying() const override { return Enemy::isDeadOrDying() || m_state == KoopaState::ShellHeld; }

protected:
    bool m_isRed;
    KoopaState m_state = KoopaState::Walking;
    float m_shellTimer = 0.0f;
    // isFlipped()/m_isFlipped deliberately NOT redeclared here.
    //
    // This class used to shadow both with its own copy. onHitByFireball() then
    // set the *derived* flag while Enemy::isDeadOrDying() and
    // Enemy::collidesWithTiles() kept reading the base one — so a fireball
    // launched this enemy into the air and it never actually died, and it went
    // on colliding with tiles the whole time. That is why "the fireball must
    // kill the enemy": it hit, it knocked them back, and nothing else happened.
    Animation m_shellAnim;
    Player* m_holder = nullptr;
};
