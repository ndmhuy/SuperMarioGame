#include "Entities/IPlayerState.hpp"
#include "Entities/Player.hpp"
#include "Utils/Constants.hpp"
#include "Core/InputManager.hpp"

// --- SmallState ---
void SmallState::enter(Player& player) {}
void SmallState::exit(Player& player) {}
void SmallState::handleInput(Player& player, const sf::Event& event) {}
void SmallState::update(Player& player, float dt) {}
sf::Vector2f SmallState::getSize() const { return sf::Vector2f{24.0f, 30.0f}; }

// --- SuperState ---
void SuperState::enter(Player& player) {}
void SuperState::exit(Player& player) {}
void SuperState::handleInput(Player& player, const sf::Event& event) {}
void SuperState::update(Player& player, float dt) {}
sf::Vector2f SuperState::getSize() const { return sf::Vector2f{24.0f, 60.0f}; }

// --- FireState ---
void FireState::enter(Player& player) {}
void FireState::exit(Player& player) {}
void FireState::handleInput(Player& player, const sf::Event& event) {}
void FireState::update(Player& player, float dt) {}
sf::Vector2f FireState::getSize() const { return sf::Vector2f{24.0f, 60.0f}; }

// --- CapeState ---
void CapeState::enter(Player& player) {}

void CapeState::exit(Player& player) {
    // Losing the cape mid-glide must not leave the flag set: the animation and
    // the HUD both read it.
    player.setGliding(false);
}

void CapeState::handleInput(Player& player, const sf::Event& event) {}

void CapeState::update(Player& player, float dt) {
    // Glide, not hover: the player still descends, just slowly, and only while
    // already falling. Holding jump on the way up does nothing, so a cape jump
    // is a normal jump that turns into a drift at the apex.
    const bool falling = !player.isOnGround() && player.getVelocity().y > 0.0f;
    const bool jumpHeld =
        InputManager::getInstance().isActionHeld("jump", player.getPlayerIndex());

    if (falling && jumpHeld) {
        player.setGliding(true);
        if (player.getVelocity().y > GLIDE_FALL_SPEED) {
            player.setVelocity({player.getVelocity().x, GLIDE_FALL_SPEED});
        }
    } else {
        player.setGliding(false);
    }
}

sf::Vector2f CapeState::getSize() const { return sf::Vector2f{24.0f, 60.0f}; }

// --- MiniState ---
void MiniState::enter(Player& player) {}
void MiniState::exit(Player& player) {}
void MiniState::handleInput(Player& player, const sf::Event& event) {}
void MiniState::update(Player& player, float dt) {}
sf::Vector2f MiniState::getSize() const { return sf::Vector2f{14.0f, 14.0f}; }


// --- PlayerStateDecorator ---
PlayerStateDecorator::PlayerStateDecorator(std::unique_ptr<IPlayerState> wrappedState)
    : m_wrappedState(std::move(wrappedState)) {}

void PlayerStateDecorator::enter(Player& player) {
    if (m_wrappedState) m_wrappedState->enter(player);
}

void PlayerStateDecorator::exit(Player& player) {
    if (m_wrappedState) m_wrappedState->exit(player);
}

void PlayerStateDecorator::handleInput(Player& player, const sf::Event& event) {
    if (m_wrappedState) m_wrappedState->handleInput(player, event);
}

void PlayerStateDecorator::update(Player& player, float dt) {
    if (m_wrappedState) m_wrappedState->update(player, dt);
}

sf::Vector2f PlayerStateDecorator::getSize() const {
    if (m_wrappedState) return m_wrappedState->getSize();
    return sf::Vector2f{32.0f, 32.0f};
}

#include "Core/SoundManager.hpp"

// --- StarDecorator ---
StarDecorator::StarDecorator(std::unique_ptr<IPlayerState> wrappedState)
    : PlayerStateDecorator(std::move(wrappedState)), m_timeLeft(Constants::STAR_DURATION) {}

void StarDecorator::enter(Player& player) {
    PlayerStateDecorator::enter(player);
    SoundManager::getInstance().playStarMusic();
}

void StarDecorator::exit(Player& player) {
    PlayerStateDecorator::exit(player);
    SoundManager::getInstance().restoreLevelBGM();
}

void StarDecorator::update(Player& player, float dt) {
    PlayerStateDecorator::update(player, dt);
    m_timeLeft -= dt;
    // Deliberately does NOT call player.changeState() here — that would free this
    // object mid-update. Player::update sees isExpired() and unwraps us instead.
}

// --- MegaDecorator ---
MegaDecorator::MegaDecorator(std::unique_ptr<IPlayerState> wrappedState)
    : PlayerStateDecorator(std::move(wrappedState)), m_timeLeft(8.0f) {}

void MegaDecorator::enter(Player& player) {
    PlayerStateDecorator::enter(player);
}

void MegaDecorator::exit(Player& player) {
    PlayerStateDecorator::exit(player);
}

void MegaDecorator::update(Player& player, float dt) {
    PlayerStateDecorator::update(player, dt);
    m_timeLeft -= dt;
    // See StarDecorator::update — expiry is reported, not acted on.
}

sf::Vector2f MegaDecorator::getSize() const {
    // SPEC 6.x gives Mega as "4 tiles (128px)" — a height, like every other row
    // of that table. This returned a 128x128 *square*, and nothing the player
    // can be is square: every form is 24 wide. drawSprite aspect-fits, so the
    // sprite came out ~39px wide inside a 128px box and the player collided
    // with things 45px to either side of where they appeared. Swapping the base
    // form underneath Mega — a Fire Flower while giant — made that obvious as a
    // stretched "big fire" figure adrift in its own hitbox.
    //
    // Scaling the wrapped form to that height instead keeps the box on the
    // sprite whatever Mega is wrapping.
    constexpr float MEGA_HEIGHT = 128.0f;
    const sf::Vector2f base = PlayerStateDecorator::getSize();
    if (base.y <= 0.0f) {
        return sf::Vector2f{MEGA_HEIGHT * 0.4f, MEGA_HEIGHT};
    }
    return sf::Vector2f{base.x * (MEGA_HEIGHT / base.y), MEGA_HEIGHT};
}
