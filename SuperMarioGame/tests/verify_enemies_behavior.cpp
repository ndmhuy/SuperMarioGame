#include <SFML/Graphics.hpp>
#include <imgui.h>
#include <imgui-SFML.h>
#include <iostream>
#include <memory>
#include <vector>
#include <cmath>
#include <algorithm>
#include <typeinfo>

#include "Entities/Goomba.hpp"
#include "Entities/KoopaTroopa.hpp"
#include "Entities/KoopaParatroopa.hpp"
#include "Entities/Boo.hpp"
#include "Entities/Thwomp.hpp"
#include "Entities/PiranhaPlant.hpp"
#include "Entities/ChainChomp.hpp"
#include "Entities/Lakitu.hpp"
#include "Entities/Spiny.hpp"
#include "Entities/HammerBro.hpp"
#include "Entities/BulletBill.hpp"
#include "Entities/Mario.hpp"
#include "Graphics/SpriteSheet.hpp"
#include "Utils/TileMap.hpp"
#include "Physics/PhysicsEngine.hpp"
#include "Core/InputManager.hpp"
#include "Core/SoundManager.hpp"
#include "Core/Game.hpp"
#include "Utils/Constants.hpp"

// Enums & Names for the 11 Enemies
enum EnemyType {
    ENEMY_GOOMBA = 0,
    ENEMY_KOOPA_TROOPA,
    ENEMY_KOOPA_PARATROOPA,
    ENEMY_BOO,
    ENEMY_THWOMP,
    ENEMY_PIRANHA_PLANT,
    ENEMY_CHAIN_CHOMP,
    ENEMY_LAKITU,
    ENEMY_SPINY,
    ENEMY_HAMMER_BRO,
    ENEMY_BULLET_BILL,
    ENEMY_COUNT
};

const char* ENEMY_NAMES[ENEMY_COUNT] = {
    "1. Goomba (Patrol & Edge)",
    "2. Koopa Troopa (Patrol)",
    "3. Koopa Paratroopa (Fly)",
    "4. Boo (Sight Chase)",
    "5. Thwomp (Slam & Rise)",
    "6. Piranha Plant (Emergence)",
    "7. Chain Chomp (Tether)",
    "8. Lakitu (Egg Spawning)",
    "9. Spiny (Egg Hatching)",
    "10. Hammer Bro (Throw & Hop)",
    "11. Bullet Bill (Shooter)"
};

// Projectile Structures
struct HammerProj {
    sf::Vector2f pos;
    sf::Vector2f vel;
    float rotation = 0.0f;
};

struct LakituEgg {
    sf::Vector2f pos;
    sf::Vector2f vel;
    bool hatched = false;
};

