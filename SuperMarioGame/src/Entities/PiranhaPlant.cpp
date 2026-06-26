#include "Entities/PiranhaPlant.hpp"
#include "Entities/TimerEmergenceStrategy.hpp"
#include "Entities/Player.hpp"
#include "Core/Game.hpp"
#include "Core/EventBus.hpp"
#include "Core/SoundManager.hpp"

PiranhaPlant::PiranhaPlant(sf::Vector2f position)
    : Enemy(position, 100) {
    // Height is 48px, width is 32px (fits in a pipe)
    boundingBox.width = 32.0f;
    boundingBox.height = 48.0f;
    
    // Set AI emergence strategy anchored at the spawn position
    setStrategy(std::make_unique<TimerEmergenceStrategy>(position));
}

void PiranhaPlant::onStomped() {
    // Spiky/Biting: Cannot be stomped, damages player instead
    Player* player = Game::getInstance().getPlayer();
    if (player) {
        player->takeDamage(1);
    }
}

void PiranhaPlant::onHitByFireball() {
    SoundManager::getInstance().playSound("kick");
    EventBus::getInstance().publish({EventType::EnemyDefeated, m_scoreValue});
    this->active = false;
}
