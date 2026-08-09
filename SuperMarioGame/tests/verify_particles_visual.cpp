#include <iostream>
#include <SFML/Graphics.hpp>
#include <imgui.h>
#include <imgui-SFML.h>
#include "Graphics/ParticleSystem.hpp"
#include "Graphics/ParticleEmitter.hpp"

int main() {
    std::cout << "[VISUAL TEST] Launching Particle System & Emitter Visualizer..." << std::endl;

    sf::RenderWindow window(sf::VideoMode(sf::Vector2u(1280, 720)), "Super Mario Particle System Visualizer");
    window.setFramerateLimit(60);

    if (!ImGui::SFML::Init(window)) {
        std::cerr << "[VISUAL TEST] Failed to initialize ImGui-SFML!" << std::endl;
        return 1;
    }

    ParticleSystem& ps = ParticleSystem::getInstance();
    ParticleEmitter emitter;

    sf::Vector2f spawnPos(640.f, 360.f);

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
            if (const auto* mouseMoved = event->getIf<sf::Event::MouseMoved>()) {
                if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left) && !ImGui::GetIO().WantCaptureMouse) {
                    spawnPos = window.mapPixelToCoords(mouseMoved->position);
                }
            }
            if (const auto* mousePressed = event->getIf<sf::Event::MouseButtonPressed>()) {
                if (mousePressed->button == sf::Mouse::Button::Left && !ImGui::GetIO().WantCaptureMouse) {
                    spawnPos = window.mapPixelToCoords(mousePressed->position);
                    emitter.burst(spawnPos, ParticleType::CoinSparkle);
                }
            }
        }

        float dt = deltaClock.restart().asSeconds();
        ImGui::SFML::Update(window, sf::seconds(dt));

        // Update continuous emitter & particle system
        emitter.update(dt, spawnPos);
        ps.update(dt);

        // ImGui Controls
        ImGui::Begin("Particle System Controls");
        ImGui::Text("Click anywhere in window to move emitter & trigger CoinSparkle burst!");
        ImGui::Text("Emitter Position: (%.1f, %.1f)", spawnPos.x, spawnPos.y);

        ImGui::Separator();
        ImGui::Text("Preset Bursts:");
        if (ImGui::Button("BrickBreak Burst")) emitter.burst(spawnPos, ParticleType::BrickBreak);
        ImGui::SameLine();
        if (ImGui::Button("CoinSparkle Burst")) emitter.burst(spawnPos, ParticleType::CoinSparkle);

        if (ImGui::Button("DeathPoof Burst")) emitter.burst(spawnPos, ParticleType::DeathPoof);
        ImGui::SameLine();
        if (ImGui::Button("Stomp Burst")) emitter.burst(spawnPos, ParticleType::Stomp);

        if (ImGui::Button("Combo Burst")) emitter.burst(spawnPos, ParticleType::Combo);
        ImGui::SameLine();
        if (ImGui::Button("WallDust Burst")) emitter.burst(spawnPos, ParticleType::WallDust);

        if (ImGui::Button("WaterBubble Burst")) emitter.burst(spawnPos, ParticleType::WaterBubble);
        ImGui::SameLine();
        if (ImGui::Button("LavaEmber Burst")) emitter.burst(spawnPos, ParticleType::LavaEmber);

        ImGui::Separator();
        ImGui::Text("Continuous Emitter Settings:");
        static bool continuous = false;
        if (ImGui::Checkbox("Enable Continuous Emission", &continuous)) {
            if (continuous) {
                EmitterSettings settings;
                settings.minVelocity = sf::Vector2f(-40.f, -80.f);
                settings.maxVelocity = sf::Vector2f(40.f, -20.f);
                settings.acceleration = sf::Vector2f(0.f, -10.f);
                settings.startColor = sf::Color(255, 200, 50, 255);
                settings.endColor = sf::Color(255, 50, 0, 0);
                settings.minLifetime = 0.5f;
                settings.maxLifetime = 1.0f;
                settings.emissionRate = 20.0f;
                emitter.setSetting(settings);
                emitter.setEnable(true);
            } else {
                emitter.setEnable(false);
            }
        }

        ImGui::End();

        // Clear & Render
        window.clear(sf::Color(30, 30, 40));

        // Draw crosshair at emitter position
        sf::CircleShape point(4.f);
        point.setOrigin({4.f, 4.f});
        point.setPosition(spawnPos);
        point.setFillColor(sf::Color::Cyan);
        window.draw(point);

        // Render Particle System
        window.draw(ps);

        // Render ImGui
        ImGui::SFML::Render(window);

        window.display();
    }

    ImGui::SFML::Shutdown();
    return 0;
}
