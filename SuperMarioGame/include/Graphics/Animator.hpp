#pragma once

#include <string>
#include "Animation.hpp"

class SpriteSheet;

class Animator {
public:
    explicit Animator(const SpriteSheet* spriteSheet);

    void play(const Animation* animation);
    void update(float dt);
    bool isDone() const;
    sf::Sprite getSprite() const;

private:
    const SpriteSheet* m_spriteSheet = nullptr;
    const Animation* m_animation = nullptr;
    float m_totalTime = 0;
    int m_curFrame = 0;
};