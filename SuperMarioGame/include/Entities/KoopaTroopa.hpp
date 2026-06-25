#pragma once

#include "Entities/Enemy.hpp"

enum class KoopaState {
    Walking,
    ShellIdle,
    ShellKicked
};

class KoopaTroopa : public Enemy {
public:
    explicit KoopaTroopa(sf::Vector2f position, bool isRed = false);
    virtual ~KoopaTroopa() override = default;

    void update(float dt) override;
    void render(sf::RenderTarget& target) override;

    void onStomped() override;
    void onHitByFireball() override;

    void kick(sf::Vector2f velocity);

    // Getters for state/unit testing
    bool isRed() const { return m_isRed; }
    KoopaState getState() const { return m_state; }
    float getShellTimer() const { return m_shellTimer; }
    bool isFlipped() const { return m_isFlipped; }

    AABB getBoundingBox() const override;

protected:
    bool m_isRed;
    KoopaState m_state = KoopaState::Walking;
    float m_shellTimer = 0.0f;
    bool m_isFlipped = false;
};
