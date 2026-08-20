#include "Graphics/EntityDeathEffect.hpp"
#include "Graphics/ParticleSystem.hpp"
#include "Graphics/ParticleEmitter.hpp"
#include <algorithm>
#include <cmath>

EntityDeathEffect& EntityDeathEffect::getInstance() {
    static EntityDeathEffect instance;
    return instance;
}

void EntityDeathEffect::spawnDeathEffect(sf::Vector2f startPosition, sf::Sprite spriteFrame, DeathEffectType type, sf::Vector2f launchVelocity) {
    float freezeTimer = (type == DeathEffectType::PlayerDeathHop) ? 0.5f : 0.0f;
    FloatingDeathInstance inst(startPosition, launchVelocity, spriteFrame, type, freezeTimer);

    // Standardize origin to center for proper flip / spin rotation
    sf::FloatRect bounds = inst.sprite.getLocalBounds();
    if (bounds.size.x > 0.0f && bounds.size.y > 0.0f) {
        inst.sprite.setOrigin(sf::Vector2f(bounds.size.x / 2.0f, bounds.size.y / 2.0f));
    }

    if (type == DeathEffectType::StarKillSpin) {
        // Spawn particle sparkle burst (`ParticleType::CoinSparkle`)
        ParticleEmitter emitter;
        emitter.burst(startPosition, ParticleType::CoinSparkle);
    }

    m_instances.push_back(inst);
}

void EntityDeathEffect::update(float dt, float cameraBottomY) {
    const float gravity = 1800.0f; // gravity acceleration in px/s^2

    for (auto& inst : m_instances) {
        if (!inst.active) continue;

        inst.elapsedTime += dt;

        if (inst.type == DeathEffectType::PlayerDeathHop) {
            if (inst.freezeTimer > 0.0f) {
                inst.freezeTimer -= dt;
                if (inst.freezeTimer <= 0.0f) {
                    inst.freezeTimer = 0.0f;
                    inst.velocity = sf::Vector2f(0.0f, -450.0f); // Launch hop impulse
                } else {
                    // Still in freeze phase
                    continue;
                }
            }
        }

        // Apply trajectory physics
        inst.velocity.y += gravity * dt;
        inst.position += inst.velocity * dt;

        if (inst.type == DeathEffectType::StarKillSpin) {
            inst.rotation += 720.0f * dt; // Continuous 720°/s spin
            inst.rotation = std::fmod(inst.rotation, 360.0f);
        }

        // Despawn condition: off-screen below camera bottom + 100px threshold
        if (inst.position.y > cameraBottomY + 100.0f) {
            inst.active = false;
        }
    }

    // Clean up inactive instances
    m_instances.erase(
        std::remove_if(m_instances.begin(), m_instances.end(), [](const FloatingDeathInstance& inst) {
            return !inst.active;
        }),
        m_instances.end()
    );
}

void EntityDeathEffect::render(sf::RenderTarget& target) {
    for (auto& inst : m_instances) {
        if (!inst.active) continue;

        sf::Sprite renderSpr = inst.sprite;
        renderSpr.setPosition(inst.position);

        if (inst.type == DeathEffectType::EnemyFlip) {
            // Upside-down Y-flip with scale.y = -abs(scale.y)
            sf::Vector2f curScale = renderSpr.getScale();
            float absX = std::abs(curScale.x == 0.0f ? 1.0f : curScale.x);
            float absY = std::abs(curScale.y == 0.0f ? 1.0f : curScale.y);
            renderSpr.setScale(sf::Vector2f(absX, -absY));
        } else if (inst.type == DeathEffectType::StarKillSpin) {
            renderSpr.setRotation(sf::degrees(inst.rotation));
        }

        target.draw(renderSpr);
    }
}

void EntityDeathEffect::clear() {
    m_instances.clear();
}
