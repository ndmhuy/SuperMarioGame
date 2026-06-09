#pragma once

#include <SFML/Window/Event.hpp>
#include <SFML/Graphics/RenderTarget.hpp>

class IGameState {
public:
    virtual ~IGameState() = default;

    virtual void enter() = 0;
    virtual void exit() = 0;
    virtual void handleInput(const sf::Event& event) = 0;
    virtual void update(float dt) = 0;
    virtual void render(sf::RenderTarget& target) = 0;
};
