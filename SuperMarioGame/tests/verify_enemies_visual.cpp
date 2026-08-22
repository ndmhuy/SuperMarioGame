#include <iostream>
#include <filesystem>
#include <unordered_map>
#include <memory>
#include <string>
#include <vector>
#include <cmath>
#include <algorithm>
#include <SFML/Graphics.hpp>
#include <imgui.h>
#include <imgui-SFML.h>

#include "Core/ResourceManager.hpp"
#include "Graphics/SpriteSheet.hpp"
#include "Graphics/Animation.hpp"
#include "Graphics/Animator.hpp"
#include "Utils/Constants.hpp"

// Entities & AI Strategies
#include "Entities/Enemy.hpp"
#include "Entities/Goomba.hpp"
#include "Entities/KoopaTroopa.hpp"
#include "Entities/KoopaParatroopa.hpp"
#include "Entities/Boo.hpp"
#include "Entities/PiranhaPlant.hpp"
#include "Entities/BulletBill.hpp"
#include "Entities/HammerBro.hpp"
#include "Entities/Thwomp.hpp"
#include "Entities/ChainChomp.hpp"
#include "Entities/Lakitu.hpp"
#include "Entities/Spiny.hpp"
#include "Entities/Mario.hpp"

#include "Entities/PatrolStrategy.hpp"
#include "Entities/ChaseStrategy.hpp"
#include "Entities/FlyStrategy.hpp"
#include "Entities/TimerEmergenceStrategy.hpp"
#include "Entities/LinearStrategy.hpp"
#include "Entities/HammerThrowStrategy.hpp"
#include "Entities/TetheredChaseStrategy.hpp"
#include "Entities/ProximityTriggerStrategy.hpp"
#include "TestSaveSandbox.hpp"

enum RoomType {
    ROOM_ALL = 0,
    ROOM_GOOMBA,
    ROOM_KOOPA_TROOPA,
    ROOM_KOOPA_PARATROOPA,
    ROOM_BOO,
    ROOM_THWOMP,
    ROOM_PIRANHA_PLANT,
    ROOM_CHAIN_CHOMP,
    ROOM_LAKITU,
    ROOM_SPINY,
    ROOM_HAMMER_BRO,
    ROOM_BULLET_BILL,
    ROOM_COUNT
};

const char* ROOM_NAMES[ROOM_COUNT] = {
    "0: All Rooms (Overview)",
    "1: Goomba Room (Wall & Ledge Patrol)",
    "2: KoopaTroopa Room (Platform Patrol)",
    "3: KoopaParatroopa Room (Flying & Landing)",
    "4: Boo Room (Line-of-Sight & Chase)",
    "5: Thwomp Room (Ceiling & Proximity Slam)",
    "6: PiranhaPlant Room (Pipe Emergence)",
    "7: ChainChomp Room (Post & Tethered Chase)",
    "8: Lakitu Room (Sky Patrol & Egg Drop)",
    "9: Spiny Room (Egg Fall & Ground Patrol)",
    "10: HammerBro Room (Multi-Level Platforms)",
    "11: BulletBill Room (Corridor Blaster)"
};

