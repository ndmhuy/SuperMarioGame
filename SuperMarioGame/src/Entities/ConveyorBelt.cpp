#include "Entities/ConveyorBelt.hpp"
#include "Entities/Player.hpp"
#include "Core/Game.hpp"
#include <SFML/Graphics/RectangleShape.hpp>
#include <cmath>

ConveyorBelt::ConveyorBelt(sf::Vector2f position, bool pushRight, float pushSpeed)
    : Block(position), m_pushRight(pushRight), m_pushSpeed(pushSpeed) {
    m_breakable = false;
}

void ConveyorBelt::onHitFromBelow(Player& player) {
    // Conveyor belt hit from below
}

void ConveyorBelt::update(float dt) {
    Block::update(dt);

    Player* player = Game::getInstance().getPlayer();
    if (player) {
        AABB pBox = player->getBoundingBox();
        AABB conveyorBox = getBoundingBox();

        // 1. Horizontal overlap check
        bool xOverlap = (pBox.x + pBox.width > conveyorBox.x) && (pBox.x < conveyorBox.x + conveyorBox.width);
        
        // 2. Vertical alignment check (bottom of player close to top of conveyor)
        bool yOverlap = std::abs((pBox.y + pBox.height) - conveyorBox.y) < 3.0f;
        
        // 3. Resting / not moving upward
        bool resting = player->getVelocity().y >= 0.0f;

        if (xOverlap && yOverlap && resting) {
            float pushAmount = (m_pushRight ? 1.0f : -1.0f) * m_pushSpeed * dt;
            player->setPosition(player->getPosition() + sf::Vector2f(pushAmount, 0.0f));
        }
    }
}

void ConveyorBelt::setupAnimations(const SpriteSheet* spriteSheet) {
    Block::setupAnimations(spriteSheet);
    m_animation = Animation("conveyor_belt");
    // This named "platform_long", which world_scenery_item does not contain.
    // setupAnimations() sets m_hasAnimation whether or not the frames exist,
    // and drawSprite() returns early on a zero-size sprite, so the conveyor belt
    // drew nothing at all — not even the placeholder rectangle.
    m_animation.frameList = {{"conveyor_belt_green", 0.12f}, {"conveyor_belt_white", 0.12f}};
    if (m_animator) {
        m_animator->play(&m_animation);
        m_hasAnimation = true;
    }
}

void ConveyorBelt::render(sf::RenderTarget& target) {
    Block::render(target);
}
