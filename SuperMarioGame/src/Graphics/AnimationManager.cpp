#include "Graphics/AnimationManager.hpp"

AnimationManager& AnimationManager::getInstance() {
    static AnimationManager instance;
    return instance;
}

const Animation* AnimationManager::getAnimation(const std::string& name) const {
    auto it = m_animationList.find(name);
    if (it == m_animationList.end()) return nullptr;
    return &(it->second);
}

void AnimationManager::addAnimation(const Animation& animation) {
    m_animationList[animation.name] = animation;
}
