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
    // Built on demand rather than kept in a function-local static.
    //
    // The static version held a reference to a ResourceManager texture and was
    // destroyed during static destruction — after ResourceManager::clear() had
    // already freed that texture, and after the graphics context was gone. It
    // also initialised itself by asking getTexture(""), which is what printed
    // the stray "Texture not found: " with an empty id once per run.
    //
    // An sf::Sprite is a transform, a texture pointer and a rect; constructing
    // one per call costs nothing, and the normal paths below already construct
    // one every call.
    auto blank = [] {
        return sf::Sprite(ResourceManager::getInstance().placeholderTexture());
    };

    if (m_spriteSheet == nullptr) return blank();
    if (m_animation == nullptr) return blank();
    if (m_animation->frameList.size() && isDone())
        return m_spriteSheet->getSprite(m_animation->frameList.back().frameName);

    const std::string& frameName = m_animation->frameList[m_curFrame].frameName;
    return m_spriteSheet->getSprite(frameName); 
}