#pragma once

#include <SFML/Graphics.hpp>
#include <vector>
#include "Graphics/SpriteTransformAnim.hpp"
#include "Graphics/SpriteColorFilter.hpp"

enum class DeathEffectType {
    EnemyFlip,       // 180° Y-flip + arc launch upward + fall off-screen
    StarKillSpin,    // Continuous 720° spin launch + CoinSparkle burst (no rainbow flash on sprite)
    PlayerDeathHop   // Freeze (0.5s) -> Hop upward -> Fall through terrain off-screen
};

struct FloatingDeathInstance {
    sf::Vector2f position;
    sf::Vector2f velocity;
    sf::Sprite sprite;
    DeathEffectType type;
    float elapsedTime = 0.0f;
    float freezeTimer = 0.5f; // Used for Player Death Freeze phase
    float rotation = 0.0f;
    bool active = true;

    FloatingDeathInstance(sf::Vector2f pos, sf::Vector2f vel, const sf::Sprite& spr, DeathEffectType t, float freeze = 0.5f)
        : position(pos), velocity(vel), sprite(spr), type(t), freezeTimer(freeze) {}
};

class EntityDeathEffect {
public:
    static EntityDeathEffect& getInstance();

    // Spawn a floating death visual instance
    void spawnDeathEffect(sf::Vector2f startPosition, sf::Sprite spriteFrame, DeathEffectType type, sf::Vector2f launchVelocity = {0.0f, -380.0f});

    void update(float dt, float cameraBottomY);
    void render(sf::RenderTarget& target);
    void clear();

    const std::vector<FloatingDeathInstance>& getInstances() const { return m_instances; }

private:
    EntityDeathEffect() = default;
    std::vector<FloatingDeathInstance> m_instances;
};
