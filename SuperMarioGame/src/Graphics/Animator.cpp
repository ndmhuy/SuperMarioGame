#include "Graphics/Animator.hpp"

#include "Core/ResourceManager.hpp"
#include "Graphics/SpriteSheet.hpp"

Animator::Animator(const SpriteSheet* spriteSheet)
    : m_spriteSheet(spriteSheet) {}

void Animator::setSpriteSheet(const SpriteSheet* spriteSheet) {
    m_spriteSheet = spriteSheet;
}

void Animator::play(const Animation* animation) {
    if (m_animation == animation) return;
    m_animation = animation;
    m_totalTime = 0;
    m_curFrame = 0;
}

void Animator::update(float dt) {
    if (m_spriteSheet == nullptr) return;
    if (m_animation == nullptr) return;

    m_totalTime += dt;
    const std::vector<KeyFrame>& frameList = m_animation->frameList;
    while (m_curFrame < frameList.size() && m_totalTime >= frameList[m_curFrame].duration) {
        m_totalTime -= frameList[m_curFrame].duration;
        m_curFrame++;
        if (m_animation->isLooping && m_curFrame == frameList.size()) m_curFrame = 0;
    }
}

bool Animator::isDone() const {
    if (m_animation == nullptr) return true;
    return m_curFrame >= m_animation->frameList.size();
}

sf::Sprite Animator::getSprite() const {
    static sf::Sprite dummySprite(
        ResourceManager::getInstance().getTexture("")
    );

    if (m_spriteSheet == nullptr) return dummySprite;
    if (m_animation == nullptr) return dummySprite;
    if (m_animation->frameList.size() && isDone())
        return m_spriteSheet->getSprite(m_animation->frameList.back().frameName);

    const std::string& frameName = m_animation->frameList[m_curFrame].frameName;
    return m_spriteSheet->getSprite(frameName); 
}