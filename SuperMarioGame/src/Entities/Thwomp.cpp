#include "Entities/Thwomp.hpp"
#include "Entities/ProximityTriggerStrategy.hpp"
#include "Entities/Player.hpp"
#include "Core/Game.hpp"

Thwomp::Thwomp(sf::Vector2f position)
    : Enemy(position, 0) {
    // 2x2 tiles block size
    boundingBox.width = 64.0f;
    boundingBox.height = 64.0f;
    
    // Proximity trigger slam strategy
    setStrategy(std::make_unique<ProximityTriggerStrategy>());
}

void Thwomp::onStomped() {
    // Stone: cannot be stomped, inflicts damage
    Player* player = Game::getInstance().getPlayer();
    if (player) {
        player->takeDamage(1);
    }
}

void Thwomp::onHitByFireball() {
    // Immune to fireball
}
