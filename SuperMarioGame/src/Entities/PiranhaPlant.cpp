#include "Entities/PiranhaPlant.hpp"
#include "Utils/Constants.hpp"
#include "Entities/TimerEmergenceStrategy.hpp"
#include "Entities/Player.hpp"
#include "Core/Game.hpp"
#include "Core/EventBus.hpp"
#include "Core/SoundManager.hpp"
#include <algorithm>

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

// How much of the plant is currently clear of the pipe mouth, in pixels. The
// strategy holds the mouth position; the plant rises above it as it emerges.
float PiranhaPlant::emergedHeight() const {
    const auto* emergence = dynamic_cast<const TimerEmergenceStrategy*>(getStrategy());
    if (!emergence) return m_targetSize.y;      // no strategy: draw it whole

    // How far the plant has risen above its anchor. The anchor IS the pipe
    // mouth: the strategy parks the plant exactly there when retracted and
    // lifts it two tiles to emerge. So "risen" is also "how much of the plant is
    // out of the pipe".
    //
    // The first version of this computed the same quantity with two extra terms
    // that cancelled to +m_targetSize.y, so it reported the plant fully visible
    // while retracted — which is the state it was supposed to hide.
    const float risen = emergence->getAnchorPos().y - position.y;
    return std::clamp(risen, 0.0f, m_targetSize.y);
}

void PiranhaPlant::render(sf::RenderTarget& target) {
    if (!active) return;

    const float visible = emergedHeight();
    if (visible <= 0.5f) return;                // entirely inside the pipe

    if (!m_animator || !m_hasAnimation) {
        drawPlaceholder(target, sf::Color::Red);
        return;
    }

    // Crop the sprite to its top `visible` pixels, so the plant slides out of
    // the pipe rather than being drawn over it.
    sf::Sprite sprite = m_animator->getSprite();
    const sf::IntRect full = sprite.getTextureRect();
    const float fraction = visible / m_targetSize.y;
    const int croppedHeight = std::max(1, static_cast<int>(full.size.y * fraction));
    sprite.setTextureRect(sf::IntRect(full.position, {full.size.x, croppedHeight}));

    const sf::FloatRect bounds = sprite.getLocalBounds();
    if (bounds.size.x <= 0.0f || bounds.size.y <= 0.0f) return;
    const float scale = std::min(m_targetSize.x / static_cast<float>(full.size.x),
                                 m_targetSize.y / static_cast<float>(full.size.y));

    sprite.setOrigin({bounds.size.x * 0.5f, 0.0f});
    sprite.setScale({scale, scale});
    // Pinned by its top edge: the plant grows downward out of frame as it sinks.
    sprite.setPosition({boundingBox.x + m_targetSize.x * 0.5f, boundingBox.y});
    target.draw(sprite);
}

bool PiranhaPlant::isCollidable() const {
    return active && emergedHeight() > m_targetSize.y * 0.25f;
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
