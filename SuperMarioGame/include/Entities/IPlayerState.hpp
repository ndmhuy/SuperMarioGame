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

protected:
    std::unique_ptr<IPlayerState> m_wrappedState;
};

class StarDecorator : public PlayerStateDecorator {
public:
    using PlayerStateDecorator::PlayerStateDecorator;
    void enter(Player& player) override;
    void exit(Player& player) override;
    void update(Player& player, float dt) override;
};

class MegaDecorator : public PlayerStateDecorator {
public:
    using PlayerStateDecorator::PlayerStateDecorator;
    void enter(Player& player) override;
    void exit(Player& player) override;
    void update(Player& player, float dt) override;
    sf::Vector2f getSize() const override;
};
