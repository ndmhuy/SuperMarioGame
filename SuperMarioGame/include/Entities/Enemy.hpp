#pragma once

#include "Entities/Character.hpp"
#include "Entities/IMovementStrategy.hpp"
#include "Graphics/Animator.hpp"
#include "Graphics/SpriteSheet.hpp"
#include <memory>

class Enemy : public Character {
public:
    bool hasArtwork() const override { return m_animator && m_hasAnimation; }
    sf::Vector2f artworkSize() const override {
        if (!m_animator || !m_hasAnimation) return {0.0f, 0.0f};
        const auto b = m_animator->getSprite().getLocalBounds();
        return {b.size.x, b.size.y};
    }

    explicit Enemy(sf::Vector2f position, int scoreValue = 100, sf::Vector2f targetSize = {32.0f, 32.0f});
    virtual ~Enemy() override = default;

    // Overrides Entity lifecycle
    void update(float dt) override;
    void render(sf::RenderTarget& target) override;
    virtual void setupAnimations(const SpriteSheet* spriteSheet);

    // Strategy setters/getters
    void setStrategy(std::unique_ptr<IMovementStrategy> strategy);
    IMovementStrategy* getStrategy() const;

    // Virtual interaction handlers
    virtual void onStomped() = 0;
    virtual void onHitByFireball() = 0;

    EntityCategory getCategory() const override { return EntityCategory::Enemy; }

    // Score properties
    int getScoreValue() const;
    void setScoreValue(int value);

    // Scales this enemy's movement speed once, as it enters the world. Used by
    // the difficulty strategy (task 9.4); deliberately an action rather than a
    // setSpeed(), so callers cannot reach in and overwrite tuned values.
    void applySpeedScale(float scale);

    // Sets the tuned base speed. Used only by EntityFactory when applying
    // assets/config/entities.json; gameplay code scales, it does not assign.
    void setSpeed(float newSpeed);

    bool isFlipped() const { return m_isFlipped; }
    bool isDyingDownward() const { return m_isDyingDownward; }
    virtual bool isDeadOrDying() const { return !active || m_isFlipped || m_isDyingDownward; }
    void triggerFlipDeath(sf::Vector2f launchVel = {80.0f, -250.0f});
    void triggerDownwardDeath(sf::Vector2f launchVel = {0.0f, 150.0f});
    bool isCollidable() const override;
    bool collidesWithTiles() const override { return !m_isFlipped && !m_isDyingDownward; }

protected:
    // Despawn plane for a flipped/falling enemy: one tile below the current
    // level's own tilemap height, not a fixed screen constant — correct for
    // any level height, not only the 720px window (audit D2).
    static float fallDespawnPlaneY();

    std::unique_ptr<IMovementStrategy> m_aiStrategy;
    int m_scoreValue;
    bool m_isFlipped = false;
    bool m_isDyingDownward = false;

    std::unique_ptr<Animator> m_animator;
    Animation m_animation;
    bool m_hasAnimation = false;
};


