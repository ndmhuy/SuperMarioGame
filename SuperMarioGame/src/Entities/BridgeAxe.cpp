#include "Entities/BridgeAxe.hpp"
#include "Entities/Player.hpp"
#include "Core/EventBus.hpp"
#include "Core/SoundManager.hpp"

BridgeAxe::BridgeAxe(sf::Vector2f pos) : Item(pos, {32.0f, 32.0f}) {
    velocity = sf::Vector2f{0.0f, 0.0f};
}

void BridgeAxe::update(float dt) {
    Item::update(dt);
}

void BridgeAxe::setupAnimations(const SpriteSheet* spriteSheet) {
    Item::setupAnimations(spriteSheet);

    // A slow glint rather than a fast spin: the axe is a landmark the player has
    // to notice from across the bridge, and a three-frame loop at 0.15s reads as
    // flickering at that distance.
    m_animation = Animation("bridge_axe");
    m_animation.frameList = {{"axe_0", 0.28f}, {"axe_1", 0.28f}, {"axe_2", 0.28f}};
    m_animation.isLooping = true;

    if (m_animator && spriteSheet && spriteSheet->hasFrame("axe_0")) {
        m_animator->play(&m_animation);
        m_hasAnimation = true;
    }
}

void BridgeAxe::activate(Player& player) {
    if (m_swung) return;
    m_swung = true;

    SoundManager::getInstance().playSound("break_brick_block");
    EventBus::getInstance().publish({EventType::BridgeChopped, this});

    // The axe leaves with the bridge it cut.
    collect();
    destroy();
}
