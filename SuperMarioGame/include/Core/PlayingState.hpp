#pragma once

#include "Core/IGameState.hpp"

class PlayingState : public IGameState {
public:
    PlayingState() = default;
    ~PlayingState() override = default;

    void enter() override;
    void exit() override;
    void handleInput(const sf::Event& event) override;
    void update(float dt) override;
    void render(sf::RenderTarget& target) override;
};