int main() {
    // Every save path in this process now points at a throwaway
    // directory, so nothing here can read or delete real save data
    // (g-rule-13). See TestSaveSandbox.hpp for what went wrong without it.
    TestSaveSandbox sandbox("enemies_visual");

    std::cout << "[VISUAL TEST] Launching Enemy AI & Strategy Edge-Case Test Environment..." << std::endl;

    sf::RenderWindow window(sf::VideoMode(sf::Vector2u(1280, 720)), "Enemy AI & Strategy Interactive Test Environment");
    window.setFramerateLimit(60);

    if (!ImGui::SFML::Init(window)) {
        std::cerr << "[VISUAL TEST] Failed to initialize ImGui-SFML!" << std::endl;
        return 1;
    }

    // Resolve resource paths
    ResourceManager& rm = ResourceManager::getInstance();
    std::string sfxPath = ResourceManager::resolvePath("assets/spriteSheet/enemy_projectile/enemy_projectile.png");
    std::string jsonPath = ResourceManager::resolvePath("assets/spriteSheet/enemy_projectile/enemy_projectile.json");
    std::string playerPng = ResourceManager::resolvePath("assets/spriteSheet/player/player.png");
    std::string playerJson = ResourceManager::resolvePath("assets/spriteSheet/player/player.json");

    rm.loadTexture("enemyTexture", sfxPath);
    rm.loadTexture("playerTexture", playerPng);

    SpriteSheet enemySheet("enemyTexture", jsonPath);
    SpriteSheet playerSheet("playerTexture", playerJson);

    // --- Environment State & Invincible Player Avatar ---
    sf::Vector2f playerPos(640.0f, 500.0f);
    sf::Vector2f playerVel(0.0f, 0.0f);
    bool playerFacingRight = true;
    bool showGizmos = true;
    bool showAABB = true;
    bool showAttackRadius = true;
    float timeScale = 1.0f;
    int currentRoom = ROOM_ALL;

    // Create Invincible Mock Player
    Mario playerObj(playerPos);
    playerObj.setupAnimations(&playerSheet);

    // Create 11 Test Enemies
    std::vector<std::unique_ptr<Enemy>> enemies;
    auto spawnEnemies = [&]() {
        enemies.clear();
        
        // 1. Goomba (Patrol wall & ledge test)
        auto goomba = std::make_unique<Goomba>(sf::Vector2f(200.0f, 528.0f), "brown");
        goomba->setupAnimations(&enemySheet);
        enemies.push_back(std::move(goomba));

        // 2. KoopaTroopa (Patrol)
        auto koopa = std::make_unique<KoopaTroopa>(sf::Vector2f(320.0f, 348.0f), "green");
        koopa->setupAnimations(&enemySheet);
        enemies.push_back(std::move(koopa));

        // 3. KoopaParatroopa (Fly -> Patrol on stomp)
        auto paratroopa = std::make_unique<KoopaParatroopa>(sf::Vector2f(750.0f, 320.0f), "green");
        paratroopa->setupAnimations(&enemySheet);
        enemies.push_back(std::move(paratroopa));

        // 4. Boo (Chase / Hide on player sight)
        auto boo = std::make_unique<Boo>(sf::Vector2f(450.0f, 250.0f));
        boo->setupAnimations(&enemySheet);
        enemies.push_back(std::move(boo));

        // 5. Thwomp (Proximity Slam)
        auto thwomp = std::make_unique<Thwomp>(sf::Vector2f(550.0f, 140.0f));
        thwomp->setupAnimations(&enemySheet);
        enemies.push_back(std::move(thwomp));

        // 6. PiranhaPlant (Timer emergence / Stand suppression)
        auto piranha = std::make_unique<PiranhaPlant>(sf::Vector2f(234.0f, 464.0f));
        piranha->setupAnimations(&enemySheet);
        enemies.push_back(std::move(piranha));

        // 7. ChainChomp (Tethered Chase)
        auto chomp = std::make_unique<ChainChomp>(sf::Vector2f(880.0f, 528.0f));
        chomp->setupAnimations(&enemySheet);
        enemies.push_back(std::move(chomp));

        // 8. Lakitu (Fly & Spiny spawn)
        auto lakitu = std::make_unique<Lakitu>(sf::Vector2f(600.0f, 100.0f));
        lakitu->setupAnimations(&enemySheet);
        enemies.push_back(std::move(lakitu));

        // 9. Spiny (Falling egg -> Patrol)
        auto spiny = std::make_unique<Spiny>(sf::Vector2f(650.0f, 528.0f));
        spiny->setupAnimations(&enemySheet);
        enemies.push_back(std::move(spiny));

        // 10. HammerBro (Hammer throw & hop)
        auto hammerBro = std::make_unique<HammerBro>(sf::Vector2f(980.0f, 392.0f));
        hammerBro->setupAnimations(&enemySheet);
        enemies.push_back(std::move(hammerBro));

        // 11. BulletBill (Horizontal Blaster Corridor)
        auto bulletBill = std::make_unique<BulletBill>(sf::Vector2f(132.0f, 512.0f), 1.0f);
        bulletBill->setupAnimations(&enemySheet);
        enemies.push_back(std::move(bulletBill));
    };

    spawnEnemies();

    sf::Clock clock;

    while (window.isOpen()) {
        float dt = clock.restart().asSeconds() * timeScale;
        if (dt > 0.1f) dt = 0.1f;

        while (const auto event = window.pollEvent()) {
            ImGui::SFML::ProcessEvent(window, *event);

            if (event->is<sf::Event::Closed>()) {
                window.close();
            }

            // Right-Click Teleportation: Teleport Invincible Player to Mouse Cursor
            if (const auto* mouseBtn = event->getIf<sf::Event::MouseButtonPressed>()) {
                if (mouseBtn->button == sf::Mouse::Button::Right && !ImGui::GetIO().WantCaptureMouse) {
                    sf::Vector2i mousePos = sf::Mouse::getPosition(window);
                    playerPos = sf::Vector2f(static_cast<float>(mousePos.x), static_cast<float>(mousePos.y));
                    std::cout << "[TELEPORT] Player teleported to (" << playerPos.x << ", " << playerPos.y << ")" << std::endl;
                }
            }

            // 'F' key toggle player facing direction
            if (const auto* key = event->getIf<sf::Event::KeyPressed>()) {
                if (key->code == sf::Keyboard::Key::F) {
                    playerFacingRight = !playerFacingRight;
                }
            }
        }

        // --- Invincible Player Controls (WASD / Arrows) ---
        float moveSpeed = 300.0f;
        playerVel = {0.0f, 0.0f};
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left)) {
            playerVel.x = -moveSpeed;
            playerFacingRight = false;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right)) {
            playerVel.x = moveSpeed;
            playerFacingRight = true;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up)) {
            playerVel.y = -moveSpeed;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down)) {
            playerVel.y = moveSpeed;
        }

        playerPos += playerVel * dt;
        playerPos.x = std::clamp(playerPos.x, 30.0f, 1250.0f);
        playerPos.y = std::clamp(playerPos.y, 30.0f, 680.0f);

        playerObj.setPosition(playerPos);
        playerObj.setFacingRight(playerFacingRight);

        // Ground and Wall Terrain Clamping Constants for Zero Clipping
        const float mainGroundY = 560.0f;
        const float elevatedPlatformY = 380.0f;

        // --- Hammer Projectiles Struct & State ---
        struct HammerProj {
            sf::Vector2f pos;
            sf::Vector2f vel;
            float rotation = 0.0f;
        };
        static std::vector<HammerProj> activeHammers;
        static float hammerTimer = 0.0f;

        // --- Thwomp State Machine State ---
        static int thwompState = 0; // 0=Idle, 1=RampUp, 2=Slam, 3=Rest, 4=Rise
        static float thwompTimer = 0.0f;
        static const sf::Vector2f thwompHomePos(550.0f, 140.0f);

        // Update active enemies & apply exact terrain collision clamping
        for (size_t i = 0; i < enemies.size(); ++i) {
            if (!enemies[i]) continue;
            
            // Filter update if in specific room
            if (currentRoom != ROOM_ALL && static_cast<int>(i + 1) != currentRoom) {
                continue;
            }

            // --- 3. Boo (Index 3): Sight-Tracking Watch & Pursuit ---
            if (i == 3) {
                sf::Vector2f bPos = enemies[3]->getPosition();
                float dx = playerPos.x - bPos.x;
                float dy = playerPos.y - bPos.y;
                float dist = std::sqrt(dx*dx + dy*dy);
                bool playerToRight = (playerPos.x > bPos.x);
                bool isPlayerWatching = (playerToRight && !playerFacingRight) || (!playerToRight && playerFacingRight);

                if (isPlayerWatching) {
                    enemies[3]->setVelocity({0.0f, 0.0f});
                } else if (dist <= 300.0f && dist > 0.01f) {
                    sf::Vector2f dir(dx / dist, dy / dist);
                    enemies[3]->setVelocity(dir * 100.0f);
                    enemies[3]->setFacingRight(dx > 0.0f);
                } else {
                    enemies[3]->setVelocity({0.0f, 0.0f});
                }
            }

            // --- 4. Thwomp (Index 4): Trigger Column, 0.2s Ramp Up, Slam Down, Rest & Slow Rise ---
            if (i == 4) {
                sf::Vector2f tPos = enemies[4]->getPosition();
                float dx = std::abs(playerPos.x - (tPos.x + 24.0f));
                bool playerUnderneath = (playerPos.x >= 470.0f && playerPos.x <= 630.0f && playerPos.y > tPos.y);

                if (thwompState == 0) { // Idle at home position
                    tPos = thwompHomePos;
                    enemies[4]->setVelocity({0.0f, 0.0f});
                    if (dx <= 80.0f && playerUnderneath) {
                        thwompState = 1; // Ramp Up
                        thwompTimer = 0.2f;
                    }
                } else if (thwompState == 1) { // Ramp Up (0.2s vibration)
                    thwompTimer -= dt;
                    float vib = (static_cast<int>(thwompTimer * 100) % 2 == 0) ? 3.0f : -3.0f;
                    tPos.x = thwompHomePos.x + vib;
                    enemies[4]->setVelocity({0.0f, 0.1f}); // trigger angry active animation
                    if (thwompTimer <= 0.0f) {
                        tPos.x = thwompHomePos.x;
                        thwompState = 2; // Rapid Slam
                    }
                } else if (thwompState == 2) { // Rapid Slam Down (700 px/s)
                    enemies[4]->setVelocity({0.0f, 700.0f});
                    tPos.y += 700.0f * dt;
                    if (tPos.y + 64.0f >= mainGroundY) {
                        tPos.y = mainGroundY - 64.0f;
                        enemies[4]->setVelocity({0.0f, 0.0f});
                        thwompState = 3; // Ground Rest
                        thwompTimer = 0.5f;
                    }
                } else if (thwompState == 3) { // Ground Rest (0.5s)
                    thwompTimer -= dt;
                    enemies[4]->setVelocity({0.0f, 0.0f});
                    if (thwompTimer <= 0.0f) {
                        thwompState = 4; // Slow Rise
                    }
                } else if (thwompState == 4) { // Slow Rise (-80 px/s)
                    enemies[4]->setVelocity({0.0f, -80.0f});
                    tPos.y -= 80.0f * dt;
                    if (tPos.y <= thwompHomePos.y) {
                        tPos.y = thwompHomePos.y;
                        enemies[4]->setVelocity({0.0f, 0.0f});
                        thwompState = 0; // Reset to Idle
                    }
                }

                enemies[4]->setPosition(tPos);
                enemies[4]->update(dt);
                continue;
            }

            // --- 9. HammerBro (Index 9): Look at player & throw parabolic hammers ---
            if (i == 9) {
                sf::Vector2f hbPos = enemies[9]->getPosition();
                bool faceRight = (playerPos.x > hbPos.x);
                enemies[9]->setFacingRight(faceRight);

                hammerTimer += dt;
                if (hammerTimer >= 1.5f) {
                    hammerTimer = 0.0f;
                    HammerProj h;
                    h.pos = sf::Vector2f(hbPos.x + 16.0f, hbPos.y + 10.0f);
                    float vx = faceRight ? 220.0f : -220.0f;
                    h.vel = sf::Vector2f(vx, -380.0f);
                    activeHammers.push_back(h);
                }
            }

            enemies[i]->update(dt);

            // Exact Terrain, Velocity Integration & Wall Collision Clamping
            sf::Vector2f vel = enemies[i]->getVelocity();
            sf::Vector2f ePos = enemies[i]->getPosition();
            AABB box = enemies[i]->getBoundingBox();

            // BulletBill wrap-around corridor check (Index 10: BulletBill)
            if (i == 10) {
                ePos.x += 150.0f * dt; // Blaster firing horizontal velocity
                if (ePos.x > 1220.0f) {
                    ePos.x = 132.0f; // Reset to Bill Blaster nozzle
                }
                enemies[i]->setPosition(ePos);
                continue;
            }

            // 1. Apply Gravity Acceleration for ground-based and hopping enemies (Exclude Boo=3 and Lakitu=7)
            if (i != 3 && i != 7) {
                if (!enemies[i]->isOnGround()) {
                    vel.y += 1200.0f * dt; // Gravity acceleration (1200 px/s^2)
                    if (vel.y > 600.0f) vel.y = 600.0f; // Terminal velocity
                    enemies[i]->setVelocity(vel);
                }
            }

            // Integrate AI velocity into position
            ePos += vel * dt;

            // Wide Room Wall Boundaries
            const float roomMinX = 100.0f;
            const float roomMaxX = 1180.0f;

            if (ePos.x <= roomMinX) {
                ePos.x = roomMinX;
                enemies[i]->setOnWall(true); 
            } else if (ePos.x + box.width >= roomMaxX) {
                ePos.x = roomMaxX - box.width;
                enemies[i]->setOnWall(true); 
            }

            // Flying enemies (Boo = index 3, Lakitu = index 7) do not clamp to ground
            if (i != 3 && i != 7) {
                float currentGround = mainGroundY;
                // HammerBro (index 9) platform landing support
                if (i == 9 && ePos.x >= 880.0f && ePos.x <= 1080.0f && ePos.y + box.height <= 440.0f + 25.0f) {
                    currentGround = 440.0f;
                } else if (ePos.x >= 100.0f && ePos.x <= 400.0f && ePos.y + box.height <= elevatedPlatformY + 20.0f) {
                    currentGround = elevatedPlatformY;
                }

                if (ePos.y + box.height >= currentGround) {
                    ePos.y = currentGround - box.height;
                    vel.y = 0.0f;
                    enemies[i]->setVelocity(vel);
                    enemies[i]->setGrounded(true);
                } else {
                    enemies[i]->setGrounded(false);
                }
            }

            // Solid Barrier Wall obstacle collision check (placed at x=750 to x=782)
            float wallLeft = 750.0f;
            float wallRight = 782.0f;
            if (ePos.y + box.height > 480.0f && ePos.y < 560.0f) {
                if (ePos.x + box.width >= wallLeft && ePos.x < wallLeft) {
                    ePos.x = wallLeft - box.width;
                    enemies[i]->setOnWall(true);
                } else if (ePos.x <= wallRight && ePos.x + box.width > wallRight) {
                    ePos.x = wallRight;
                    enemies[i]->setOnWall(true);
                }
            }

            enemies[i]->setPosition(ePos);
        }

        // --- Update Active Hammer Projectiles ---
        for (size_t h = 0; h < activeHammers.size(); ) {
            activeHammers[h].vel.y += 1200.0f * dt;
            activeHammers[h].pos += activeHammers[h].vel * dt;
            activeHammers[h].rotation += 720.0f * dt;
            if (activeHammers[h].pos.y > 560.0f || activeHammers[h].pos.x < 50.0f || activeHammers[h].pos.x > 1200.0f) {
                activeHammers.erase(activeHammers.begin() + h);
            } else {
                ++h;
            }
        }

        ImGui::SFML::Update(window, sf::Time(sf::seconds(dt)));

        // --- ImGui Control Panel ---
        ImGui::Begin("Enemy AI & Strategy Edge-Case Tester");

        // 1. Room Selection Combo & Tab Bar
        ImGui::Text("Room Environment Selection:");
        ImGui::Combo("Select Room", &currentRoom, ROOM_NAMES, ROOM_COUNT);

        if (ImGui::BeginTabBar("RoomTabs")) {
            for (int r = 0; r < ROOM_COUNT; ++r) {
                char tabLabel[32];
                snprintf(tabLabel, sizeof(tabLabel), "Room %d", r);
                if (ImGui::BeginTabItem(tabLabel)) {
                    currentRoom = r;
                    ImGui::EndTabItem();
                }
            }
            ImGui::EndTabBar();
        }

        ImGui::Separator();
        ImGui::Text("Invincible Player Controls:");
        ImGui::Text(" - WASD / Arrow Keys: Move Player");
        ImGui::Text(" - Right Click Anywhere: Teleport Player instantly");
        ImGui::Text(" - Press 'F': Toggle Player Facing Direction (Currently: %s)", playerFacingRight ? "RIGHT ->" : "<- LEFT");
        ImGui::Separator();

        ImGui::Text("Teleport Presets:");
        if (ImGui::Button("Under Thwomp")) playerPos = {550.0f, 520.0f};
        ImGui::SameLine();
        if (ImGui::Button("Facing Boo")) { playerPos = {550.0f, 250.0f}; playerFacingRight = false; }
        ImGui::SameLine();
        if (ImGui::Button("Behind Boo")) { playerPos = {350.0f, 250.0f}; playerFacingRight = true; }
        
        if (ImGui::Button("On Piranha Pipe")) playerPos = {234.0f, 432.0f};
        ImGui::SameLine();
        if (ImGui::Button("ChainChomp Radius")) playerPos = {830.0f, 520.0f};
        ImGui::SameLine();
        if (ImGui::Button("HammerBro Platform")) playerPos = {960.0f, 390.0f};

        ImGui::Separator();
        ImGui::SliderFloat("Simulation Speed", &timeScale, 0.1f, 3.0f, "%.1fx");
        ImGui::Checkbox("Show Visual AABB Bounding Boxes", &showAABB);
        ImGui::Checkbox("Show Attack Range Radius", &showAttackRadius);
        ImGui::Checkbox("Show Debug Gizmos", &showGizmos);
        if (ImGui::Button("Reset All Enemies")) spawnEnemies();

        ImGui::Separator();
        ImGui::Text("Edge-Case Status & Active States:");
        for (size_t i = 0; i < enemies.size(); ++i) {
            if (!enemies[i]) continue;
            sf::Vector2f ePos = enemies[i]->getPosition();
            AABB box = enemies[i]->getBoundingBox();
            ImGui::Text("[%zu] %s at (%.0f, %.0f) - AABB: %.0fx%.0f", i, typeid(*enemies[i]).name(), ePos.x, ePos.y, box.width, box.height);
        }

        ImGui::End();

        // --- Render World & Terrain ---
        window.clear(sf::Color(40, 50, 60));

        // 1. Draw Terrain Floors & Solid Wall Boundaries
        sf::RectangleShape floor(sf::Vector2f(1180.0f, 160.0f));
        floor.setPosition(sf::Vector2f(50.0f, 560.0f));
        floor.setFillColor(sf::Color(100, 100, 100));
        window.draw(floor);

        // Elevated Platform (Ledge Test: x=100 to x=400, y=380)
        sf::RectangleShape platform(sf::Vector2f(300.0f, 20.0f));
        platform.setPosition(sf::Vector2f(100.0f, 380.0f));
        platform.setFillColor(sf::Color(140, 100, 60));
        window.draw(platform);

        // Solid Boundary Wall (Wall Hit Test: x=750 to 782)
        sf::RectangleShape wall(sf::Vector2f(32.0f, 80.0f));
        wall.setPosition(sf::Vector2f(750.0f, 480.0f));
        wall.setFillColor(sf::Color(80, 80, 80));
        window.draw(wall);

        // Pipe for Piranha Plant (x=234, y=496)
        sf::RectangleShape pipe(sf::Vector2f(64.0f, 64.0f));
        pipe.setPosition(sf::Vector2f(234.0f, 496.0f));
        pipe.setFillColor(sf::Color(30, 160, 40));
        window.draw(pipe);

        // Bill Blaster Cannon Shooter using bullet_bill_combined sprite (x=100, y=496)
        sf::Sprite blasterSprite = enemySheet.getSprite("bullet_bill_combined");
        sf::FloatRect bounds = blasterSprite.getLocalBounds();
        if (bounds.size.x > 0.0f && bounds.size.y > 0.0f) {
            // Keep correct aspect ratio scaling (width=16, height=32 -> w=32, h=64 at scale=2)
            blasterSprite.setScale(sf::Vector2f(2.0f, 2.0f));
            blasterSprite.setPosition(sf::Vector2f(100.0f, 496.0f));
            window.draw(blasterSprite);
        } else {
            sf::RectangleShape blasterBase(sf::Vector2f(32.0f, 64.0f));
            blasterBase.setPosition(sf::Vector2f(100.0f, 496.0f));
            blasterBase.setFillColor(sf::Color(40, 40, 45));
            window.draw(blasterBase);
        }

        // HammerBro Platform (x=900, y=440)
        sf::RectangleShape hbPlatform(sf::Vector2f(160.0f, 16.0f));
        hbPlatform.setPosition(sf::Vector2f(900.0f, 440.0f));
        hbPlatform.setFillColor(sf::Color(180, 80, 40));
        window.draw(hbPlatform);

        // 2. Draw 11 Enemies (Filtered by selected room environment) & Togglable Attack Radius Overlays
        for (size_t i = 0; i < enemies.size(); ++i) {
            if (!enemies[i]) continue;
            if (currentRoom != ROOM_ALL && static_cast<int>(i + 1) != currentRoom) continue;

            enemies[i]->render(window);

            // Togglable Attack Range Radius Overlay
            if (showAttackRadius) {
                sf::Vector2f eCenter = enemies[i]->getPosition() + sf::Vector2f(enemies[i]->getBoundingBox().width * 0.5f, enemies[i]->getBoundingBox().height * 0.5f);
                float radius = 0.0f;
                bool isTriggered = false;

                if (i == 3) { // Boo: 250px radius
                    radius = 250.0f;
                    float dx = playerPos.x - eCenter.x;
                    float dy = playerPos.y - eCenter.y;
                    isTriggered = (std::sqrt(dx*dx + dy*dy) <= radius) && (playerFacingRight);
                } else if (i == 4) { // Thwomp: 80px X-column
                    float dx = std::abs(playerPos.x - eCenter.x);
                    isTriggered = (dx <= 80.0f) && (playerPos.y > eCenter.y);
                    sf::RectangleShape colBox(sf::Vector2f(160.0f, 380.0f));
                    colBox.setOrigin(sf::Vector2f(80.0f, 0.0f));
                    colBox.setPosition(sf::Vector2f(eCenter.x, eCenter.y));
                    colBox.setFillColor(isTriggered ? sf::Color(255, 50, 50, 40) : sf::Color(255, 220, 0, 20));
                    colBox.setOutlineColor(isTriggered ? sf::Color(255, 50, 50, 220) : sf::Color(255, 220, 0, 180));
                    colBox.setOutlineThickness(1.5f);
                    window.draw(colBox);
                } else if (i == 6) { // ChainChomp: 180px radius
                    radius = 180.0f;
                    float dx = playerPos.x - eCenter.x;
                    float dy = playerPos.y - eCenter.y;
                    isTriggered = (std::sqrt(dx*dx + dy*dy) <= radius);
                } else if (i == 9) { // HammerBro: 300px radius
                    radius = 300.0f;
                    float dx = playerPos.x - eCenter.x;
                    float dy = playerPos.y - eCenter.y;
                    isTriggered = (std::sqrt(dx*dx + dy*dy) <= radius);
                }

                if (radius > 0.0f) {
                    sf::CircleShape rangeCircle(radius);
                    rangeCircle.setOrigin(sf::Vector2f(radius, radius));
                    rangeCircle.setPosition(eCenter);
                    rangeCircle.setFillColor(isTriggered ? sf::Color(255, 50, 50, 35) : sf::Color(255, 220, 0, 15));
                    rangeCircle.setOutlineColor(isTriggered ? sf::Color(255, 50, 50, 220) : sf::Color(255, 220, 0, 180));
                    rangeCircle.setOutlineThickness(1.5f);
                    window.draw(rangeCircle);
                }
            }

            if (showAABB) {
                AABB b = enemies[i]->getBoundingBox();
                sf::RectangleShape rect(sf::Vector2f(b.width, b.height));
                rect.setPosition(sf::Vector2f(b.x, b.y));
                rect.setFillColor(sf::Color::Transparent);
                rect.setOutlineColor(sf::Color::Green);
                rect.setOutlineThickness(1.0f);
                window.draw(rect);
            }

            if (showGizmos) {
                // Draw Velocity Arrow
                sf::Vector2f pos = enemies[i]->getPosition();
                sf::Vector2f vel = enemies[i]->getVelocity();
                if (std::abs(vel.x) > 1.0f || std::abs(vel.y) > 1.0f) {
                    sf::Vertex line[] = {
                        sf::Vertex{pos, sf::Color::Cyan},
                        sf::Vertex{pos + vel * 0.2f, sf::Color::Yellow}
                    };
                    window.draw(line, 2, sf::PrimitiveType::Lines);
                }
            }
        }

        // 3. Render Active Parabolic Hammer Projectiles for HammerBro
        if (currentRoom == ROOM_ALL || currentRoom == ROOM_HAMMER_BRO) {
            for (const auto& h : activeHammers) {
                sf::Sprite hSprite = enemySheet.getSprite("hammer_black_0");
                sf::FloatRect bounds = hSprite.getLocalBounds();
                if (bounds.size.x > 0.0f && bounds.size.y > 0.0f) {
                    hSprite.setOrigin(sf::Vector2f(bounds.size.x * 0.5f, bounds.size.y * 0.5f));
                    hSprite.setScale(sf::Vector2f(2.0f, 2.0f));
                    hSprite.setPosition(h.pos);
                    hSprite.setRotation(sf::degrees(h.rotation));
                    window.draw(hSprite);
                }
            }
        }

        // 4. Draw Crisp Invincible Player Avatar (Without yellow aura circle)
        playerObj.render(window);

        // Draw Player Facing Line-of-Sight Ray (Magenta)
        sf::Vector2f rayDir = playerFacingRight ? sf::Vector2f(150.0f, 0.0f) : sf::Vector2f(-150.0f, 0.0f);
        sf::Vector2f rayStart = playerPos - sf::Vector2f(0.0f, 16.0f);
        sf::Vertex sightRay[] = {
            sf::Vertex{rayStart, sf::Color::Magenta},
            sf::Vertex{rayStart + rayDir, sf::Color::Magenta}
        };
        window.draw(sightRay, 2, sf::PrimitiveType::Lines);

        ImGui::SFML::Render(window);
        window.display();
    }

    ImGui::SFML::Shutdown();
    return 0;
}

