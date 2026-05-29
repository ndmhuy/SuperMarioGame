#include <SFML/Graphics.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/Window/Event.hpp>
#include <imgui.h>
#include <imgui-SFML.h>
#include <iostream>

int main()
{
    // Create rendering window (SFML 3.0.2 uses sf::VideoMode({width, height}))
    sf::RenderWindow window(sf::VideoMode({1024, 768}), "Super Mario Game");
    window.setFramerateLimit(60);

    // Initialize ImGui-SFML
    if (!ImGui::SFML::Init(window))
    {
        std::cerr << "Failed to initialize ImGui-SFML!" << std::endl;
        return -1;
    }

    sf::Clock deltaClock;

    // A simple shape representing Mario (placeholder red circle)
    sf::CircleShape mario(30.f);
    mario.setFillColor(sf::Color::Red);
    mario.setPosition({512.f - 30.f, 384.f - 30.f}); // Center of screen

    while (window.isOpen())
    {
        // Event handling (SFML 3.0 style)
        while (const std::optional<sf::Event> event = window.pollEvent())
        {
            ImGui::SFML::ProcessEvent(window, *event);

            if (event->is<sf::Event::Closed>())
            {
                window.close();
            }
            else if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>())
            {
                if (keyPressed->code == sf::Keyboard::Key::Escape)
                {
                    window.close();
                }
            }
        }

        // Update ImGui
        ImGui::SFML::Update(window, deltaClock.restart());

        // Simple ImGui panel
        ImGui::Begin("Super Mario Game Dev Tools");
        ImGui::Text("Welcome to the Super Mario Game!");
        ImGui::Separator();
        ImGui::Text("Mario Placeholder Position:");
        float pos[2] = { mario.getPosition().x, mario.getPosition().y };
        if (ImGui::DragFloat2("Mario Position", pos))
        {
            mario.setPosition({pos[0], pos[1]});
        }
        ImGui::End();

        // Rendering
        window.clear(sf::Color(100, 149, 237)); // Cornflower Blue sky background
        
        window.draw(mario); // Draw Mario placeholder
        
        ImGui::SFML::Render(window);
        window.display();
    }

    ImGui::SFML::Shutdown();
    return 0;
}
