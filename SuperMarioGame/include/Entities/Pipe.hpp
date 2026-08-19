#pragma once

#include "Entities/Block.hpp"
#include <string>

class Pipe : public Block {
public:
    explicit Pipe(sf::Vector2f position, int pipeId = 0, sf::Vector2f exitPosition = {0.0f, 0.0f}, std::string targetLevel = "", bool isEntrance = false, float rotationDegrees = 0.0f);
    ~Pipe() override = default;

    std::string getTypeName() const override { return "pipe"; }

    void onHitFromBelow(Player& player) override;
    void render(sf::RenderTarget& target) override;
    void setupAnimations(const SpriteSheet* spriteSheet) override;

    // Checks if warp conditions (player standing on top and pressing down) are met
    bool checkWarp(Player& player) const;

    int getPipeId() const { return m_pipeId; }
    sf::Vector2f getExitPosition() const { return m_exitPosition; }
    std::string getTargetLevel() const { return m_targetLevel; }
    bool isEntrance() const { return m_isEntrance; }
    float getRotationDegrees() const { return m_rotationDegrees; }
    void setRotationDegrees(float deg) { m_rotationDegrees = deg; }

private:
    int m_pipeId = 0;
    sf::Vector2f m_exitPosition;
    std::string m_targetLevel;
    bool m_isEntrance = false;
    float m_rotationDegrees = 0.0f;
    const SpriteSheet* m_spriteSheet = nullptr;
};
