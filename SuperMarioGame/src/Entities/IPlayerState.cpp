#include "Entities/IPlayerState.hpp"
#include "Entities/Player.hpp"
#include "Utils/Constants.hpp"

// --- SmallState ---
void SmallState::enter(Player& player) {}
void SmallState::exit(Player& player) {}
void SmallState::handleInput(Player& player, const sf::Event& event) {}
void SmallState::update(Player& player, float dt) {}
sf::Vector2f SmallState::getSize() const { return sf::Vector2f{32.0f, 32.0f}; }

// --- SuperState ---
void SuperState::enter(Player& player) {}
void SuperState::exit(Player& player) {}
void SuperState::handleInput(Player& player, const sf::Event& event) {}
void SuperState::update(Player& player, float dt) {}
sf::Vector2f SuperState::getSize() const { return sf::Vector2f{32.0f, 64.0f}; }

// --- FireState ---
void FireState::enter(Player& player) {}
void FireState::exit(Player& player) {}
void FireState::handleInput(Player& player, const sf::Event& event) {}
void FireState::update(Player& player, float dt) {}
sf::Vector2f FireState::getSize() const { return sf::Vector2f{32.0f, 64.0f}; }

// --- CapeState ---
void CapeState::enter(Player& player) {}
void CapeState::exit(Player& player) {}
void CapeState::handleInput(Player& player, const sf::Event& event) {}
void CapeState::update(Player& player, float dt) {}
sf::Vector2f CapeState::getSize() const { return sf::Vector2f{32.0f, 64.0f}; }

// --- MiniState ---
void MiniState::enter(Player& player) {}
void MiniState::exit(Player& player) {}
void MiniState::handleInput(Player& player, const sf::Event& event) {}
void MiniState::update(Player& player, float dt) {}
sf::Vector2f MiniState::getSize() const { return sf::Vector2f{16.0f, 16.0f}; }

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

// --- StarDecorator ---
StarDecorator::StarDecorator(std::unique_ptr<IPlayerState> wrappedState)
    : PlayerStateDecorator(std::move(wrappedState)), m_timeLeft(Constants::STAR_DURATION) {}

void StarDecorator::enter(Player& player) {
    PlayerStateDecorator::enter(player);
}

void StarDecorator::exit(Player& player) {
    PlayerStateDecorator::exit(player);
}

void StarDecorator::update(Player& player, float dt) {
    PlayerStateDecorator::update(player, dt);
    m_timeLeft -= dt;
    if (m_timeLeft <= 0.0f) {
        player.changeState(std::move(m_wrappedState));
    }
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
    if (m_timeLeft <= 0.0f) {
        player.changeState(std::move(m_wrappedState));
    }
}

sf::Vector2f MegaDecorator::getSize() const {
    return sf::Vector2f{128.0f, 128.0f};
}
