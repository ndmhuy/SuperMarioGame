#include "Entities/POWBlock.hpp"
#include "Entities/Player.hpp"
#include "Physics/CollisionDetector.hpp"
#include "Core/EventBus.hpp"
#include <SFML/Graphics/RectangleShape.hpp>
#include <cmath>

POWBlock::POWBlock(sf::Vector2f pos) : Item(pos) {
    velocity = sf::Vector2f{0.0f, 0.0f};
}

void POWBlock::update(float dt) {
    Item::update(dt);
    if (m_strikeCooldown > 0.0f) m_strikeCooldown -= dt;
}

void POWBlock::setupAnimations(const SpriteSheet* spriteSheet) {
    Item::setupAnimations(spriteSheet);
    m_animation = Animation("pow_block");
    m_animation.frameList = {
        {"pow_block_0", 0.15f},
        {"pow_block_1", 0.15f},
        {"pow_block_2", 0.15f},
        {"pow_block_3", 0.15f},
        {"pow_block_4", 0.15f},
        {"pow_block_5", 0.15f},
        {"pow_block_6", 0.15f},
        {"pow_block_7", 0.15f}
    };
    if (m_animator) {
        m_animator->play(&m_animation);
        m_hasAnimation = true;
    }
}

void POWBlock::render(sf::RenderTarget& target) {
    Item::render(target);
}

void POWBlock::activate(Player& player) {
    // One strike per press. The resolver calls this for every frame of contact,
    // so without the cooldown a single hit spent all three charges at once and
    // the block looked like it did nothing but disappear.
    if (m_strikeCooldown > 0.0f || isSpent()) return;
    m_strikeCooldown = 0.25f;
    --m_charges;

    // PlayingState listens for this and knocks over every grounded enemy on
    // screen; Camera listens for the shake; SoundManager plays the slam.
    EventBus::getInstance().publish({EventType::POWBlockHit, this});

    // Spent blocks leave the world. Collecting it is how an Item removes
    // itself, and the resolver already skips collected items.
    if (isSpent()) {
        collect();
        destroy();
    }
}

ItemTouch POWBlock::onPlayerTouch(Player& player, const CollisionInfo& info) {
    // A strike is a hit from below (the arcade original) or a genuine descending
    // stomp onto the top. Brushing past the side does nothing, and neither does
    // resting on it — the descent-speed floor is what separates "landed on it"
    // from "standing on it", the same distinction Boss::STOMP_MIN_DESCENT_SPEED
    // draws.
    //
    // The approach speed is read here, before the resolver's push-out cancels
    // it. activate() ignores the player entirely, so running it before the
    // push-out rather than after is the same block being struck either way.
    constexpr float MIN_STRIKE_DESCENT = 60.0f;
    const float approachY = player.getVelocity().y;
    const bool struckFromBelow = info.normal.y == 1.0f;
    const bool stomped = info.normal.y == -1.0f && approachY >= MIN_STRIKE_DESCENT;
    if (struckFromBelow || stomped) {
        activate(player);
    }

    // Solid in every direction, exactly like a brick — the POW block is terrain
    // you can stand on. It used to fall through Item's default pickup path, so
    // walking sideways into it "collected" it.
    return ItemTouch::Solid;
}
