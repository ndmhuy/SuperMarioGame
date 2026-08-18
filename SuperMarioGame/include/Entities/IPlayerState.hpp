#pragma once

#include <SFML/Window/Event.hpp>
#include <SFML/System/Vector2.hpp>
#include <memory>

class Player;

class IPlayerState {
public:
    virtual ~IPlayerState() = default;

    virtual void enter(Player& player) = 0;
    virtual void exit(Player& player) = 0;
    virtual void handleInput(Player& player, const sf::Event& event) = 0;
    virtual void update(Player& player, float dt) = 0;
    virtual sf::Vector2f getSize() const = 0;

    // A timed state reports expiry here instead of swapping itself out from inside
    // update(). Player::update performs the swap once update() has returned.
    //
    // Doing it the other way round is a use-after-free: changeState() assigns over
    // Player::m_currentState, destroying the very object whose update() is still on
    // the stack (audit A-7).
    virtual bool isExpired() const { return false; }
};

// --- Concrete Player States ---

class SmallState : public IPlayerState {
public:
    void enter(Player& player) override;
    void exit(Player& player) override;
    void handleInput(Player& player, const sf::Event& event) override;
    void update(Player& player, float dt) override;
    sf::Vector2f getSize() const override;
};

class SuperState : public IPlayerState {
public:
    void enter(Player& player) override;
    void exit(Player& player) override;
    void handleInput(Player& player, const sf::Event& event) override;
    void update(Player& player, float dt) override;
    sf::Vector2f getSize() const override;
};

class FireState : public IPlayerState {
public:
    void enter(Player& player) override;
    void exit(Player& player) override;
    void handleInput(Player& player, const sf::Event& event) override;
    void update(Player& player, float dt) override;
    sf::Vector2f getSize() const override;
};

class CapeState : public IPlayerState {
public:
    void enter(Player& player) override;
    void exit(Player& player) override;
    void handleInput(Player& player, const sf::Event& event) override;
    void update(Player& player, float dt) override;
    sf::Vector2f getSize() const override;
};

class MiniState : public IPlayerState {
public:
    void enter(Player& player) override;
    void exit(Player& player) override;
    void handleInput(Player& player, const sf::Event& event) override;
    void update(Player& player, float dt) override;
    sf::Vector2f getSize() const override;
};

// --- Decorators for Temporary Forms ---

class PlayerStateDecorator : public IPlayerState {
public:
    explicit PlayerStateDecorator(std::unique_ptr<IPlayerState> wrappedState);
    ~PlayerStateDecorator() override = default;

    void enter(Player& player) override;
    void exit(Player& player) override;
    void handleInput(Player& player, const sf::Event& event) override;
    void update(Player& player, float dt) override;
    sf::Vector2f getSize() const override;

    IPlayerState* getWrappedState() const { return m_wrappedState.get(); }

    // Hands ownership of the inner state to the caller. Used by Player when a
    // decorator expires, and by Player::setBaseState when swapping the base form
    // underneath an active decorator.
    std::unique_ptr<IPlayerState> releaseWrappedState() { return std::move(m_wrappedState); }
    void setWrappedState(std::unique_ptr<IPlayerState> state) { m_wrappedState = std::move(state); }

protected:
    std::unique_ptr<IPlayerState> m_wrappedState;
};

class StarDecorator : public PlayerStateDecorator {
public:
    explicit StarDecorator(std::unique_ptr<IPlayerState> wrappedState);
    void enter(Player& player) override;
    void exit(Player& player) override;
    void update(Player& player, float dt) override;
    bool isExpired() const override { return m_timeLeft <= 0.0f; }

    float getTimeLeft() const { return m_timeLeft; }

private:
    float m_timeLeft;
};

class MegaDecorator : public PlayerStateDecorator {
public:
    explicit MegaDecorator(std::unique_ptr<IPlayerState> wrappedState);
    void enter(Player& player) override;
    void exit(Player& player) override;
    void update(Player& player, float dt) override;
    sf::Vector2f getSize() const override;
    bool isExpired() const override { return m_timeLeft <= 0.0f; }

    float getTimeLeft() const { return m_timeLeft; }

private:
    float m_timeLeft;
};
