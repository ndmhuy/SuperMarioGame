#pragma once

#include "Entities/Block.hpp"
#include <string>

class Pipe : public Block {
public:
    explicit Pipe(sf::Vector2f position, int pipeId = 0, sf::Vector2f exitPosition = {0.0f, 0.0f}, std::string targetLevel = "", bool isEntrance = false);
    ~Pipe() override = default;

    void onHitFromBelow(Player& player) override;
    void render(sf::RenderTarget& target) override;

    // Checks if warp conditions (player standing on top and pressing down) are met
    bool checkWarp(Player& player) const;

    int getPipeId() const { return m_pipeId; }
    sf::Vector2f getExitPosition() const { return m_exitPosition; }
    std::string getTargetLevel() const { return m_targetLevel; }
    bool isEntrance() const { return m_isEntrance; }

private:
    int m_pipeId = 0;
    sf::Vector2f m_exitPosition;
    std::string m_targetLevel;
    bool m_isEntrance = false;
};
