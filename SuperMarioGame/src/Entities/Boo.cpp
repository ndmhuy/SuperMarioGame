#include "Entities/Boo.hpp"
#include "Entities/ChaseStrategy.hpp"
#include "Utils/Constants.hpp"

Boo::Boo(sf::Vector2f position)
    : Enemy(position, 0) { // Boo gives 0 points
    speed = Constants::BOO_SPEED;
    boundingBox = AABB{ position.x, position.y, Constants::TILE_SIZE, Constants::TILE_SIZE };
    
    setStrategy(std::make_unique<ChaseStrategy>());
}

void Boo::update(float dt) {
    Enemy::update(dt);
    boundingBox.x = position.x;
    boundingBox.y = position.y;
}

void Boo::render(sf::RenderTarget& target) {
    // Visual rendering will be implemented in Phase 5
}

void Boo::onStomped() {
    // Boo is invulnerable to stomp
}

void Boo::onHitByFireball() {
    // Boo is invulnerable to fireballs
}
