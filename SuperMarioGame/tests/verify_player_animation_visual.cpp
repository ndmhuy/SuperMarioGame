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

enum class FormState { Small, Super, Fire, Cape, Mini, Mega };
enum class MotionState { Idle, Walk, Run, Jump, Fall, Crouch, Skid, Slide, WallSlide, Glide, Spin, Damaged, Death };

int main() {
    std::cout << "[VISUAL TEST] Launching Full Player Animation Visualizer (Small, Super, Fire, Cape, Mini, Mega)..." << std::endl;

    sf::RenderWindow window(sf::VideoMode(sf::Vector2u(1280, 720)), "Super Mario Full Player Animation Visualizer");
    window.setFramerateLimit(60);

    if (!ImGui::SFML::Init(window)) {
        std::cerr << "[VISUAL TEST] Failed to initialize ImGui-SFML!" << std::endl;
        return 1;
    }

    // Resolve resource paths
    ResourceManager& rm = ResourceManager::getInstance();
    std::string pngPath = "assets/spriteSheet/player/players.png";
    std::string jsonPath = "assets/spriteSheet/player/players.json";
    if (!std::filesystem::exists(pngPath)) pngPath = "../assets/spriteSheet/player/players.png";
    if (!std::filesystem::exists(pngPath)) pngPath = "../../assets/spriteSheet/player/players.png";
    if (!std::filesystem::exists(pngPath)) pngPath = "SuperMarioGame/assets/spriteSheet/player/players.png";

    if (!std::filesystem::exists(jsonPath)) jsonPath = "../assets/spriteSheet/player/players.json";
    if (!std::filesystem::exists(jsonPath)) jsonPath = "../../assets/spriteSheet/player/players.json";
    if (!std::filesystem::exists(jsonPath)) jsonPath = "SuperMarioGame/assets/spriteSheet/player/players.json";

    if (!rm.loadTexture("playerTexture", pngPath)) {
        std::cerr << "[VISUAL TEST] Failed to load player texture from " << pngPath << std::endl;
        return 1;
    }

    SpriteSheet playerSheet("playerTexture", jsonPath);
    Animator animator(&playerSheet);

    // Build Animations Dictionary
    std::unordered_map<std::string, Animation> anims;

    // --- 1. Small Mario (32x32) ---
    anims["small_idle"] = Animation("small_idle");
    anims["small_idle"].frameList = {{"mario_small_idle", 0.15f}};

    anims["small_walk"] = Animation("small_walk");
    anims["small_walk"].frameList = {
        {"mario_small_walk_0", 0.12f},
        {"mario_small_walk_1", 0.12f},
        {"mario_small_walk_2", 0.12f}
    };

    anims["small_run"] = Animation("small_run");
    anims["small_run"].frameList = {
        {"mario_small_run_0", 0.08f},
        {"mario_small_run_1", 0.08f},
        {"mario_small_run_2", 0.08f}
    };

    anims["small_jump"] = Animation("small_jump");
    anims["small_jump"].frameList = {{"mario_small_jump", 0.15f}};

    anims["small_fall"] = Animation("small_fall");
    anims["small_fall"].frameList = {{"mario_small_fall", 0.15f}};

    anims["small_crouch"] = Animation("small_crouch");
    anims["small_crouch"].frameList = {{"mario_small_crouch", 0.15f}};

    anims["small_skid"] = Animation("small_skid");
    anims["small_skid"].frameList = {{"mario_small_skid", 0.15f}};

    anims["small_slide"] = Animation("small_slide");
    anims["small_slide"].frameList = {{"mario_small_slide", 0.15f}};

    anims["small_wall_slide"] = Animation("small_wall_slide");
    anims["small_wall_slide"].frameList = {{"mario_small_wall_slide", 0.15f}};

    anims["small_damaged"] = Animation("small_damaged");
    anims["small_damaged"].frameList = {{"mario_small_damaged", 0.15f}};

    anims["small_death"] = Animation("small_death");
    anims["small_death"].frameList = {{"mario_small_death", 0.15f}};

    // --- 2. Super Mario (32x64) ---
    anims["super_idle"] = Animation("super_idle");
    anims["super_idle"].frameList = {{"mario_super_idle", 0.15f}};

    anims["super_walk"] = Animation("super_walk");
    anims["super_walk"].frameList = {
        {"mario_super_walk_0", 0.12f},
        {"mario_super_walk_1", 0.12f},
        {"mario_super_walk_2", 0.12f}
    };

    anims["super_run"] = Animation("super_run");
    anims["super_run"].frameList = {
        {"mario_super_run_0", 0.08f},
        {"mario_super_run_1", 0.08f},
        {"mario_super_run_2", 0.08f}
    };

    anims["super_jump"] = Animation("super_jump");
    anims["super_jump"].frameList = {{"mario_super_jump", 0.15f}};

    anims["super_fall"] = Animation("super_fall");
    anims["super_fall"].frameList = {{"mario_super_fall", 0.15f}};

    anims["super_crouch"] = Animation("super_crouch");
    anims["super_crouch"].frameList = {{"mario_super_crouch", 0.15f}};

    anims["super_skid"] = Animation("super_skid");
    anims["super_skid"].frameList = {{"mario_super_skid", 0.15f}};

    anims["super_slide"] = Animation("super_slide");
    anims["super_slide"].frameList = {{"mario_super_slide", 0.15f}};

    anims["super_wall_slide"] = Animation("super_wall_slide");
    anims["super_wall_slide"].frameList = {{"mario_super_wall_slide", 0.15f}};

    anims["super_damaged"] = Animation("super_damaged");
    anims["super_damaged"].frameList = {{"mario_super_damaged", 0.15f}};

    // --- 3. Fire Mario (32x64) ---
    anims["fire_idle"] = Animation("fire_idle");
    anims["fire_idle"].frameList = {{"mario_fire_idle", 0.15f}};

    anims["fire_walk"] = Animation("fire_walk");
    anims["fire_walk"].frameList = {
        {"mario_fire_walk_0", 0.12f},
        {"mario_fire_walk_1", 0.12f},
        {"mario_fire_walk_2", 0.12f}
    };

    anims["fire_run"] = Animation("fire_run");
    anims["fire_run"].frameList = {
        {"mario_fire_run_0", 0.08f},
        {"mario_fire_run_1", 0.08f},
        {"mario_fire_run_2", 0.08f}
    };

    anims["fire_jump"] = Animation("fire_jump");
    anims["fire_jump"].frameList = {{"mario_fire_jump", 0.15f}};

    anims["fire_fall"] = Animation("fire_fall");
    anims["fire_fall"].frameList = {{"mario_fire_fall", 0.15f}};

    anims["fire_crouch"] = Animation("fire_crouch");
    anims["fire_crouch"].frameList = {{"mario_fire_crouch", 0.15f}};

    anims["fire_skid"] = Animation("fire_skid");
    anims["fire_skid"].frameList = {{"mario_fire_skid", 0.15f}};

    anims["fire_slide"] = Animation("fire_slide");
    anims["fire_slide"].frameList = {{"mario_fire_slide", 0.15f}};

    anims["fire_wall_slide"] = Animation("fire_wall_slide");
    anims["fire_wall_slide"].frameList = {{"mario_fire_wall_slide", 0.15f}};

    // --- 4. Cape Mario (32x64) ---
    anims["cape_idle"] = Animation("cape_idle");
    anims["cape_idle"].frameList = {{"mario_cape_idle", 0.15f}};

    anims["cape_walk"] = Animation("cape_walk");
    anims["cape_walk"].frameList = {
        {"mario_cape_walk_0", 0.12f},
        {"mario_cape_walk_1", 0.12f},
        {"mario_cape_walk_2", 0.12f}
    };

    anims["cape_run"] = Animation("cape_run");
    anims["cape_run"].frameList = {
        {"mario_cape_run_0", 0.08f},
        {"mario_cape_run_1", 0.08f},
        {"mario_cape_run_2", 0.08f}
    };

    anims["cape_jump"] = Animation("cape_jump");
    anims["cape_jump"].frameList = {{"mario_cape_jump", 0.15f}};

    anims["cape_fall"] = Animation("cape_fall");
    anims["cape_fall"].frameList = {{"mario_cape_fall", 0.15f}};

    anims["cape_crouch"] = Animation("cape_crouch");
    anims["cape_crouch"].frameList = {{"mario_cape_crouch", 0.15f}};

    anims["cape_skid"] = Animation("cape_skid");
    anims["cape_skid"].frameList = {{"mario_cape_skid", 0.15f}};

    anims["cape_glide"] = Animation("cape_glide");
    anims["cape_glide"].frameList = {
        {"mario_cape_glide_0", 0.12f},
        {"mario_cape_glide_1", 0.12f}
    };

    anims["cape_spin"] = Animation("cape_spin");
    anims["cape_spin"].frameList = {
        {"mario_cape_spin_0", 0.10f},
        {"mario_cape_spin_1", 0.10f},
        {"mario_cape_spin_2", 0.10f}
    };

    // --- 5. Mini Mario (16x16) ---
    anims["mini_idle"] = Animation("mini_idle");
    anims["mini_idle"].frameList = {{"mario_mini_idle", 0.15f}};

    anims["mini_walk"] = Animation("mini_walk");
    anims["mini_walk"].frameList = {
        {"mario_mini_walk_0", 0.12f},
        {"mario_mini_walk_1", 0.12f}
    };

    anims["mini_run"] = Animation("mini_run");
    anims["mini_run"].frameList = {
        {"mario_mini_walk_0", 0.08f},
        {"mario_mini_walk_1", 0.08f}
    };

    anims["mini_jump"] = Animation("mini_jump");
    anims["mini_jump"].frameList = {{"mario_mini_jump", 0.15f}};

    anims["mini_fall"] = Animation("mini_fall");
    anims["mini_fall"].frameList = {{"mario_mini_fall", 0.15f}};

    anims["mini_damaged"] = Animation("mini_damaged");
    anims["mini_damaged"].frameList = {{"mario_mini_damaged", 0.15f}};

    anims["mini_death"] = Animation("mini_death");
    anims["mini_death"].frameList = {{"mario_mini_death", 0.15f}};

    // --- 6. Mega Mario (128x128) ---
    anims["mega_idle"] = Animation("mega_idle");
    anims["mega_idle"].frameList = {{"mario_mega_idle", 0.15f}};

    anims["mega_walk"] = Animation("mega_walk");
    anims["mega_walk"].frameList = {
        {"mario_mega_walk_0", 0.15f},
        {"mario_mega_walk_1", 0.15f}
    };

    anims["mega_run"] = Animation("mega_run");
    anims["mega_run"].frameList = {
        {"mario_mega_walk_0", 0.10f},
        {"mario_mega_walk_1", 0.10f}
    };

    anims["mega_jump"] = Animation("mega_jump");
    anims["mega_jump"].frameList = {{"mario_mega_jump", 0.15f}};

    anims["mega_fall"] = Animation("mega_fall");
    anims["mega_fall"].frameList = {{"mario_mega_jump", 0.15f}};

    // Interactive State Variables
    FormState formState = FormState::Small;
    MotionState motionState = MotionState::Idle;

    sf::Vector2f marioPos(640.f, 500.f);
    sf::Vector2f marioVel(0.f, 0.f);

    bool facingRight = true;
    bool onGround = true;
    bool manualControl = true;
    bool showBBox = true;
    float renderScale = 2.0f;

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
                if (marioVel.y < 0.f) motionState = MotionState::Jump;
                else motionState = MotionState::Fall;
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
        std::string prefix = "small_";
        if (formState == FormState::Super) prefix = "super_";
        else if (formState == FormState::Fire) prefix = "fire_";
        else if (formState == FormState::Cape) prefix = "cape_";
        else if (formState == FormState::Mini) prefix = "mini_";
        else if (formState == FormState::Mega) prefix = "mega_";

        std::string animKey = prefix + "idle";
        switch (motionState) {
            case MotionState::Idle: animKey = prefix + "idle"; break;
            case MotionState::Walk: animKey = prefix + "walk"; break;
            case MotionState::Run: animKey = prefix + "run"; break;
            case MotionState::Jump: animKey = prefix + "jump"; break;
            case MotionState::Fall: animKey = prefix + "fall"; break;
            case MotionState::Crouch: animKey = prefix + "crouch"; break;
            case MotionState::Skid: animKey = prefix + "skid"; break;
            case MotionState::Slide: animKey = prefix + "slide"; break;
            case MotionState::WallSlide: animKey = prefix + "wall_slide"; break;
            case MotionState::Glide: animKey = prefix + "glide"; break;
            case MotionState::Spin: animKey = prefix + "spin"; break;
            case MotionState::Damaged: animKey = prefix + "damaged"; break;
            case MotionState::Death: animKey = (formState == FormState::Mini) ? "mini_death" : "small_death"; break;
        }

        // Fallback for missing animations in Mini/Mega
        if (anims.find(animKey) == anims.end()) {
            animKey = prefix + "idle";
        }

        if (anims.find(animKey) != anims.end()) {
            animator.play(&anims[animKey]);
        }
        animator.update(dt);

        // ImGui Controls Panel
        ImGui::Begin("Player Animation Tester (All 6 Forms)");
        ImGui::Text("Use A/D (or Left/Right) to Walk/Run");
        ImGui::Text("Hold Shift to Run | Press Space to Jump | Down to Crouch");

        ImGui::Separator();
        ImGui::Text("Character Powerup Form:");
        int currentForm = static_cast<int>(formState);
        if (ImGui::RadioButton("Small (32x32)", &currentForm, 0)) formState = FormState::Small; ImGui::SameLine();
        if (ImGui::RadioButton("Super (32x64)", &currentForm, 1)) formState = FormState::Super; ImGui::SameLine();
        if (ImGui::RadioButton("Fire (32x64)", &currentForm, 2)) formState = FormState::Fire;

        if (ImGui::RadioButton("Cape (32x64)", &currentForm, 3)) formState = FormState::Cape; ImGui::SameLine();
        if (ImGui::RadioButton("Mini (16x16)", &currentForm, 4)) formState = FormState::Mini; ImGui::SameLine();
        if (ImGui::RadioButton("Mega (128x128)", &currentForm, 5)) formState = FormState::Mega;

        ImGui::Separator();
        ImGui::Checkbox("Enable Interactive Keyboard Physics", &manualControl);
        ImGui::Checkbox("Show Bounding Box Overlay", &showBBox);
        ImGui::SliderFloat("Render Scale", &renderScale, 0.5f, 4.0f, "%.1fx");

        if (!manualControl) {
            ImGui::Separator();
            ImGui::Text("Manual Motion State Preview:");
            int currentMotion = static_cast<int>(motionState);
            if (ImGui::RadioButton("Idle", &currentMotion, 0)) motionState = MotionState::Idle; ImGui::SameLine();
            if (ImGui::RadioButton("Walk", &currentMotion, 1)) motionState = MotionState::Walk; ImGui::SameLine();
            if (ImGui::RadioButton("Run", &currentMotion, 2)) motionState = MotionState::Run; ImGui::SameLine();
            if (ImGui::RadioButton("Jump", &currentMotion, 3)) motionState = MotionState::Jump;

            if (ImGui::RadioButton("Fall", &currentMotion, 4)) motionState = MotionState::Fall; ImGui::SameLine();
            if (ImGui::RadioButton("Crouch", &currentMotion, 5)) motionState = MotionState::Crouch; ImGui::SameLine();
            if (ImGui::RadioButton("Skid", &currentMotion, 6)) motionState = MotionState::Skid; ImGui::SameLine();
            if (ImGui::RadioButton("Slide", &currentMotion, 7)) motionState = MotionState::Slide;

            if (ImGui::RadioButton("Glide", &currentMotion, 9)) motionState = MotionState::Glide; ImGui::SameLine();
            if (ImGui::RadioButton("Spin", &currentMotion, 10)) motionState = MotionState::Spin; ImGui::SameLine();
            if (ImGui::RadioButton("Damaged", &currentMotion, 11)) motionState = MotionState::Damaged; ImGui::SameLine();
            if (ImGui::RadioButton("Death", &currentMotion, 12)) motionState = MotionState::Death;

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

        float scaleX = facingRight ? renderScale : -renderScale;
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