int main() {
    sf::RenderWindow window(sf::VideoMode(sf::Vector2u(1280, 720)), "Super Mario Game - Enemy Behavior Test Suite");
    window.setFramerateLimit(60);

    if (!ImGui::SFML::Init(window)) {
        std::cerr << "Failed to initialize ImGui-SFML!" << std::endl;
        return -1;
    }

    // Load Sprite Sheets
    SpriteSheet playerSheet("assets/spriteSheet/player");
    SpriteSheet enemySheet("assets/spriteSheet/enemy_projectile");

    // Pre-load all sound effects upfront to eliminate audio load latency
    SoundManager::getInstance().loadAllSounds();

    // Environment & Interactive Controls State
    int activeEnemyIdx = ENEMY_GOOMBA;
    bool enableFloor = true;
    bool enableLedge = true;
    bool enableWall = true;
    bool enablePipe = true;
    bool enableHammerPlatform = true;
    bool enableBlaster = true;

    bool showAABB = true;
    bool showThreatRadius = true;
    bool showVelocity = true;
    float timeScale = 1.0f;
    float throwSpeed = 250.0f;
    float throwAngleDeg = 10.0f;

    // Master list of entities for the physics engine
    std::vector<std::unique_ptr<Entity>> entities;
    std::vector<HammerProj> activeHammers;
    std::vector<LakituEgg> activeLakituEggs;

    // TileMap setup
    TileMap tileMap;
    tileMap.initialize(40, 23); // 1280x736 px (40 cols x 23 rows of 32x32 tiles)
    Game::getInstance().setTileMap(&tileMap);

    // Dynamic Safe Accessors (Never cache raw entity pointers across calls)
    auto getPlayer = [&]() -> Player* {
        if (!entities.empty() && entities[0]) {
            return dynamic_cast<Player*>(entities[0].get());
        }
        return nullptr;
    };

    auto getEnemy = [&]() -> Enemy* {
        if (entities.size() > 1 && entities[1]) {
            return dynamic_cast<Enemy*>(entities[1].get());
        }
        return nullptr;
    };

    // Thwomp State Machine
    int thwompState = 0; // 0=Idle, 1=RampUp, 2=Slam, 3=Rest, 4=Rise
    float thwompTimer = 0.0f;
    const sf::Vector2f thwompHomePos(544.0f, 128.0f);
    float lakituTimer = 0.0f;
    float hammerTimer = 0.0f;

    auto syncTileMap = [&]() {
        // Clear all tiles
        for (int y = 0; y < 23; ++y) {
            for (int x = 0; x < 40; ++x) {
                tileMap.setTile(x, y, TileType::Empty);
            }
        }
        // 1. Flat Floor: y = 544 (gy = 17) to gy = 22
        if (enableFloor) {
            for (int gy = 17; gy < 23; ++gy) {
                for (int gx = 7; gx < 31; ++gx) { // x = 224 to 992
                    tileMap.setTile(gx, gy, TileType::Ground);
                }
            }
        }
        // 2. Elevated Ledge: y = 352 (gy = 11)
        if (enableLedge) {
            for (int gx = 7; gx <= 14; ++gx) { // x = 224 to 448
                tileMap.setTile(gx, 11, TileType::Brick);
            }
        }
        // 3. Blocking Wall: x = 640 (gx = 20), gy = 15, 16
        if (enableWall) {
            for (int gy = 15; gy < 17; ++gy) {
                tileMap.setTile(20, gy, TileType::Ground);
            }
        }
        // 4. Pipe: x = 224 (gx = 7, 8), y = 480 (gy = 15, 16)
        if (enablePipe) {
            tileMap.setTile(7, 15, TileType::Pipe);
            tileMap.setTile(8, 15, TileType::Pipe);
            tileMap.setTile(7, 16, TileType::Pipe);
            tileMap.setTile(8, 16, TileType::Pipe);
        }
        // 5. Hammer Platform: x = 800 (gx = 25) to 960 (gx = 29), y = 416 (gy = 13)
        if (enableHammerPlatform) {
            for (int gx = 25; gx <= 29; ++gx) {
                tileMap.setTile(gx, 13, TileType::Brick);
            }
        }
        // 6. Blaster: x = 96 (gx = 3, 4), y = 480 (gy = 15, 16)
        if (enableBlaster) {
            tileMap.setTile(3, 15, TileType::Ground);
            tileMap.setTile(4, 15, TileType::Ground);
            tileMap.setTile(3, 16, TileType::Ground);
            tileMap.setTile(4, 16, TileType::Ground);
        }
    };

    auto spawnActiveEnemy = [&]() {
        // Safely unbind singletons before clearing entity memory
        Game::getInstance().setPlayer(nullptr);
        InputManager::getInstance().registerPlayer(nullptr, 0);

        entities.clear();
        activeHammers.clear();
        activeLakituEggs.clear();
        thwompState = 0;
        thwompTimer = 0.0f;
        lakituTimer = 0.0f;
        hammerTimer = 0.0f;

        // Setup Player for testing: vulnerable to hurt/knockback, but immortal (cannot die)
        auto player = std::make_unique<Mario>(sf::Vector2f(400.0f, 512.0f));
        player->setupAnimations(&playerSheet);
        player->setImmortal(true);
        player->setInvincible(0.0f);
        InputManager::getInstance().registerPlayer(player.get(), 0);
        Game::getInstance().setPlayer(player.get());
        entities.push_back(std::move(player));

        // 2. Setup Selected Enemy
        switch (activeEnemyIdx) {
            case ENEMY_GOOMBA: {
                auto e = std::make_unique<Goomba>(sf::Vector2f(300.0f, 512.0f), false);
                e->setupAnimations(&enemySheet);
                entities.push_back(std::move(e));
                break;
            }
            case ENEMY_KOOPA_TROOPA: {
                auto e = std::make_unique<KoopaTroopa>(sf::Vector2f(320.0f, 320.0f), false);
                e->setupAnimations(&enemySheet);
                entities.push_back(std::move(e));
                break;
            }
            case ENEMY_KOOPA_PARATROOPA: {
                auto e = std::make_unique<KoopaParatroopa>(sf::Vector2f(650.0f, 320.0f), false);
                e->setupAnimations(&enemySheet);
                entities.push_back(std::move(e));
                break;
            }
            case ENEMY_BOO: {
                auto e = std::make_unique<Boo>(sf::Vector2f(450.0f, 250.0f));
                e->setupAnimations(&enemySheet);
                entities.push_back(std::move(e));
                break;
            }
            case ENEMY_THWOMP: {
                auto e = std::make_unique<Thwomp>(thwompHomePos);
                e->setupAnimations(&enemySheet);
                entities.push_back(std::move(e));
                break;
            }
            case ENEMY_PIRANHA_PLANT: {
                auto e = std::make_unique<PiranhaPlant>(sf::Vector2f(224.0f, 448.0f));
                e->setupAnimations(&enemySheet);
                entities.push_back(std::move(e));
                break;
            }
            case ENEMY_CHAIN_CHOMP: {
                auto e = std::make_unique<ChainChomp>(sf::Vector2f(750.0f, 512.0f));
                e->setupAnimations(&enemySheet);
                entities.push_back(std::move(e));
                break;
            }
            case ENEMY_LAKITU: {
                auto e = std::make_unique<Lakitu>(sf::Vector2f(500.0f, 100.0f));
                e->setupAnimations(&enemySheet);
                entities.push_back(std::move(e));
                break;
            }
            case ENEMY_SPINY: {
                auto e = std::make_unique<Spiny>(sf::Vector2f(550.0f, 512.0f));
                e->setupAnimations(&enemySheet);
                entities.push_back(std::move(e));
                break;
            }
            case ENEMY_HAMMER_BRO: {
                auto e = std::make_unique<HammerBro>(sf::Vector2f(900.0f, 384.0f));
                e->setupAnimations(&enemySheet);
                entities.push_back(std::move(e));
                break;
            }
            case ENEMY_BULLET_BILL: {
                auto e = std::make_unique<BulletBill>(sf::Vector2f(128.0f, 480.0f), 1.0f);
                e->setupAnimations(&enemySheet);
                entities.push_back(std::move(e));
                break;
            }
        }
    };

    auto safeWorldChange = [&]() {
        syncTileMap();
        spawnActiveEnemy();
    };

    spawnActiveEnemy();
    syncTileMap();

    PhysicsEngine physicsEngine;
    sf::Clock clock;

    while (window.isOpen()) {
        float dt = clock.restart().asSeconds() * timeScale;
        if (dt > 0.1f) dt = 0.1f;

        // Revive mechanics for testing
        if (Player* p = getPlayer()) {
            if (!p->isActive() || p->getLives() <= 0) {
                p->active = true;
                p->setPosition(sf::Vector2f(400.0f, 512.0f));
                p->setVelocity(sf::Vector2f(0.0f, 0.0f));
            }

            // Clamp player position to testing room canvas
            sf::Vector2f pPos = p->getPosition();
            if (pPos.x < 230.0f) { pPos.x = 230.0f; p->setPosition(pPos); }
            if (pPos.x > 970.0f) { pPos.x = 970.0f; p->setPosition(pPos); }
            if (pPos.y > 680.0f) {
                pPos.y = 512.0f;
                p->setPosition(pPos);
                p->setVelocity({0.0f, 0.0f});
            }
        }

        // Prune extra inactive entities (e.g., hatched Spinies that died)
        if (entities.size() > 2) {
            entities.erase(
                std::remove_if(entities.begin() + 2, entities.end(), [](const std::unique_ptr<Entity>& e) {
                    return !e || !e->isActive();
                }),
                entities.end()
            );
        }

        while (const auto event = window.pollEvent()) {
            ImGui::SFML::ProcessEvent(window, *event);

            if (event->is<sf::Event::Closed>()) {
                window.close();
            }

            // Route events to standard player input
            if (Player* p = getPlayer()) {
                InputManager::getInstance().handleInput(*event, *p);
            }

            // Press F to throw held entity (e.g. Koopa Shell)
            if (const auto* keyPress = event->getIf<sf::Event::KeyPressed>()) {
                if (keyPress->code == sf::Keyboard::Key::F) {
                    if (Player* p = getPlayer()) {
                        if (auto* koopa = dynamic_cast<KoopaTroopa*>(p->getHeldEntity())) {
                            koopa->throwShell(550.0f, 45.0f);
                            p->releaseHeldEntity();
                        }
                    }
                }
            }

            // Right-Click Instant Teleportation
            if (const auto* mouseBtn = event->getIf<sf::Event::MouseButtonPressed>()) {
                if (mouseBtn->button == sf::Mouse::Button::Right && !ImGui::GetIO().WantCaptureMouse) {
                    sf::Vector2i mousePos = sf::Mouse::getPosition(window);
                    if (Player* p = getPlayer()) {
                        p->setPosition(sf::Vector2f(static_cast<float>(mousePos.x), static_cast<float>(mousePos.y)));
                        p->setVelocity(sf::Vector2f(0.0f, 0.0f));
                    }
                }
            }
        }

        // Process held keys for normal player movement
        if (Player* p = getPlayer()) {
            InputManager::getInstance().update(*p);
        }

        const float mainGroundY = 544.0f;

        // --- Specific Enemy AI Triggers ---
        Player* curPlayer = getPlayer();
        Enemy* curEnemy = getEnemy();
        if (curPlayer && curEnemy && curEnemy->isActive()) {
            sf::Vector2f playerPos = curPlayer->getPosition();

            // 1. Boo Chase & Sight Reaction
            if (activeEnemyIdx == ENEMY_BOO) {
                sf::Vector2f bPos = curEnemy->getPosition();
                float dx = playerPos.x - bPos.x;
                float dy = playerPos.y - bPos.y;
                float dist = std::sqrt(dx*dx + dy*dy);
                bool playerToRight = (playerPos.x > bPos.x);
                bool playerFacingRight = curPlayer->isFacingRight();
                bool isPlayerWatching = (playerToRight && !playerFacingRight) || (!playerToRight && playerFacingRight);

                if (isPlayerWatching) {
                    curEnemy->setVelocity({0.0f, 0.0f});
                } else if (dist <= 300.0f && dist > 0.01f) {
                    sf::Vector2f dir(dx / dist, dy / dist);
                    curEnemy->setVelocity(dir * 100.0f);
                    curEnemy->setFacingRight(dx > 0.0f);
                } else {
                    curEnemy->setVelocity({0.0f, 0.0f});
                }
            }

            // 2. Thwomp State Machine
            if (activeEnemyIdx == ENEMY_THWOMP) {
                sf::Vector2f tPos = curEnemy->getPosition();
                float dx = std::abs(playerPos.x - (tPos.x + 24.0f));
                bool playerUnderneath = (playerPos.x >= 470.0f && playerPos.x <= 630.0f && playerPos.y > tPos.y);

                if (thwompState == 0) { // Idle at home
                    tPos = thwompHomePos;
                    curEnemy->setVelocity({0.0f, 0.0f});
                    if (dx <= 80.0f && playerUnderneath) {
                        thwompState = 1;
                        thwompTimer = 0.2f;
                    }
                } else if (thwompState == 1) { // Ramp Up (0.2s vibration)
                    thwompTimer -= dt;
                    float vib = (static_cast<int>(thwompTimer * 100) % 2 == 0) ? 3.0f : -3.0f;
                    tPos.x = thwompHomePos.x + vib;
                    curEnemy->setVelocity({0.0f, 0.1f}); // angry face
                    if (thwompTimer <= 0.0f) {
                        tPos.x = thwompHomePos.x;
                        thwompState = 2;
                    }
                } else if (thwompState == 2) { // Slam Down
                    curEnemy->setVelocity({0.0f, 700.0f});
                    tPos.y += 700.0f * dt;
                    if (tPos.y + 64.0f >= mainGroundY) {
                        tPos.y = mainGroundY - 64.0f;
                        curEnemy->setVelocity({0.0f, 0.0f});
                        thwompState = 3;
                        thwompTimer = 0.5f;
                    }
                } else if (thwompState == 3) { // Ground Rest
                    thwompTimer -= dt;
                    curEnemy->setVelocity({0.0f, 0.0f});
                    if (thwompTimer <= 0.0f) {
                        thwompState = 4;
                    }
                } else if (thwompState == 4) { // Rise Up
                    curEnemy->setVelocity({0.0f, -80.0f});
                    tPos.y -= 80.0f * dt;
                    if (tPos.y <= thwompHomePos.y) {
                        tPos.y = thwompHomePos.y;
                        curEnemy->setVelocity({0.0f, 0.0f});
                        thwompState = 0;
                    }
                }
                curEnemy->setPosition(tPos);
            }

            // 3. Lakitu Egg Spawning
            if (activeEnemyIdx == ENEMY_LAKITU) {
                sf::Vector2f lPos = curEnemy->getPosition();
                lakituTimer += dt;
                if (lakituTimer >= 3.0f) {
                    lakituTimer = 0.0f;
                    // Spawn a real Spiny entity in egg state
                    sf::Vector2f spawnPos = lPos + sf::Vector2f(16.0f, 16.0f);
                    auto spinyEgg = std::make_unique<Spiny>(spawnPos, true);
                    spinyEgg->setupAnimations(&enemySheet);
                    spinyEgg->setVelocity(sf::Vector2f((playerPos.x > lPos.x ? 80.0f : -80.0f), -100.0f));
                    entities.push_back(std::move(spinyEgg));
                }
            }

            // 4. HammerBro Hopping & Projectile Throwing
            if (activeEnemyIdx == ENEMY_HAMMER_BRO) {
                sf::Vector2f hbPos = curEnemy->getPosition();
                bool faceRight = (playerPos.x > hbPos.x);
                curEnemy->setFacingRight(faceRight);

                hammerTimer += dt;
                if (hammerTimer >= 1.5f) {
                    hammerTimer = 0.0f;
                    HammerProj h;
                    h.pos = sf::Vector2f(hbPos.x + 16.0f, hbPos.y + 10.0f);
                    
                    // Precise parabolic throw velocity targeting player position with speed caps
                    float dx = playerPos.x - h.pos.x;
                    float dy = playerPos.y - h.pos.y;
                    float vy0 = -380.0f;
                    float g = 1200.0f;

                    float discriminant = vy0 * vy0 + 2.0f * g * dy;
                    float flightTime = 0.633f;
                    if (discriminant >= 0.0f) {
                        float T = (-vy0 + std::sqrt(discriminant)) / g;
                        if (T > 0.1f) flightTime = T;
                    }

                    float targetVx = std::clamp(dx / flightTime, -400.0f, 400.0f);
                    if (std::abs(targetVx) < 80.0f) targetVx = (targetVx >= 0.0f) ? 80.0f : -80.0f;
                    
                    // 10% velocity inaccuracy variation (multiplier between 0.90 and 1.10)
                    float variationVx = 1.0f + ((static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX)) * 0.20f - 0.10f);
                    float variationVy = 1.0f + ((static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX)) * 0.20f - 0.10f);
                    h.vel = sf::Vector2f(targetVx * variationVx, -380.0f * variationVy);
                    activeHammers.push_back(h);
                }
            }
        }

        // --- Update Active Projectiles ---
        for (size_t h = 0; h < activeHammers.size(); ) {
            activeHammers[h].vel.y += 1200.0f * dt;
            activeHammers[h].pos += activeHammers[h].vel * dt;
            activeHammers[h].rotation += 720.0f * dt;

            // Hitbox collision check against Player
            bool hitPlayer = false;
            AABB hammerBox{ activeHammers[h].pos.x - 8.0f, activeHammers[h].pos.y - 8.0f, 16.0f, 16.0f };
            if (Player* p = getPlayer()) {
                if (p->isActive() && hammerBox.intersects(p->getBoundingBox())) {
                    if (p->getInvincibilityTimer() == 0.0f) {
                        float dx = p->getPosition().x - activeHammers[h].pos.x;
                        float dir = (dx >= 0.0f) ? 1.0f : -1.0f;
                        p->setVelocity(sf::Vector2f(dir * Constants::KNOCKBACK_FORCE_X, -Constants::KNOCKBACK_FORCE_Y));
                        p->takeDamage(1);
                    }
                    hitPlayer = true;
                }
            }

            if (hitPlayer || activeHammers[h].pos.y > 560.0f || activeHammers[h].pos.x < 100.0f || activeHammers[h].pos.x > 1100.0f) {
                activeHammers.erase(activeHammers.begin() + h);
            } else {
                ++h;
            }
        }

        for (size_t eg = 0; eg < activeLakituEggs.size(); ) {
            if (!activeLakituEggs[eg].hatched) {
                activeLakituEggs[eg].vel.y += 1200.0f * dt;
                activeLakituEggs[eg].pos += activeLakituEggs[eg].vel * dt;
                if (activeLakituEggs[eg].pos.y >= mainGroundY - 32.0f) {
                    activeLakituEggs[eg].pos.y = mainGroundY - 32.0f;
                    activeLakituEggs[eg].hatched = true;
                    // Spawn a real Spiny entity!
                    auto spiny = std::make_unique<Spiny>(activeLakituEggs[eg].pos);
                    spiny->setupAnimations(&enemySheet);
                    entities.push_back(std::move(spiny));
                }
            }
            if (activeLakituEggs[eg].hatched) {
                activeLakituEggs.erase(activeLakituEggs.begin() + eg);
            } else {
                ++eg;
            }
        }

        // Update active animators/states
        for (auto& entity : entities) {
            if (entity && entity->isActive()) {
                entity->update(dt);
            }
        }

        // Run full collision check pipeline and move entities
        physicsEngine.update(entities, tileMap, dt);

        ImGui::SFML::Update(window, sf::Time(sf::seconds(dt)));

        // =========================================================================
        // 1. LEFT SIDEBAR: ENEMY SELECTION LIST
        // =========================================================================
        ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
        ImGui::SetNextWindowSize(ImVec2(220.0f, 720.0f));
        ImGui::Begin("Select Enemy", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
        
        ImGui::Text("Enemy Hierarchy:");
        ImGui::Separator();
        for (int e = 0; e < ENEMY_COUNT; ++e) {
            if (ImGui::Selectable(ENEMY_NAMES[e], activeEnemyIdx == e)) {
                activeEnemyIdx = e;
                safeWorldChange();
            }
        }

        ImGui::Separator();
        if (ImGui::Button("Reset Active Enemy", ImVec2(195.0f, 35.0f))) {
            spawnActiveEnemy();
        }

        ImGui::End();

        // =========================================================================
        // 2. RIGHT PANEL: TEST CONTROLS & GIZMOS
        // =========================================================================
        ImGui::SetNextWindowPos(ImVec2(980.0f, 0.0f));
        ImGui::SetNextWindowSize(ImVec2(300.0f, 720.0f));
        ImGui::Begin("Test Controls & Gizmos", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

        ImGui::Text("Environment Terrains:");
        if (ImGui::Checkbox("Flat Ground (y=544)", &enableFloor)) safeWorldChange();
        if (ImGui::Checkbox("Elevated Ledge", &enableLedge)) safeWorldChange();
        if (ImGui::Checkbox("Blocking Wall", &enableWall)) safeWorldChange();
        if (ImGui::Checkbox("Piranha Pipe", &enablePipe)) safeWorldChange();
        if (ImGui::Checkbox("Hammer Platform", &enableHammerPlatform)) safeWorldChange();
        if (ImGui::Checkbox("Bill Blaster Cannon", &enableBlaster)) safeWorldChange();

        ImGui::Separator();
        ImGui::Text("Visual Debug Gizmos:");
        ImGui::Checkbox("Show AABB Bounding Box", &showAABB);
        ImGui::Checkbox("Show Attack Radius", &showThreatRadius);
        ImGui::Checkbox("Show Velocity Arrow", &showVelocity);

        ImGui::Separator();
        ImGui::SliderFloat("Simulation Speed", &timeScale, 0.1f, 3.0f, "%.1fx");

        ImGui::Separator();
        ImGui::Text("Player Controls & Teleport:");
        ImGui::Text(" - WASD / Arrows: Move Player");
        ImGui::Text(" - Spacebar: Jump Player");
        ImGui::Text(" - F: Throw Held Koopa Shell (550 px/s, 45 deg)");
        ImGui::Text(" - Right Click: Instant Teleport");

        if (ImGui::Button("Under Thwomp")) { if (Player* p = getPlayer()) p->setPosition({544.0f, 512.0f}); }
        ImGui::SameLine();
        if (ImGui::Button("Facing Boo")) { if (Player* p = getPlayer()) { p->setPosition({544.0f, 250.0f}); p->setFacingRight(false); } }
        
        if (ImGui::Button("Behind Boo")) { if (Player* p = getPlayer()) { p->setPosition({350.0f, 250.0f}); p->setFacingRight(true); } }
        ImGui::SameLine();
        if (ImGui::Button("On Pipe")) { if (Player* p = getPlayer()) p->setPosition({224.0f, 416.0f}); }

        if (ImGui::Button("ChainChomp Zone")) { if (Player* p = getPlayer()) p->setPosition({750.0f, 512.0f}); }
        ImGui::SameLine();
        if (ImGui::Button("Hammer Platform")) { if (Player* p = getPlayer()) p->setPosition({900.0f, 384.0f}); }

        ImGui::Separator();
        ImGui::Text("Active Enemy Status:");
        if (Enemy* curE = getEnemy()) {
            if (curE->isActive()) {
                sf::Vector2f ePos = curE->getPosition();
                AABB b = curE->getBoundingBox();
                ImGui::Text("Type: %s", typeid(*curE).name());
                ImGui::Text("Pos: (%.0f, %.0f) - AABB: %.0fx%.0f", ePos.x, ePos.y, b.width, b.height);
            } else {
                ImGui::Text("Status: Defeated / Inactive");
            }
        } else {
            ImGui::Text("Status: None");
        }

        ImGui::End();

        // =========================================================================
        // 3. CENTER VISUAL CANVAS: TERRAINS, GIZMOS & ENTITIES
        // =========================================================================
        window.clear(sf::Color(35, 42, 50));

        // Render TileMap Tiles directly (100% sync with physics engine)
        for (int y = 0; y < 23; ++y) {
            for (int x = 0; x < 40; ++x) {
                TileType type = tileMap.getTileType(x, y);
                if (type == TileType::Empty) continue;
                sf::RectangleShape rect(sf::Vector2f(32.0f, 32.0f));
                rect.setPosition(sf::Vector2f(x * 32.0f, y * 32.0f));
                if (type == TileType::Ground) rect.setFillColor(sf::Color(90, 90, 95));
                else if (type == TileType::Brick) rect.setFillColor(sf::Color(140, 100, 60));
                else if (type == TileType::Pipe) rect.setFillColor(sf::Color(30, 160, 40));
                window.draw(rect);
            }
        }
        if (enableBlaster) {
            // Authentic bullet_bill_combined sprite at 1:2 ratio (32x64px)
            sf::Sprite blasterSprite = enemySheet.getSprite("bullet_bill_combined");
            sf::FloatRect bounds = blasterSprite.getLocalBounds();
            if (bounds.size.x > 0.0f && bounds.size.y > 0.0f) {
                blasterSprite.setScale(sf::Vector2f(2.0f, 2.0f));
                blasterSprite.setPosition(sf::Vector2f(96.0f, 480.0f));
                window.draw(blasterSprite);
            }
        }

        // Render Entities (Player, Selected Enemy, and any spawned Spinies)
        for (const auto& entity : entities) {
            if (!entity || !entity->isActive()) continue;

            entity->render(window);

            // AABB Outline
            if (showAABB) {
                AABB b = entity->getBoundingBox();
                sf::RectangleShape rect(sf::Vector2f(b.width, b.height));
                rect.setPosition(sf::Vector2f(b.x, b.y));
                rect.setFillColor(sf::Color::Transparent);
                rect.setOutlineColor(sf::Color::Green);
                rect.setOutlineThickness(1.0f);
                window.draw(rect);
            }

            // Velocity Vector Line
            if (showVelocity) {
                sf::Vector2f pos = entity->getPosition();
                sf::Vector2f vel = entity->getVelocity();
                if (std::abs(vel.x) > 1.0f || std::abs(vel.y) > 1.0f) {
                    sf::Vertex line[] = {
                        sf::Vertex{pos, sf::Color::Cyan},
                        sf::Vertex{pos + vel * 0.2f, sf::Color::Yellow}
                    };
                    window.draw(line, 2, sf::PrimitiveType::Lines);
                }
            }
        }

        // Render Threat Ranges for Active Enemy
        if (showThreatRadius) {
            Player* p = getPlayer();
            Enemy* curE = getEnemy();
            if (curE && p && curE->isActive()) {
                sf::Vector2f pPos = p->getPosition();
                sf::Vector2f eCenter = curE->getPosition() + sf::Vector2f(curE->getBoundingBox().width * 0.5f, curE->getBoundingBox().height * 0.5f);
                float radius = 0.0f;
                bool isTriggered = false;

                if (activeEnemyIdx == ENEMY_BOO) {
                    radius = 300.0f;
                    float dx = pPos.x - eCenter.x;
                    float dy = pPos.y - eCenter.y;
                    bool playerToRight = (pPos.x > eCenter.x);
                    bool playerFacingRight = p->isFacingRight();
                    bool isWatching = (playerToRight && !playerFacingRight) || (!playerToRight && playerFacingRight);
                    isTriggered = (std::sqrt(dx*dx + dy*dy) <= radius) && (!isWatching);
                } else if (activeEnemyIdx == ENEMY_THWOMP) {
                    float dx = std::abs(pPos.x - eCenter.x);
                    isTriggered = (dx <= 80.0f) && (pPos.y > eCenter.y);
                    sf::RectangleShape colBox(sf::Vector2f(160.0f, 380.0f));
                    colBox.setOrigin(sf::Vector2f(80.0f, 0.0f));
                    colBox.setPosition(sf::Vector2f(eCenter.x, eCenter.y));
                    colBox.setFillColor(isTriggered ? sf::Color(255, 50, 50, 40) : sf::Color(255, 220, 0, 20));
                    colBox.setOutlineColor(isTriggered ? sf::Color(255, 50, 50, 220) : sf::Color(255, 220, 0, 180));
                    colBox.setOutlineThickness(1.5f);
                    window.draw(colBox);
                } else if (activeEnemyIdx == ENEMY_CHAIN_CHOMP) {
                    radius = 180.0f;
                    float dx = pPos.x - eCenter.x;
                    float dy = pPos.y - eCenter.y;
                    isTriggered = (std::sqrt(dx*dx + dy*dy) <= radius);
                } else if (activeEnemyIdx == ENEMY_HAMMER_BRO) {
                    radius = 300.0f;
                    float dx = pPos.x - eCenter.x;
                    float dy = pPos.y - eCenter.y;
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
        }

        // Render Active Hammer Projectiles
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

            if (showAABB) {
                sf::RectangleShape rect(sf::Vector2f(16.0f, 16.0f));
                rect.setPosition(sf::Vector2f(h.pos.x - 8.0f, h.pos.y - 8.0f));
                rect.setFillColor(sf::Color::Transparent);
                rect.setOutlineColor(sf::Color::Red);
                rect.setOutlineThickness(1.0f);
                window.draw(rect);
            }
        }

        // Render Active Lakitu Eggs
        for (const auto& eg : activeLakituEggs) {
            if (!eg.hatched) {
                sf::Sprite eggSprite = enemySheet.getSprite("spiny_egg_0");
                eggSprite.setPosition(eg.pos);
                window.draw(eggSprite);
            }
        }

        // Draw Player Facing Line-of-Sight Ray (Magenta)
        if (Player* p = getPlayer()) {
            sf::Vector2f pPos = p->getPosition();
            sf::Vector2f rayDir = p->isFacingRight() ? sf::Vector2f(150.0f, 0.0f) : sf::Vector2f(-150.0f, 0.0f);
            sf::Vector2f rayStart = pPos + sf::Vector2f(p->getBoundingBox().width * 0.5f, p->getBoundingBox().height * 0.5f);
            sf::Vertex sightRay[] = {
                sf::Vertex{rayStart, sf::Color::Magenta},
                sf::Vertex{rayStart + rayDir, sf::Color::Magenta}
            };
            window.draw(sightRay, 2, sf::PrimitiveType::Lines);
        }

        ImGui::SFML::Render(window);
        window.display();
    }

    Game::getInstance().setPlayer(nullptr);
    Game::getInstance().setTileMap(nullptr);
    InputManager::getInstance().registerPlayer(nullptr, 0);
    ImGui::SFML::Shutdown();
    return 0;
}
