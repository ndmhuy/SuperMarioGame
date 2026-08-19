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
    void throwShell(float speed = 550.0f, float angleDeg = 45.0f);

    // Getters for state/unit testing
    bool isRed() const { return m_isRed; }
    KoopaState getState() const { return m_state; }
    float getShellTimer() const { return m_shellTimer; }
    bool isFlipped() const { return m_isFlipped; }

    const AABB& getBoundingBox() const override;
    bool isDeadOrDying() const override { return Enemy::isDeadOrDying() || m_state == KoopaState::ShellHeld; }

protected:
    bool m_isRed;
    KoopaState m_state = KoopaState::Walking;
    float m_shellTimer = 0.0f;
    bool m_isFlipped = false;
    Animation m_shellAnim;
    Player* m_holder = nullptr;
};
