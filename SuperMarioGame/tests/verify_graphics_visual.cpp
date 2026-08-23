#include <algorithm>
#include <cmath>
#include <deque>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <vector>
#include <SFML/Graphics.hpp>
#include <imgui.h>
#include <imgui-SFML.h>
#include "Graphics/SpriteSheet.hpp"
#include "Graphics/Animation.hpp"
#include "Graphics/Animator.hpp"
#include "Graphics/Camera.hpp"
#include "Graphics/ScreenTransitionManager.hpp"
#include "Graphics/SpriteColorFilter.hpp"
#include "Graphics/SpriteTransformAnim.hpp"
#include "Graphics/EntityDeathEffect.hpp"
#include "Graphics/ParticleSystem.hpp"
#include "Graphics/ParticleEmitter.hpp"
#include "Core/ResourceManager.hpp"
#include "Core/EventBus.hpp"
#include "Utils/Constants.hpp"
#include "TestSaveSandbox.hpp"

struct CallbackLogEntry {
    std::string timestamp;
    std::string message;
};

std::string getCurrentTimeString(float totalTime) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(2) << "[" << totalTime << "s]";
    return ss.str();
}

std::string resolveSpriteSheetDirectory(const std::string& name) {
    std::vector<std::string> prefixes = {
        "assets/spritesheet/" + name,
        "../assets/spritesheet/" + name,
        "../../assets/spritesheet/" + name,
        "SuperMarioGame/assets/spritesheet/" + name,
        "../SuperMarioGame/assets/spritesheet/" + name
    };
    for (const auto& p : prefixes) {
        if (std::filesystem::exists(p + "/" + name + ".json")) {
            return p;
        }
    }
    return "assets/spritesheet/" + name;
}

// Unbound Suffix Sequence Builder (scans _0, _1, ..., _N continuously)
Animation buildUnboundAnimation(const SpriteSheet* sheet, const std::string& animName, const std::string& prefix, float frameDuration = 0.15f) {
    Animation anim(animName);
    anim.isLooping = true;

    if (!sheet) return anim;

    int idx = 0;
    while (true) {
        std::string frameKey = prefix + "_" + std::to_string(idx);
        sf::Sprite sprite = sheet->getSprite(frameKey);
        sf::FloatRect bounds = sprite.getLocalBounds();
        if (bounds.size.x == 0.0f && bounds.size.y == 0.0f) {
            break; // End of unbound sequence _N
        }
        anim.frameList.push_back({frameKey, frameDuration});
        idx++;
    }

    // Fallback: If no _0, _1 sequence exists, check for standalone prefix frame key
    if (anim.frameList.empty()) {
        sf::Sprite sprite = sheet->getSprite(prefix);
        sf::FloatRect bounds = sprite.getLocalBounds();
        if (bounds.size.x > 0.0f && bounds.size.y > 0.0f) {
            anim.frameList.push_back({prefix, frameDuration});
        }
    }

    return anim;
}

// Auto-locating Unbound Animation Builder (searches across all atlases to guarantee resolution)
Animation buildUnboundAnimationFromAny(const std::map<std::string, std::unique_ptr<SpriteSheet>>& sheets,
                                       const std::vector<std::string>& sheetNames,
                                       const std::string& animName,
                                       const std::string& prefix,
                                       int& outSheetIndex,
                                       float frameDuration = 0.15f)
{
    const SpriteSheet* targetSheet = nullptr;
    outSheetIndex = 0;

    for (size_t i = 0; i < sheetNames.size(); ++i) {
        const auto& sName = sheetNames[i];
        auto it = sheets.find(sName);
        if (it != sheets.end()) {
            sf::Sprite testSprite = it->second->getSprite(prefix + "_0");
            sf::FloatRect bounds = testSprite.getLocalBounds();
            if (bounds.size.x > 0.0f && bounds.size.y > 0.0f) {
                targetSheet = it->second.get();
                outSheetIndex = static_cast<int>(i);
                break;
            }
            testSprite = it->second->getSprite(prefix);
            bounds = testSprite.getLocalBounds();
            if (bounds.size.x > 0.0f && bounds.size.y > 0.0f) {
                targetSheet = it->second.get();
                outSheetIndex = static_cast<int>(i);
                break;
            }
        }
    }

    return buildUnboundAnimation(targetSheet, animName, prefix, frameDuration);
}

