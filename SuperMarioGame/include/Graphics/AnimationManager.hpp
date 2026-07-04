#pragma once

#include "Animation.hpp"
#include <string>
#include <unordered_map>

class AnimationManager {
public:
    static AnimationManager& getInstance();

    // Delete copy/move semantics for Singleton
    AnimationManager(const AnimationManager &) = delete;
    AnimationManager &operator=(const AnimationManager &) = delete;
    AnimationManager(AnimationManager &&) = delete;
    AnimationManager &operator=(AnimationManager &&) = delete;

    const Animation* getAnimation(const std::string& name) const;
    void addAnimation(const Animation& animation);

private:
    AnimationManager() = default;
    ~AnimationManager() = default;

    std::unordered_map<std::string,Animation> m_animationList;

};
