#pragma once

#include "Core/IGameState.hpp"
#include "Utils/MapGenerator.hpp"

class MenuState : public IGameState {
public:
    MenuState() = default;
    ~MenuState() override = default;

    void enter() override;
    void exit() override;
    void handleInput(const sf::Event& event) override;
    void update(float dt) override;
    void render(sf::RenderTarget& target) override;

private:
    MapGeneratorConfig m_generatorConfig;
    int m_selectedThemeIdx = 0;
    int m_selectedDifficultyIdx = 0;
    bool m_showGeneratorPanel = false;
};

