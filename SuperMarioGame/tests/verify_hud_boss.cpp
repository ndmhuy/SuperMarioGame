#include <iostream>
#include <filesystem>
#include <algorithm>
#include <SFML/Graphics.hpp>
#include <imgui.h>
#include <imgui-SFML.h>
#include "Graphics/Hud.hpp"
#include "Core/ResourceManager.hpp"

int main() {
    std::cout << "[VISUAL TEST] Launching Boss HUD Visualizer..." << std::endl;

    // Create SFML RenderWindow (1280x720 design size)
    sf::RenderWindow window(sf::VideoMode(sf::Vector2u(1280, 720)), "Super Mario Boss HUD Visualizer");
    window.setFramerateLimit(60);

    // Initialize ImGui-SFML
    if (!ImGui::SFML::Init(window)) {
        std::cerr << "[VISUAL TEST] Failed to initialize ImGui-SFML!" << std::endl;
        return 1;
    }

    // Resolve font path
    ResourceManager& rm = ResourceManager::getInstance();
    std::string fontPath = "asset/font/PressStart2P.ttf";
    if (!std::filesystem::exists(fontPath)) fontPath = "../asset/font/PressStart2P.ttf";
    if (!std::filesystem::exists(fontPath)) fontPath = "../../asset/font/PressStart2P.ttf";
    if (!std::filesystem::exists(fontPath)) fontPath = "SuperMarioGame/asset/font/PressStart2P.ttf";
    if (!std::filesystem::exists(fontPath)) fontPath = "../SuperMarioGame/asset/font/PressStart2P.ttf";

    std::cout << "[VISUAL TEST] Resolved font path: " << fontPath << std::endl;

    if (!rm.loadFont("PressStart2P", fontPath)) {
        std::cout << "[VISUAL TEST] Local font file not found, falling back to system consola.ttf..." << std::endl;
        if (!rm.loadFont("PressStart2P", "C:/Windows/Fonts/consola.ttf")) {
            rm.loadFont("PressStart2P", "C:/Windows/Fonts/arial.ttf");
        }
    }

    // Instantiate HUD
    Hud hud(sf::Vector2i(1280, 720));

    // Setup initial HudData
    HudData data;
    data.score = 102520;
    data.coins = 57;
    data.lives = 9;
    data.timeLeft = 260;
    data.worldMajor = 1;
    data.worldMinor = 1;
    data.comboCount = 1;
    data.characterName = "MARIO";
    data.pSwitchActive = false;
    data.pSwitchTimer = 15.0f;
    data.starCoinsCollected = {true, true, false};

    // Boss initial data
    data.bossActive = true;
    data.bossName = "BOWSER";
    data.bossHealth = 80;
    data.bossMaxHealth = 100;

    // Buffers for text inputs
    char charNameBuf[64] = "MARIO";
    char bossNameBuf[64] = "BOWSER";

    sf::Clock deltaClock;
    while (window.isOpen()) {
        while (const auto event = window.pollEvent()) {
            ImGui::SFML::ProcessEvent(window, *event);
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
            if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
                if (keyPressed->code == sf::Keyboard::Key::Escape) {
                    window.close();
                }
            }
        }

        // Restart clock and update ImGui
        float dt = deltaClock.restart().asSeconds();
        ImGui::SFML::Update(window, sf::seconds(dt));

        // P-Switch Timer auto-decrement if active
        if (data.pSwitchActive) {
            data.pSwitchTimer = std::max(0.0f, data.pSwitchTimer - dt);
            if (data.pSwitchTimer <= 0.0f) {
                data.pSwitchActive = false;
            }
        }

        // Sync buffers to HudData
        data.characterName = charNameBuf;
        data.bossName = bossNameBuf;

        // Sync Hud
        hud.sync(data);

        // ImGui Controls Panel
        ImGui::Begin("HUD & Boss Editor");
        ImGui::Text("Edit HUD Elements:");
        ImGui::InputInt("Score", &data.score);
        ImGui::SliderInt("Coins", &data.coins, 0, 99);
        ImGui::SliderInt("Lives", &data.lives, 0, 99);
        ImGui::SliderInt("Time Left", &data.timeLeft, 0, 999);
        ImGui::InputInt("World Major", &data.worldMajor);
        ImGui::InputInt("World Minor", &data.worldMinor);
        ImGui::InputText("Character Name", charNameBuf, sizeof(charNameBuf));

        ImGui::Separator();
        ImGui::Text("Level Events:");
        ImGui::Checkbox("P-Switch Active", &data.pSwitchActive);
        if (data.pSwitchActive) {
            ImGui::SliderFloat("P-Switch Timer", &data.pSwitchTimer, 0.0f, 15.0f);
        }
        ImGui::SliderInt("Combo Count", &data.comboCount, 1, 8);
        ImGui::Checkbox("Star Coin 1", &data.starCoinsCollected[0]);
        ImGui::Checkbox("Star Coin 2", &data.starCoinsCollected[1]);
        ImGui::Checkbox("Star Coin 3", &data.starCoinsCollected[2]);

        ImGui::Separator();
        ImGui::Text("Boss Settings:");
        ImGui::Checkbox("Boss Active", &data.bossActive);
        if (data.bossActive) {
            ImGui::InputText("Boss Name", bossNameBuf, sizeof(bossNameBuf));
            ImGui::SliderInt("Boss Health", &data.bossHealth, 0, data.bossMaxHealth);
            ImGui::InputInt("Boss Max Health", &data.bossMaxHealth);
        }

        ImGui::End();

        // Clear window to a standard Mario sky blue background
        window.clear(sf::Color(92, 148, 252));

        // Render HUD
        window.draw(hud);

        // Render ImGui
        ImGui::SFML::Render(window);

        window.display();
    }

    ImGui::SFML::Shutdown();
    return 0;
}
