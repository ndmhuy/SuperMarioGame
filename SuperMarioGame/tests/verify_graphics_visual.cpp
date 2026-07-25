#include <iostream>
#include <filesystem>
#include <vector>
#include <string>
#include <SFML/Graphics.hpp>
#include <imgui.h>
#include <imgui-SFML.h>
#include "Graphics/SpriteSheet.hpp"
#include "Graphics/Animation.hpp"
#include "Graphics/Animator.hpp"
#include "Core/ResourceManager.hpp"

int main() {
    std::cout << "[VISUAL TEST] Launching Graphics & Animation Visualizer..." << std::endl;

    // Create SFML RenderWindow (SFML 3 style)
    sf::RenderWindow window(sf::VideoMode(sf::Vector2u(800, 600)), "Sprite Sheet & Animation Visualizer");
    window.setFramerateLimit(60);

    // Initialize ImGui-SFML
    if (!ImGui::SFML::Init(window)) {
        std::cerr << "[VISUAL TEST] Failed to initialize ImGui-SFML!" << std::endl;
        return 1;
    }

    // Resolve asset path
    std::string assetPath = "assets/spriteSheet/test";
    if (!std::filesystem::exists(assetPath + "/test.json")) {
        assetPath = "../assets/spriteSheet/test";
    }
    if (!std::filesystem::exists(assetPath + "/test.json")) {
        assetPath = "../../assets/spriteSheet/test";
    }
    if (!std::filesystem::exists(assetPath + "/test.json")) {
        assetPath = "SuperMarioGame/assets/spriteSheet/test";
    }
    if (!std::filesystem::exists(assetPath + "/test.json")) {
        assetPath = "../SuperMarioGame/assets/spriteSheet/test";
    }

    std::cout << "[VISUAL TEST] Resolved asset path: " << assetPath << std::endl;

    // Initialize SpriteSheet
    SpriteSheet sheet(assetPath);

    // Setup Animations
    Animation walkAnim("Walk");
    walkAnim.isLooping = true;
    walkAnim.frameList = {
        {"frame_00", 0.15f},
        {"frame_01", 0.15f},
        {"frame_02", 0.15f},
        {"frame_03", 0.15f},
        {"frame_04", 0.15f},
        {"frame_05", 0.15f}
    };

    Animation dieAnim("Die");
    dieAnim.isLooping = false;
    dieAnim.frameList = {
        {"frame_06", 0.2f},
        {"frame_07", 0.2f},
        {"frame_08", 0.4f}
    };

    // Setup Animator
    Animator animator(&sheet);
    animator.play(&walkAnim);

    const Animation* currentAnim = &walkAnim;
    float speedMultiplier = 1.0f;
    bool isPlaying = true;
    int staticFrameIndex = 0;
    bool showStaticFrame = false;

    std::vector<std::string> allFrames = {
        "frame_00", "frame_01", "frame_02",
        "frame_03", "frame_04", "frame_05",
        "frame_06", "frame_07", "frame_08"
    };

    sf::Clock deltaClock;

    sf::Sprite renderSprite(ResourceManager::getInstance().getTexture(""));

    // Game loop
    while (window.isOpen()) {
        sf::Time dt = deltaClock.restart();

        // 1. Process Events
        while (const std::optional<sf::Event> event = window.pollEvent()) {
            ImGui::SFML::ProcessEvent(window, *event);

            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
            else if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
                if (keyPressed->code == sf::Keyboard::Key::Escape) {
                    window.close();
                }
                else if (keyPressed->code == sf::Keyboard::Key::Num1) {
                    animator.play(&walkAnim);
                    currentAnim = &walkAnim;
                    showStaticFrame = false;
                    isPlaying = true;
                }
                else if (keyPressed->code == sf::Keyboard::Key::Num2) {
                    animator.play(&dieAnim);
                    currentAnim = &dieAnim;
                    showStaticFrame = false;
                    isPlaying = true;
                }
                else if (keyPressed->code == sf::Keyboard::Key::Space) {
                    isPlaying = !isPlaying;
                }
            }
        }

        // 2. Update Animation & ImGui
        if (isPlaying && !showStaticFrame) {
            animator.update(dt.asSeconds() * speedMultiplier);
        }

        ImGui::SFML::Update(window, dt);

        // ImGui Controls Panel
        ImGui::Begin("Animation Controls");
        
        ImGui::Text("Controls:");
        ImGui::BulletText("Press [1] or click 'Walk' for Looping Walking Anim");
        ImGui::BulletText("Press [2] or click 'Die' for Non-Looping Death Anim");
        ImGui::BulletText("Press [Space] to pause/resume playback");
        ImGui::Separator();

        // Animation info
        if (showStaticFrame) {
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "State: Showing Static Frame");
            ImGui::Text("Active Frame: %s", allFrames[staticFrameIndex].c_str());
        } else {
            ImGui::Text("Active Animation: %s", currentAnim->name.c_str());
            ImGui::Text("Looping: %s", currentAnim->isLooping ? "Yes" : "No");
            ImGui::Text("Completed: %s", animator.isDone() ? "Yes" : "No");
        }
        ImGui::Separator();

        // Controls
        if (ImGui::Button("Play Walk Animation")) {
            animator.play(&walkAnim);
            currentAnim = &walkAnim;
            showStaticFrame = false;
            isPlaying = true;
        }
        if (ImGui::Button("Play Die Animation")) {
            animator.play(&dieAnim);
            currentAnim = &dieAnim;
            showStaticFrame = false;
            isPlaying = true;
        }
        ImGui::Separator();

        ImGui::Checkbox("Play/Pause", &isPlaying);
        ImGui::SliderFloat("Speed Multiplier", &speedMultiplier, 0.1f, 3.0f);
        
        ImGui::Separator();
        ImGui::Text("Static Frame Browser:");
        if (ImGui::Checkbox("Enable Static Frame View", &showStaticFrame)) {
            if (showStaticFrame) isPlaying = false;
        }

        if (showStaticFrame) {
            if (ImGui::BeginCombo("Select Frame", allFrames[staticFrameIndex].c_str())) {
                for (size_t i = 0; i < allFrames.size(); ++i) {
                    bool isSelected = (staticFrameIndex == static_cast<int>(i));
                    if (ImGui::Selectable(allFrames[i].c_str(), isSelected)) {
                        staticFrameIndex = static_cast<int>(i);
                    }
                    if (isSelected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
        }

        ImGui::End();

        if (showStaticFrame) {
            renderSprite = sheet.getSprite(allFrames[staticFrameIndex]);
        } else {
            renderSprite = animator.getSprite();
        }

        // Center and scale the sprite
        renderSprite.setScale(sf::Vector2f(8.f, 8.f));
        // Center the sprite in the remaining window space
        sf::FloatRect bounds = renderSprite.getLocalBounds();
        renderSprite.setOrigin(sf::Vector2f(bounds.size.x / 2.f, bounds.size.y / 2.f));
        renderSprite.setPosition(sf::Vector2f(400.f, 300.f));

        // 4. Render
        window.clear(sf::Color(40, 44, 52)); // Dark grey background for contrast
        window.draw(renderSprite);
        ImGui::SFML::Render(window);
        window.display();
    }

    ImGui::SFML::Shutdown();
    return 0;
}
