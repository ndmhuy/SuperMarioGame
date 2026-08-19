#include "Entities/PiranhaPlant.hpp"
#include "Utils/Constants.hpp"
#include "Entities/TimerEmergenceStrategy.hpp"
#include "Entities/Player.hpp"
#include "Core/Game.hpp"
#include "Core/EventBus.hpp"
#include "Core/SoundManager.hpp"

PiranhaPlant::PiranhaPlant(sf::Vector2f position)
    : Enemy(position, 100, {32.0f, 48.0f}) {
    // Character::speed was left at zero here, so every strategy substituted a
    // literal and the difficulty modifier had nothing to scale.
    speed = Constants::ENEMY_PIRANHA_SPEED;

    // Set AI emergence strategy anchored at the spawn position
    setStrategy(std::make_unique<TimerEmergenceStrategy>(position));
}

void PiranhaPlant::setupAnimations(const SpriteSheet* spriteSheet) {
    Enemy::setupAnimations(spriteSheet);
    m_animation = Animation("pirhana");
    m_animation.frameList = {{"pirhana_green_0", 0.15f}, {"pirhana_green_1", 0.15f}};
    if (m_animator) {
        m_animator->play(&m_animation);
        m_hasAnimation = true;
    }
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
