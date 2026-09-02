#pragma once

#include "Entities/IMovementStrategy.hpp"
#include <string>
#include <SFML/System/Vector2.hpp>

class TetheredChaseStrategy : public IMovementStrategy {
public:
    explicit TetheredChaseStrategy(sf::Vector2f anchorPos = sf::Vector2f(0.f, 0.f), float tetherRadius = 128.0f);
    virtual ~TetheredChaseStrategy() override = default;

    std::string getName() const override { return "TetheredChase"; }

    sf::Vector2f getAnchorPos() const;
    void setAnchorPos(sf::Vector2f anchorPos);
    void translateAnchor(sf::Vector2f delta) override { setAnchorPos(m_anchorPos + delta); }
    float getTetherRadius() const;
    void setTetherRadius(float radius);

    // Triggered when hitting the player or chain limit to knock back and reset
    void triggerRecoil(sf::Vector2f recoilDir);

protected:
    void calculateTarget(Enemy& enemy, float dt) override;
    void applyMovement(Enemy& enemy, float dt) override;
    void checkConstraints(Enemy& enemy, float dt) override;

private:
    sf::Vector2f m_anchorPos;
    float m_tetherRadius;
    float m_timer;
    bool m_anchorInitialized;
    bool m_isLunging;
    sf::Vector2f m_lungeDir;
    float m_lungeTimer;     // Duration of active lunge forward
    float m_cooldownTimer;  // Rest/cooldown period between lunges to avoid camping
    float m_recoilTimer;    // Knockback recoil timer
};
