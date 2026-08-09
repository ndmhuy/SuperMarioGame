#include <iostream>
#include <filesystem>
#include <unordered_map>
#include <memory>
#include <cmath>
#include <SFML/Graphics.hpp>
#include <imgui.h>
#include <imgui-SFML.h>

#include "Core/ResourceManager.hpp"
#include "Graphics/SpriteSheet.hpp"
#include "Graphics/Animation.hpp"
#include "Graphics/Animator.hpp"
#include "Utils/Constants.hpp"

enum class FormState { Small, Tiny };
enum class MotionState { Idle, Wave, Walk, Jump, Fall, Run, Skid, Crouch, CrouchHold, Climb, Float, Death };

int main() {
    std::cout << "[VISUAL TEST] Launching SMB2 Player Animation Visualizer (from player/player)..." << std::endl;

    sf::RenderWindow window(sf::VideoMode(sf::Vector2u(1280, 720)), "SMB2 Player Animation Visualizer");
    window.setFramerateLimit(60);

    if (!ImGui::SFML::Init(window)) {
        std::cerr << "[VISUAL TEST] Failed to initialize ImGui-SFML!" << std::endl;
        return 1;
    }

    // Resolve resource paths for player
    ResourceManager& rm = ResourceManager::getInstance();
    std::string pngPath = "assets/spriteSheet/player/player.png";
    std::string jsonPath = "assets/spriteSheet/player/player.json";
    if (!std::filesystem::exists(pngPath)) pngPath = "../assets/spriteSheet/player/player.png";
    if (!std::filesystem::exists(pngPath)) pngPath = "../../assets/spriteSheet/player/player.png";
    if (!std::filesystem::exists(pngPath)) pngPath = "SuperMarioGame/assets/spriteSheet/player/player.png";

    if (!std::filesystem::exists(jsonPath)) jsonPath = "../assets/spriteSheet/player/player.json";
    if (!std::filesystem::exists(jsonPath)) jsonPath = "../../assets/spriteSheet/player/player.json";
    if (!std::filesystem::exists(jsonPath)) jsonPath = "SuperMarioGame/assets/spriteSheet/player/player.json";

    if (!rm.loadTexture("playerTexture", pngPath)) {
        std::cerr << "[VISUAL TEST] Failed to load player texture from " << pngPath << std::endl;
        return 1;
    }

    SpriteSheet playerSheet("playerTexture", jsonPath);
    Animator animator(&playerSheet);

    // Build Animations Dictionary
    std::unordered_map<std::string, Animation> anims;
    std::vector<std::string> characters = {"mario", "luigi", "toad", "peach"};

    for (const auto& ch : characters) {
        // --- Small Form (SMB2 Style) ---
        anims[ch + "_small_idle"] = Animation(ch + "_small_idle");
        anims[ch + "_small_idle"].frameList = {{ch + "_small_idle", 0.15f}};

        anims[ch + "_small_wave"] = Animation(ch + "_small_wave");
        anims[ch + "_small_wave"].frameList = {{ch + "_small_wave", 0.15f}};

        anims[ch + "_small_walk"] = Animation(ch + "_small_walk");
        anims[ch + "_small_walk"].frameList = {
            {ch + "_small_walk_0", 0.15f},
            {ch + "_small_walk_1", 0.15f}
        };

        anims[ch + "_small_run"] = Animation(ch + "_small_run");
        anims[ch + "_small_run"].frameList = {
            {ch + "_small_run_0", 0.10f},
            {ch + "_small_run_1", 0.10f}
        };

        anims[ch + "_small_skid"] = Animation(ch + "_small_skid");
        anims[ch + "_small_skid"].frameList = {{ch + "_small_skid", 0.15f}};

        anims[ch + "_small_crouch"] = Animation(ch + "_small_crouch");
        anims[ch + "_small_crouch"].frameList = {{ch + "_small_crouch", 0.15f}};

        anims[ch + "_small_crouch_hold"] = Animation(ch + "_small_crouch_hold");
        anims[ch + "_small_crouch_hold"].frameList = {{ch + "_small_crouch_hold", 0.15f}};

        anims[ch + "_small_climb"] = Animation(ch + "_small_climb");
        anims[ch + "_small_climb"].frameList = {
            {ch + "_small_climb_0", 0.15f},
            {ch + "_small_climb_1", 0.15f}
        };

        anims[ch + "_small_hurt"] = Animation(ch + "_small_hurt");
        anims[ch + "_small_hurt"].frameList = {{ch + "_small_hurt", 0.15f}};

        anims[ch + "_small_death"] = Animation(ch + "_small_death");
        anims[ch + "_small_death"].frameList = {{ch + "_death", 0.15f}};

        // --- Tiny Form (SMB2 Style) ---
        anims[ch + "_tiny_idle"] = Animation(ch + "_tiny_idle");
        anims[ch + "_tiny_idle"].frameList = {{ch + "_tiny_walk_0", 0.15f}}; // fallback to walk_0

        anims[ch + "_tiny_wave"] = Animation(ch + "_tiny_wave");
        anims[ch + "_tiny_wave"].frameList = {{ch + "_tiny_walk_0", 0.15f}}; // fallback to walk_0

        anims[ch + "_tiny_walk"] = Animation(ch + "_tiny_walk");
        anims[ch + "_tiny_walk"].frameList = {
            {ch + "_tiny_walk_0", 0.15f},
            {ch + "_tiny_walk_1", 0.15f}
        };

        anims[ch + "_tiny_run"] = Animation(ch + "_tiny_run");
        anims[ch + "_tiny_run"].frameList = {
            {ch + "_tiny_run_0", 0.10f},
            {ch + "_tiny_run_1", 0.10f}
        };

        anims[ch + "_tiny_skid"] = Animation(ch + "_tiny_skid");
        anims[ch + "_tiny_skid"].frameList = {{ch + "_tiny_skid", 0.15f}};

        anims[ch + "_tiny_crouch"] = Animation(ch + "_tiny_crouch");
        anims[ch + "_tiny_crouch"].frameList = {{ch + "_tiny_crouch", 0.15f}};

        anims[ch + "_tiny_crouch_hold"] = Animation(ch + "_tiny_crouch_hold");
        anims[ch + "_tiny_crouch_hold"].frameList = {{ch + "_tiny_crouch_hold", 0.15f}};

        anims[ch + "_tiny_climb"] = Animation(ch + "_tiny_climb");
        anims[ch + "_tiny_climb"].frameList = {
            {ch + "_tiny_climb_0", 0.15f},
            {ch + "_tiny_climb_1", 0.15f}
        };

        anims[ch + "_tiny_hurt"] = Animation(ch + "_tiny_hurt");
        anims[ch + "_tiny_hurt"].frameList = {{ch + "_tiny_hurt", 0.15f}};

        anims[ch + "_tiny_death"] = Animation(ch + "_tiny_death");
        anims[ch + "_tiny_death"].frameList = {{ch + "_death", 0.15f}}; // fallback to small death
    }

    // Interactive State Variables
    std::string selectedChar = "mario";
    FormState formState = FormState::Small;
    MotionState motionState = MotionState::Idle;

    sf::Vector2f marioPos(640.f, 500.f);
    sf::Vector2f marioVel(0.f, 0.f);

    bool facingRight = true;
    bool onGround = true;
    bool manualControl = true;
    bool showBBox = true;
    float renderScale = 3.0f; // Scale up 3x for clear viewing of 16-pixel art

    const float groundY = 500.f;
    const float gravity = 1200.f;
    const float walkSpeed = 200.f;
    const float runSpeed = 380.f;
    const float jumpVelocity = -520.f;

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

        float dt = deltaClock.restart().asSeconds();
        ImGui::SFML::Update(window, sf::seconds(dt));

        // Physics & Movement Simulation
        if (manualControl) {
            bool moveLeft = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left);
            bool moveRight = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right);
            bool isRunning = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RShift);
            bool isCrouching = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down);
            bool isJumping = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up);

            float maxSpeed = isRunning ? runSpeed : walkSpeed;

            // Horizontal Movement
            if (moveLeft && !moveRight) {
                facingRight = false;
                marioVel.x = -maxSpeed;
            } else if (moveRight && !moveLeft) {
                facingRight = true;
                marioVel.x = maxSpeed;
            } else {
                marioVel.x = 0.f;
            }

            // Jump
            if (isJumping && onGround) {
                marioVel.y = jumpVelocity;
                onGround = false;
            }

            // Apply Gravity
            if (!onGround) {
                marioVel.y += gravity * dt;
            }

            // Position Integration
            marioPos += marioVel * dt;

            // Screen & Ground Boundaries
            if (marioPos.x < 32.f) marioPos.x = 32.f;
            if (marioPos.x > 1248.f) marioPos.x = 1248.f;

            if (marioPos.y >= groundY) {
                marioPos.y = groundY;
                marioVel.y = 0.f;
                onGround = true;
            }

            // Auto-Determine Motion State
            if (!onGround) {
                // Peach float behavior: check float on descent (after completing ascent)
                if (selectedChar == "peach" && formState == FormState::Tiny && 
                    sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space) && marioVel.y >= 0.f) {
                    motionState = MotionState::Float;
                    marioVel.y = 20.f; // hover slowly
                } else {
                    if (marioVel.y < 0.f) motionState = MotionState::Jump;
                    else motionState = MotionState::Fall;
                }
            } else if (isCrouching) {
                motionState = MotionState::Crouch;
            } else if (std::abs(marioVel.x) > 0.f) {
                if (isRunning) motionState = MotionState::Run;
                else motionState = MotionState::Walk;
            } else {
                motionState = MotionState::Idle;
            }
        }

        // Determine Animation Key Name
        std::string formStr = (formState == FormState::Small) ? "_small_" : "_tiny_";
        std::string animKey = selectedChar + formStr + "idle";

        switch (motionState) {
            case MotionState::Idle: animKey = selectedChar + formStr + "idle"; break;
            case MotionState::Wave: animKey = selectedChar + formStr + "wave"; break;
            case MotionState::Walk: animKey = selectedChar + formStr + "walk"; break;
            case MotionState::Run: animKey = selectedChar + formStr + "run"; break;
            case MotionState::Skid: animKey = selectedChar + formStr + "skid"; break;
            case MotionState::Crouch: animKey = selectedChar + formStr + "crouch"; break;
            case MotionState::CrouchHold: animKey = selectedChar + formStr + "crouch_hold"; break;
            case MotionState::Climb: animKey = selectedChar + formStr + "climb"; break;
            // Jump and fall map to walk per user request (uses same sprites as walk)
            case MotionState::Jump: animKey = selectedChar + formStr + "walk"; break;
            case MotionState::Fall: animKey = selectedChar + formStr + "walk"; break;
            // Float maps to walk
            case MotionState::Float: animKey = selectedChar + formStr + "walk"; break;
            case MotionState::Death: animKey = selectedChar + "_death"; break;
        }

        // Fallback for missing animation keys
        if (anims.find(animKey) == anims.end()) {
            animKey = selectedChar + formStr + "idle";
        }

        if (anims.find(animKey) != anims.end()) {
            animator.play(&anims[animKey]);
        }
        animator.update(dt);

        // ImGui Controls Panel
        ImGui::Begin("SMB2 Player Animation Tester");
        ImGui::Text("Use A/D (or Left/Right) to Walk/Run");
        ImGui::Text("Hold Shift to Run | Press Space to Jump | Down to Crouch");
        if (selectedChar == "peach" && formState == FormState::Tiny) {
            ImGui::TextColored(sf::Color::Yellow, "Hold Space in mid-air (on descent) to FLOAT!");
        }

        ImGui::Separator();
        ImGui::Text("Select Character:");
        if (ImGui::RadioButton("Mario", selectedChar == "mario")) selectedChar = "mario"; ImGui::SameLine();
        if (ImGui::RadioButton("Luigi", selectedChar == "luigi")) selectedChar = "luigi"; ImGui::SameLine();
        if (ImGui::RadioButton("Toad", selectedChar == "toad")) selectedChar = "toad"; ImGui::SameLine();
        if (ImGui::RadioButton("Peach", selectedChar == "peach")) selectedChar = "peach";

        ImGui::Separator();
        ImGui::Text("Character Form:");
        int currentForm = static_cast<int>(formState);
        if (ImGui::RadioButton("Small Form", &currentForm, 0)) formState = FormState::Small; ImGui::SameLine();
        if (ImGui::RadioButton("Tiny Form (Mini)", &currentForm, 1)) formState = FormState::Tiny;

        ImGui::Separator();
        ImGui::Checkbox("Enable Interactive Keyboard Physics", &manualControl);
        ImGui::Checkbox("Show Bounding Box Overlay", &showBBox);
        ImGui::SliderFloat("Render Scale", &renderScale, 1.0f, 6.0f, "%.1fx");

        if (!manualControl) {
            ImGui::Separator();
            ImGui::Text("Manual Motion State Preview:");
            int currentMotion = static_cast<int>(motionState);
            if (ImGui::RadioButton("Idle", &currentMotion, 0)) motionState = MotionState::Idle; ImGui::SameLine();
            if (ImGui::RadioButton("Wave", &currentMotion, 1)) motionState = MotionState::Wave; ImGui::SameLine();
            if (ImGui::RadioButton("Walk", &currentMotion, 2)) motionState = MotionState::Walk; ImGui::SameLine();
            if (ImGui::RadioButton("Jump (Walk fallback)", &currentMotion, 3)) motionState = MotionState::Jump;

            if (ImGui::RadioButton("Fall (Walk fallback)", &currentMotion, 4)) motionState = MotionState::Fall; ImGui::SameLine();
            if (ImGui::RadioButton("Run", &currentMotion, 5)) motionState = MotionState::Run; ImGui::SameLine();
            if (ImGui::RadioButton("Skid", &currentMotion, 6)) motionState = MotionState::Skid; ImGui::SameLine();
            if (ImGui::RadioButton("Crouch", &currentMotion, 7)) motionState = MotionState::Crouch;

            if (ImGui::RadioButton("Crouch Hold", &currentMotion, 8)) motionState = MotionState::CrouchHold; ImGui::SameLine();
            if (ImGui::RadioButton("Climb", &currentMotion, 9)) motionState = MotionState::Climb; ImGui::SameLine();
            if (ImGui::RadioButton("Float (Walk fallback)", &currentMotion, 10)) motionState = MotionState::Float; ImGui::SameLine();
            if (ImGui::RadioButton("Death", &currentMotion, 11)) motionState = MotionState::Death;

            ImGui::Checkbox("Facing Right", &facingRight);
        }

        ImGui::Separator();
        ImGui::Text("Active Animation Key: %s", animKey.c_str());
        ImGui::Text("Position: (%.1f, %.1f) | Velocity: (%.1f, %.1f)", marioPos.x, marioPos.y, marioVel.x, marioVel.y);
        ImGui::Text("OnGround: %s | Facing: %s", onGround ? "TRUE" : "FALSE", facingRight ? "RIGHT" : "LEFT");

        ImGui::End();

        // Window Clear
        window.clear(sf::Color(40, 50, 60));

        // Draw Ground Floor Line
        sf::RectangleShape groundLine(sf::Vector2f(1280.f, 4.f));
        groundLine.setPosition(sf::Vector2f(0.f, groundY + 16.f * renderScale));
        groundLine.setFillColor(sf::Color(80, 180, 80));
        window.draw(groundLine);

        // Fetch & Configure Player Sprite
        sf::Sprite sprite = animator.getSprite();
        sf::FloatRect bounds = sprite.getLocalBounds();
        sprite.setOrigin(sf::Vector2f(bounds.size.x * 0.5f, bounds.size.y));

        // Sprites face LEFT by default in player.png. Flip them when facing right.
        float scaleX = facingRight ? -renderScale : renderScale;
        sprite.setScale(sf::Vector2f(scaleX, renderScale));
        sprite.setPosition(marioPos);

        window.draw(sprite);

        // Bounding Box Overlay
        if (showBBox) {
            sf::RectangleShape bbox(sf::Vector2f(bounds.size.x * renderScale, bounds.size.y * renderScale));
            bbox.setOrigin(sf::Vector2f(bounds.size.x * renderScale * 0.5f, bounds.size.y * renderScale));
            bbox.setPosition(marioPos);
            bbox.setFillColor(sf::Color::Transparent);
            bbox.setOutlineColor(sf::Color::Yellow);
            bbox.setOutlineThickness(1.5f);
            window.draw(bbox);
        }

        // Render ImGui
        ImGui::SFML::Render(window);

        window.display();
    }

    ImGui::SFML::Shutdown();
    return 0;
}
