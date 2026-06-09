#pragma once

#include "Core/IGameState.hpp"

class MenuState : public IGameState {
public:
    MenuState() = default;
    ~MenuState() override = default;

    void enter() override;
    void exit() override;
    void handleInput(const sf::Event& event) override;
    void update(float dt) override;
    void render(sf::RenderTarget& target) override;
};