int main() {
    // Every save path in this process now points at a throwaway
    // directory, so nothing here can read or delete real save data
    // (g-rule-13). See TestSaveSandbox.hpp for what went wrong without it.
    TestSaveSandbox sandbox("graphics_visual");

    std::cout << "[VISUAL TEST] Launching Unbound Multi-Spritesheet, FX, Shake, Transitions & Transform Visualizer..." << std::endl;

    // Create SFML RenderWindow (1280x720)
    sf::RenderWindow window(sf::VideoMode(sf::Vector2u(1280, 720)), "Super Mario Engine — Transform & Color Filter Visualizer");
    window.setFramerateLimit(60);

    // Initialize ImGui-SFML
    if (!ImGui::SFML::Init(window)) {
        std::cerr << "[VISUAL TEST] Failed to initialize ImGui-SFML!" << std::endl;
        return 1;
    }

    // Load available SpriteSheets
    std::vector<std::string> sheetNames = {"player", "item", "enemy_projectile", "world_scenery_item", "particles"};
    std::map<std::string, std::unique_ptr<SpriteSheet>> spriteSheets;

    for (const auto& name : sheetNames) {
        std::string resolvedPath = resolveSpriteSheetDirectory(name);
        std::cout << "[VISUAL TEST] Loading SpriteSheet '" << name << "' from: " << resolvedPath << std::endl;
        spriteSheets[name] = std::make_unique<SpriteSheet>(resolvedPath);
    }

    int currentSheetIndex = 0; // default to "player"
    SpriteSheet* activeSheet = spriteSheets["player"].get();

    Camera camera;
    camera.setBounds(AABB{0.0f, 0.0f, 2560.0f, 720.0f});

    // Sheet index holders for presets
    int marioWalkIndex = 0, marioClimbIndex = 0, luigiIndex = 0, peachIndex = 0, toadIndex = 0;
    int fireFlowerRedIndex = 0, fireFlowerGreenIndex = 0, fireFlowerBlueIndex = 0, starIndex = 0, coinIndex = 0, powIndex = 0;
    int goombaIndex = 0, koopaIndex = 0, booIndex = 0, fireballIndex = 0;

    // Dynamically resolve unbound animations across all atlases
    Animation marioWalkAnim = buildUnboundAnimationFromAny(spriteSheets, sheetNames, "Mario Small Walk", "mario_small_walk", marioWalkIndex, 0.15f);
    Animation marioClimbAnim = buildUnboundAnimationFromAny(spriteSheets, sheetNames, "Mario Small Climb", "mario_small_climb", marioClimbIndex, 0.18f);
    Animation luigiWalkAnim = buildUnboundAnimationFromAny(spriteSheets, sheetNames, "Luigi Walk", "luigi_small_walk", luigiIndex, 0.15f);
    Animation peachWalkAnim = buildUnboundAnimationFromAny(spriteSheets, sheetNames, "Peach Walk", "peach_small_walk", peachIndex, 0.15f);
    Animation toadWalkAnim = buildUnboundAnimationFromAny(spriteSheets, sheetNames, "Toad Walk", "toad_small_walk", toadIndex, 0.15f);

    Animation fireFlowerRedAnim = buildUnboundAnimationFromAny(spriteSheets, sheetNames, "Fire Flower (Red)", "fire_flower", fireFlowerRedIndex, 0.12f);
    Animation fireFlowerGreenAnim = buildUnboundAnimationFromAny(spriteSheets, sheetNames, "Fire Flower (Green)", "fire_flower_green", fireFlowerGreenIndex, 0.12f);
    Animation fireFlowerBlueAnim = buildUnboundAnimationFromAny(spriteSheets, sheetNames, "Fire Flower (Blue)", "fire_flower_blue", fireFlowerBlueIndex, 0.12f);

    Animation starAnim = buildUnboundAnimationFromAny(spriteSheets, sheetNames, "Star Spin", "star", starIndex, 0.10f);
    Animation coinAnim = buildUnboundAnimationFromAny(spriteSheets, sheetNames, "Coin Spin", "coin", coinIndex, 0.12f);
    Animation powAnim = buildUnboundAnimationFromAny(spriteSheets, sheetNames, "POW Block", "pow_block", powIndex, 0.12f);

    Animation goombaWalkAnim = buildUnboundAnimationFromAny(spriteSheets, sheetNames, "Goomba Move", "goomba_brown_move", goombaIndex, 0.20f);
    Animation koopaWalkAnim = buildUnboundAnimationFromAny(spriteSheets, sheetNames, "Koopa Move Left", "koopa_green_move_left", koopaIndex, 0.20f);
    Animation booAttackAnim = buildUnboundAnimationFromAny(spriteSheets, sheetNames, "Boo Attack", "boo_attack", booIndex, 0.20f);
    Animation fireballAnim = buildUnboundAnimationFromAny(spriteSheets, sheetNames, "Fireball Spin", "flower_fireball", fireballIndex, 0.08f);

    // Dynamic Animation created when switching sheets
    Animation dynamicAtlasAnim("Dynamic Unbound Loop");
    dynamicAtlasAnim.isLooping = true;

    Animator animator(activeSheet);
    animator.play(&marioWalkAnim);

    const Animation* currentAnim = &marioWalkAnim;
    float speedMultiplier = 1.0f;
    float spriteRenderScale = 4.0f;
    bool isPlaying = true;
    int staticFrameIndex = 0;
    bool showStaticFrame = false;

    // Transformation & Color Filter FX State
    SpriteTransformAnim transformAnim;
    bool starPowerActive = false;
    float starPowerCycleSpeed = 600.0f;
    float sparkleTimer = 0.0f;

    bool hurtFlickerActive = false;
    float invincibilityTimer = 0.0f;
    float hurtElapsedTime = 0.0f;

    bool formFlickerActive = false;
    float formFlickerTimer = 0.0f;
    bool formFlickerToggleState = false;

    // Helper to safely switch spritesheets and reset animations cleanly
    auto switchSheet = [&](int sheetIndex) {
        currentSheetIndex = sheetIndex;
        activeSheet = spriteSheets[sheetNames[currentSheetIndex]].get();
        animator.setSpriteSheet(activeSheet);
        staticFrameIndex = 0;

        std::vector<std::string> frames = activeSheet->getFrameNames();
        if (!frames.empty()) {
            dynamicAtlasAnim.frameList.clear();
            dynamicAtlasAnim.name = "Unbound Atlas Sequence (" + sheetNames[currentSheetIndex] + ")";
            for (const auto& frameKey : frames) {
                dynamicAtlasAnim.frameList.push_back({frameKey, 0.18f});
            }
            animator.play(&dynamicAtlasAnim);
            currentAnim = &dynamicAtlasAnim;
        }
    };

    // Helper to trigger preset animations safely with atlas binding
    auto playPreset = [&](int sheetIndex, const Animation* anim) {
        currentSheetIndex = sheetIndex;
        activeSheet = spriteSheets[sheetNames[currentSheetIndex]].get();
        animator.setSpriteSheet(activeSheet);
        animator.play(anim);
        currentAnim = anim;
        showStaticFrame = false;
        isPlaying = true;
    };

    // Moving Sprite State (Engine Stability Check)
    sf::Vector2f spritePos(640.0f, 360.0f);
    float spriteSpeed = 150.0f;
    int spriteDir = 1;

    // Custom Shake State
    float customIntensity = 5.0f;
    float customDuration = 0.3f;
    sf::Vector2f customDir(0.0f, 1.0f);
    bool customDecay = true;
    float customFreq = 30.0f;

    // Custom Transition State
    int selectedTransitionType = 5; // PipeTransition
    float transitionDuration = 1.0f;
    sf::Vector2f transitionFocalPoint(640.0f, 360.0f);
    float transitionColor[3] = {0.0f, 0.0f, 0.0f};

    // Callback Logging & Interactive Verification State
    std::deque<CallbackLogEntry> callbackLogs;
    auto addLog = [&](const std::string& msg, float totalTime) {
        callbackLogs.push_front({getCurrentTimeString(totalTime), msg});
        if (callbackLogs.size() > 15) callbackLogs.pop_back();
    };

    int interactiveClickCount = 0;
    char textBuffer[64] = "Test Input";
    sf::Color backgroundClearColor = sf::Color(40, 44, 52);

    sf::Clock deltaClock;
    sf::Clock totalClock;

    sf::Sprite renderSprite(ResourceManager::getInstance().getTexture(""));

    // Game loop
    while (window.isOpen()) {
        sf::Time dt = deltaClock.restart();
        float dtSeconds = dt.asSeconds();
        float totalTime = totalClock.getElapsedTime().asSeconds();

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
            }
        }

        // 2. Update Moving Sprite & Camera Follow
        spritePos.x += spriteSpeed * spriteDir * dtSeconds;
        if (spritePos.x >= 1200.0f) {
            spritePos.x = 1200.0f;
            spriteDir = -1;
        } else if (spritePos.x <= 200.0f) {
            spritePos.x = 200.0f;
            spriteDir = 1;
        }

        if (isPlaying && !showStaticFrame) {
            animator.update(dtSeconds * speedMultiplier);
        }

        transformAnim.update(dtSeconds);
        ParticleSystem::getInstance().update(dtSeconds);
        EntityDeathEffect::getInstance().update(dtSeconds, 720.0f);



        // Hurt Flicker updates
        if (hurtFlickerActive) {
            hurtElapsedTime += dtSeconds;
            invincibilityTimer -= dtSeconds;
            if (invincibilityTimer <= 0.0f) {
                invincibilityTimer = 0.0f;
                hurtFlickerActive = false;
            }
        }

        // Form Flicker updates (12Hz flickering during transformation lerp)
        if (formFlickerActive) {
            formFlickerTimer += dtSeconds;
            if (formFlickerTimer >= (1.0f / 12.0f)) {
                formFlickerTimer = 0.0f;
                formFlickerToggleState = !formFlickerToggleState;
            }
            if (transformAnim.isFinished()) {
                formFlickerActive = false;
                formFlickerToggleState = false;
            }
        }

        camera.follow(spritePos, dtSeconds);
        camera.update(dtSeconds);

        ScreenTransitionManager::getInstance().update(dtSeconds);

        ImGui::SFML::Update(window, dt);

        // 3. ImGui Visualizer Window
        ImGui::SetNextWindowSize(ImVec2(640, 680), ImGuiCond_FirstUseEver);
        ImGui::Begin("Graphics & FX Visualizer Harness");

        if (ImGui::BeginTabBar("VisualizerTabs")) {
            // TAB 1: Animation & Spritesheet Visualizer
            if (ImGui::BeginTabItem("Animation & Spritesheets")) {
                ImGui::Text("Active Spritesheet Atlas:");
                const char* sheetLabels[] = {
                    "player (Mario, Luigi, Peach, Toad)",
                    "item (Powerups, Coins, Star, POW, PSwitch)",
                    "enemy_projectile (Goomba, Koopa, Boo, Bowser)",
                    "world_scenery_item (Blocks, Pipes, Tiles, Red Flower)",
                    "particles (Stomp, WallDust, Sparkle)"
                };

                int selectedSheet = currentSheetIndex;
                if (ImGui::Combo("Select Spritesheet", &selectedSheet, sheetLabels, 5)) {
                    switchSheet(selectedSheet);
                }

                ImGui::Separator();
                ImGui::Text("Unbound Animation Sequence Presets:");

                // Player Presets
                if (ImGui::Button("Mario Small Walk")) {
                    playPreset(marioWalkIndex, &marioWalkAnim);
                }
                ImGui::SameLine();
                if (ImGui::Button("Mario Small Climb")) {
                    playPreset(marioClimbIndex, &marioClimbAnim);
                }
                ImGui::SameLine();
                if (ImGui::Button("Luigi Walk")) {
                    playPreset(luigiIndex, &luigiWalkAnim);
                }

                if (ImGui::Button("Peach Walk")) {
                    playPreset(peachIndex, &peachWalkAnim);
                }
                ImGui::SameLine();
                if (ImGui::Button("Toad Walk")) {
                    playPreset(toadIndex, &toadWalkAnim);
                }

                // Item & Scenery Presets
                ImGui::Separator();
                if (ImGui::Button("Fire Flower (Red)")) {
                    playPreset(fireFlowerRedIndex, &fireFlowerRedAnim);
                }
                ImGui::SameLine();
                if (ImGui::Button("Fire Flower (Green)")) {
                    playPreset(fireFlowerGreenIndex, &fireFlowerGreenAnim);
                }
                ImGui::SameLine();
                if (ImGui::Button("Fire Flower (Blue)")) {
                    playPreset(fireFlowerBlueIndex, &fireFlowerBlueAnim);
                }

                if (ImGui::Button("Star Spin")) {
                    playPreset(starIndex, &starAnim);
                }
                ImGui::SameLine();
                if (ImGui::Button("Coin Spin")) {
                    playPreset(coinIndex, &coinAnim);
                }
                ImGui::SameLine();
                if (ImGui::Button("POW Block")) {
                    playPreset(powIndex, &powAnim);
                }

                // Enemy & Projectile Presets
                ImGui::Separator();
                if (ImGui::Button("Goomba Walk")) {
                    playPreset(goombaIndex, &goombaWalkAnim);
                }
                ImGui::SameLine();
                if (ImGui::Button("Koopa Walk")) {
                    playPreset(koopaIndex, &koopaWalkAnim);
                }
                ImGui::SameLine();
                if (ImGui::Button("Boo Attack")) {
                    playPreset(booIndex, &booAttackAnim);
                }
                ImGui::SameLine();
                if (ImGui::Button("Fireball Spin")) {
                    playPreset(fireballIndex, &fireballAnim);
                }

                ImGui::Separator();
                ImGui::Text("Playback Settings:");
                ImGui::Checkbox("Play/Pause", &isPlaying);
                ImGui::SameLine();
                if (ImGui::Button("Reset Speed")) speedMultiplier = 1.0f;
                ImGui::SliderFloat("Speed Multiplier", &speedMultiplier, 0.1f, 3.0f);
                ImGui::SliderFloat("Render Scale", &spriteRenderScale, 1.0f, 8.0f);

                ImGui::Separator();
                ImGui::Text("Frame Inspector for Active Sheet (%s):", sheetNames[currentSheetIndex].c_str());
                std::vector<std::string> frameNames = activeSheet->getFrameNames();
                ImGui::Text("Total Frames in Atlas: %zu", frameNames.size());

                if (ImGui::Checkbox("Enable Static Frame Inspection", &showStaticFrame)) {
                    if (showStaticFrame) isPlaying = false;
                }

                if (showStaticFrame && !frameNames.empty()) {
                    if (staticFrameIndex >= static_cast<int>(frameNames.size())) {
                        staticFrameIndex = 0;
                    }

                    if (ImGui::BeginCombo("Select Frame", frameNames[staticFrameIndex].c_str())) {
                        for (size_t i = 0; i < frameNames.size(); ++i) {
                            bool isSelected = (staticFrameIndex == static_cast<int>(i));
                            if (ImGui::Selectable(frameNames[i].c_str(), isSelected)) {
                                staticFrameIndex = static_cast<int>(i);
                            }
                            if (isSelected) ImGui::SetItemDefaultFocus();
                        }
                        ImGui::EndCombo();
                    }

                    ImGui::TextColored(ImVec4(1.0f, 0.84f, 0.0f, 1.0f), "Inspect Key: %s", frameNames[staticFrameIndex].c_str());
                }

                ImGui::EndTabItem();
            }

            // TAB 2: Transform & Color Filter FX (Tasks 5.8 & 5.9 Visualizer)
            if (ImGui::BeginTabItem("Transform & Color Filter FX")) {
                ImGui::TextColored(ImVec4(0.2f, 0.9f, 1.0f, 1.0f), "Task 5.8: Entity Death Animations");
                
                sf::Sprite goombaSpr = spriteSheets["enemy_projectile"]->getSprite("goomba_brown_move_0");
                sf::Sprite marioDeathSpr = spriteSheets["player"]->getSprite("mario_death");
                if (marioDeathSpr.getLocalBounds().size.x == 0) {
                    marioDeathSpr = spriteSheets["player"]->getSprite("mario_small_idle_right");
                }

                if (ImGui::Button("Trigger Enemy Flip Death Launch")) {
                    EntityDeathEffect::getInstance().spawnDeathEffect(spritePos, goombaSpr, DeathEffectType::EnemyFlip, sf::Vector2f(100.0f, -380.0f));
                    addLog("Spawned Enemy Flip Death Instance", totalTime);
                }
                ImGui::SameLine();
                if (ImGui::Button("Trigger Star Kill Spin Launch")) {
                    EntityDeathEffect::getInstance().spawnDeathEffect(spritePos, goombaSpr, DeathEffectType::StarKillSpin, sf::Vector2f(-150.0f, -480.0f));
                    addLog("Spawned Star Kill Spin Launch + CoinSparkle Burst", totalTime);
                }

                if (ImGui::Button("Trigger Player Death Hop & Fall")) {
                    EntityDeathEffect::getInstance().spawnDeathEffect(spritePos, marioDeathSpr, DeathEffectType::PlayerDeathHop);
                    addLog("Spawned Player Death Hop & Fall Instance", totalTime);
                }

                ImGui::Separator();
                ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f), "Task 5.9: Color Filters & Form Transformations");

                if (ImGui::Checkbox("Star Power Rainbow Hue Cycle & Sparkles", &starPowerActive)) {
                    addLog(starPowerActive ? "Star Power Activated" : "Star Power Deactivated", totalTime);
                }
                ImGui::SliderFloat("Rainbow Cycle Speed", &starPowerCycleSpeed, 100.0f, 1200.0f);

                if (ImGui::Button("Trigger Hit Invincibility / Hurt Flicker (2.0s)")) {
                    hurtFlickerActive = true;
                    invincibilityTimer = 2.0f;
                    hurtElapsedTime = 0.0f;
                    addLog("Triggered 2.0s Hit Invincibility Hurt Flicker", totalTime);
                }

                ImGui::Separator();
                ImGui::Text("Scale Transformation & Form Flickering Presets:");
                if (ImGui::Button("Mini (0.5x Scale)")) {
                    transformAnim.startScaleAnim(transformAnim.getCurrentScale(), 0.5f, 0.6f);
                    formFlickerActive = true; formFlickerTimer = 0.0f;
                    addLog("Scaling to Mini Form (0.5x)", totalTime);
                }
                ImGui::SameLine();
                if (ImGui::Button("Normal (1.0x Scale)")) {
                    transformAnim.startScaleAnim(transformAnim.getCurrentScale(), 1.0f, 0.6f);
                    formFlickerActive = true; formFlickerTimer = 0.0f;
                    addLog("Scaling to Normal Form (1.0x)", totalTime);
                }
                ImGui::SameLine();
                if (ImGui::Button("Super (1.5x Scale)")) {
                    transformAnim.startScaleAnim(transformAnim.getCurrentScale(), 1.5f, 0.6f);
                    formFlickerActive = true; formFlickerTimer = 0.0f;
                    addLog("Scaling to Super Form (1.5x)", totalTime);
                }
                ImGui::SameLine();
                if (ImGui::Button("Mega (3.0x Scale)")) {
                    transformAnim.startScaleAnim(transformAnim.getCurrentScale(), 3.0f, 0.6f);
                    formFlickerActive = true; formFlickerTimer = 0.0f;
                    addLog("Scaling to Mega Form (3.0x)", totalTime);
                }

                ImGui::Separator();
                ImGui::Text("Active FX Status Monitor:");
                ImGui::BulletText("Current Transform Scale: %.2fx", transformAnim.getCurrentScale());
                ImGui::BulletText("Star Power Active: %s", starPowerActive ? "YES" : "NO");
                ImGui::BulletText("Hurt Flicker Active: %s (Timer: %.2fs)", hurtFlickerActive ? "YES" : "NO", invincibilityTimer);
                ImGui::BulletText("Active Floating Death FX Count: %zu", EntityDeathEffect::getInstance().getInstances().size());

                ImGui::EndTabItem();
            }

            // TAB 3: Screen Shake Visualizer
            if (ImGui::BeginTabItem("Screen Shake")) {
                ImGui::Text("Camera Shake Presets:");
                if (ImGui::Button("Light Shake (2.0px, 0.10s)")) {
                    camera.triggerScreenShake(ShakePreset::Light);
                    addLog("Triggered ShakePreset::Light", totalTime);
                }
                ImGui::SameLine();
                if (ImGui::Button("Medium Shake (4.0px, 0.20s)")) {
                    camera.triggerScreenShake(ShakePreset::Medium);
                    addLog("Triggered ShakePreset::Medium", totalTime);
                }
                ImGui::SameLine();
                if (ImGui::Button("Heavy Shake (6.0px, 0.30s)")) {
                    camera.triggerScreenShake(ShakePreset::Heavy);
                    addLog("Triggered ShakePreset::Heavy", totalTime);
                }

                ImGui::Text("Custom Shake Parameters:");
                ImGui::SliderFloat("Intensity (px)", &customIntensity, 0.5f, 25.0f);
                ImGui::SliderFloat("Duration (s)", &customDuration, 0.05f, 2.0f);
                ImGui::SliderFloat2("Direction (X, Y)", &customDir.x, -1.0f, 1.0f);
                ImGui::Checkbox("Use Decay", &customDecay);
                ImGui::SliderFloat("Frequency (Hz)", &customFreq, 5.0f, 60.0f);

                if (ImGui::Button("Trigger Custom Shake")) {
                    ShakeParams params{customIntensity, customDuration, customDir, customDecay, customFreq};
                    camera.triggerScreenShake(params);
                    addLog("Triggered Custom Screen Shake", totalTime);
                }

                ImGui::Separator();
                ImGui::Text("EventBus Listener Emulation:");
                if (ImGui::Button("Publish POWBlockHit")) {
                    EventBus::getInstance().publish({EventType::POWBlockHit, nullptr});
                    addLog("EventBus Published: POWBlockHit", totalTime);
                }
                ImGui::SameLine();
                if (ImGui::Button("Publish ThwompSlam")) {
                    EventBus::getInstance().publish({EventType::ThwompSlam, nullptr});
                    addLog("EventBus Published: ThwompSlam", totalTime);
                }
                if (ImGui::Button("Publish GroundPoundSlam")) {
                    EventBus::getInstance().publish({EventType::GroundPoundSlam, nullptr});
                    addLog("EventBus Published: GroundPoundSlam", totalTime);
                }
                ImGui::SameLine();
                if (ImGui::Button("Publish PlayerDamaged")) {
                    EventBus::getInstance().publish({EventType::PlayerDamaged, nullptr});
                    addLog("EventBus Published: PlayerDamaged", totalTime);
                }
                if (ImGui::Button("Publish BossDefeated")) {
                    EventBus::getInstance().publish({EventType::BossDefeated, nullptr});
                    addLog("EventBus Published: BossDefeated", totalTime);
                }
                ImGui::SameLine();
                if (ImGui::Button("Publish BlockBroken")) {
                    EventBus::getInstance().publish({EventType::BlockBroken, nullptr});
                    addLog("EventBus Published: BlockBroken", totalTime);
                }

                ImGui::Separator();
                ImGui::Text("Active Camera Shake Monitor:");
                ImGui::BulletText("Is Shaking: %s", camera.isShaking() ? "YES" : "NO");
                ImGui::BulletText("Remaining Time: %.3fs", camera.getShakeRemainingTime());
                ImGui::BulletText("Elapsed Time: %.3fs", camera.getShakeElapsedTime());
                const ShakeParams& active = camera.getActiveShakeParams();
                ImGui::BulletText("Peak Intensity: %.1fpx", active.intensity);
                ImGui::BulletText("Direction: (%.2f, %.2f)", active.direction.x, active.direction.y);

                ImGui::EndTabItem();
            }

            // TAB 4: Screen Transitions & Stability Check
            if (ImGui::BeginTabItem("Transitions & Engine Stability")) {
                ScreenTransitionManager& stm = ScreenTransitionManager::getInstance();

                ImGui::Text("Transition Controls:");
                const char* transTypes[] = { "None", "FadeIn", "FadeOut", "CircleWipeIn", "CircleWipeOut", "PipeTransition" };
                ImGui::Combo("Effect Type", &selectedTransitionType, transTypes, 6);
                ImGui::SliderFloat("Duration (s)", &transitionDuration, 0.1f, 3.0f);
                ImGui::SliderFloat2("Focal Point", &transitionFocalPoint.x, 0.0f, 1280.0f);
                ImGui::ColorEdit3("Overlay Color", transitionColor);

                sf::Color overlayCol(static_cast<std::uint8_t>(transitionColor[0] * 255.0f),
                                     static_cast<std::uint8_t>(transitionColor[1] * 255.0f),
                                     static_cast<std::uint8_t>(transitionColor[2] * 255.0f));

                if (ImGui::Button("Start Selected Transition")) {
                    TransitionType type = static_cast<TransitionType>(selectedTransitionType);
                    stm.startTransition(type, transitionDuration,
                        [&addLog, totalTime]() { addLog("ON_MIDPOINT Callback Fired!", totalTime); },
                        [&addLog, totalTime]() { addLog("ON_COMPLETE Callback Fired!", totalTime); },
                        transitionFocalPoint, overlayCol);
                    addLog("Started Custom Transition", totalTime);
                }

                ImGui::Separator();
                ImGui::Text("Quick Effect Shortcuts:");
                if (ImGui::Button("Fade Out (0.5s)")) {
                    stm.fadeOut(0.5f, [&addLog, totalTime]() { addLog("Fade Out Complete", totalTime); }, overlayCol);
                }
                ImGui::SameLine();
                if (ImGui::Button("Fade In (0.5s)")) {
                    stm.fadeIn(0.5f, [&addLog, totalTime]() { addLog("Fade In Complete", totalTime); }, overlayCol);
                }
                if (ImGui::Button("Pipe Wipe (1.0s)")) {
                    stm.pipeWipe(1.0f, spritePos,
                        [&addLog, totalTime, &backgroundClearColor]() {
                            addLog("Pipe Midpoint: Swapped Scene Background Color", totalTime);
                            backgroundClearColor = sf::Color(20, 60, 40);
                        },
                        [&addLog, totalTime, &backgroundClearColor]() {
                            addLog("Pipe Complete: Scene Ready!", totalTime);
                            backgroundClearColor = sf::Color(40, 44, 52);
                        });
                }
                ImGui::SameLine();
                if (ImGui::Button("Circle Wipe Out (0.75s)")) {
                    stm.circleWipeOut(0.75f, spritePos,
                        [&addLog, totalTime]() { addLog("Circle Wipe Midpoint Fired", totalTime); },
                        [&addLog, totalTime]() { addLog("Circle Wipe Complete Fired", totalTime); });
                }

                ImGui::Separator();
                ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.4f, 1.0f), "POST-TRANSITION ENGINE STABILITY MONITOR");

                const char* stateStr = "Idle";
                switch (stm.getState()) {
                    case TransitionState::Out: stateStr = "Out (Obscuring)"; break;
                    case TransitionState::Midpoint: stateStr = "Midpoint (State Swap)"; break;
                    case TransitionState::In: stateStr = "In (Revealing)"; break;
                    case TransitionState::Completed: stateStr = "Completed"; break;
                    case TransitionState::Idle: stateStr = "Idle"; break;
                }

                ImGui::BulletText("Transition State: %s", stateStr);
                ImGui::BulletText("Transition Progress: %.2f", stm.getProgress());
                ImGui::BulletText("Frame Clock Active: YES (FPS: %.1f)", 1.0f / (dtSeconds > 0.0001f ? dtSeconds : 0.016f));
                ImGui::BulletText("Sprite Movement & Physics Active: YES (Pos: %.1f, %.1f)", spritePos.x, spritePos.y);
                ImGui::BulletText("Camera Target Follow Active: YES (Center: %.1f, %.1f)", camera.getView().getCenter().x, camera.getView().getCenter().y);

                ImGui::Separator();
                ImGui::Text("Input System & UI Unlocked Test:");
                if (ImGui::Button("Click Test Button (Post-Transition Responsiveness)")) {
                    interactiveClickCount++;
                    addLog("Interactive Click Count: " + std::to_string(interactiveClickCount), totalTime);
                }
                ImGui::SameLine();
                ImGui::Text("Clicks: %d", interactiveClickCount);
                ImGui::InputText("Text Input Test", textBuffer, sizeof(textBuffer));

                ImGui::Separator();
                ImGui::Text("Recent Callback Log:");
                ImGui::BeginChild("CallbackLogRegion", ImVec2(0, 120), true);
                for (const auto& entry : callbackLogs) {
                    ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.3f, 1.0f), "%s %s", entry.timestamp.c_str(), entry.message.c_str());
                }
                ImGui::EndChild();

                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        ImGui::End();

        // 4. Render Scene with Camera View
        window.clear(backgroundClearColor);

        window.setView(camera.getView());

        // Draw ground guide line
        sf::RectangleShape groundLine(sf::Vector2f(2560.0f, 4.0f));
        groundLine.setPosition(sf::Vector2f(0.0f, 400.0f));
        groundLine.setFillColor(sf::Color(100, 100, 100));
        window.draw(groundLine);

        // Render moving animated sprite safely
        std::vector<std::string> activeFrameNames = activeSheet->getFrameNames();
        if (showStaticFrame && !activeFrameNames.empty()) {
            if (staticFrameIndex >= static_cast<int>(activeFrameNames.size())) staticFrameIndex = 0;
            renderSprite = activeSheet->getSprite(activeFrameNames[staticFrameIndex]);
        } else {
            renderSprite = animator.getSprite();
        }

        sf::FloatRect bounds = renderSprite.getLocalBounds();
        if (bounds.size.x > 0.0f && bounds.size.y > 0.0f) {
            // Apply scale transformation animation & floor origin anchoring
            transformAnim.applyToSprite(renderSprite, spriteDir > 0);
            renderSprite.setPosition(sf::Vector2f(spritePos.x, 400.0f)); // Floor anchored at Y=400

            // Apply color filter modulation
            sf::Color curFilter = sf::Color::White;
            if (starPowerActive) {
                curFilter = SpriteColorFilter::getRainbowColor(totalTime, starPowerCycleSpeed);
            } else if (hurtFlickerActive) {
                curFilter = SpriteColorFilter::getHurtFlickerColor(invincibilityTimer, hurtElapsedTime, 15.0f);
            }

            if (formFlickerActive && formFlickerToggleState) {
                curFilter.a = static_cast<std::uint8_t>(curFilter.a * 0.3f);
            }

            SpriteColorFilter::applyColorFilter(renderSprite, curFilter);

            window.draw(renderSprite);

            // Render sprite bounding box indicator
            sf::RectangleShape box(sf::Vector2f(bounds.size.x * transformAnim.getCurrentScale(), bounds.size.y * transformAnim.getCurrentScale()));
            box.setOrigin(sf::Vector2f(bounds.size.x * (transformAnim.getCurrentScale() / 2.0f), bounds.size.y * transformAnim.getCurrentScale()));
            box.setPosition(sf::Vector2f(spritePos.x, 400.0f));
            box.setFillColor(sf::Color::Transparent);
            box.setOutlineColor(sf::Color::Green);
            box.setOutlineThickness(1.5f);
            window.draw(box);
        }

        // Render floating death FX instances
        EntityDeathEffect::getInstance().render(window);

        // Render particle system FX
        window.draw(ParticleSystem::getInstance());

        // Render Screen Transition Overlay in Screen Space
        ScreenTransitionManager::getInstance().render(window);

        // Render ImGui Overlays
        ImGui::SFML::Render(window);

        window.display();
    }

    ImGui::SFML::Shutdown();
    return 0;
}
