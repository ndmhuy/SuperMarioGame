#include <iostream>
#include <filesystem>
#include <vector>
#include <unordered_map>
#include <memory>
#include <string>
#include <SFML/Graphics.hpp>
#include <imgui.h>
#include <imgui-SFML.h>

#include "Core/ResourceManager.hpp"
#include "Graphics/AnimationManager.hpp"
#include "Graphics/EntityDeathEffect.hpp"
#include "Graphics/ParticleSystem.hpp"
#include "Utils/Constants.hpp"

// Players
#include "Entities/Mario.hpp"
#include "Entities/Luigi.hpp"
#include "Entities/Toad.hpp"
#include "Entities/Peach.hpp"

// Enemies
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

// Items
#include "Entities/Mushroom.hpp"
#include "Entities/FireFlower.hpp"
#include "Entities/Star.hpp"
#include "Entities/Coin.hpp"
#include "Entities/OneUpMushroom.hpp"
#include "Entities/CapeFeather.hpp"
#include "Entities/MegaMushroom.hpp"
#include "Entities/MiniMushroom.hpp"
#include "Entities/POWBlock.hpp"
#include "Entities/PSwitch.hpp"
#include "Entities/Trampoline.hpp"
#include "Entities/StarCoin.hpp"

// Blocks
#include "Entities/QuestionBlock.hpp"
#include "Entities/BrickBlock.hpp"
#include "Entities/Pipe.hpp"
#include "Entities/Flagpole.hpp"
#include "Entities/HiddenBlock.hpp"
#include "Entities/MovingPlatform.hpp"
#include "Entities/FallingPlatform.hpp"
#include "Entities/IceBlock.hpp"
#include "Entities/ConveyorBelt.hpp"

static void registerAnimations() {
    AnimationManager& am = AnimationManager::getInstance();
    std::vector<std::string> characters = {"mario", "luigi", "toad", "peach"};

    for (const auto& ch : characters) {
        Animation smallIdle(ch + "_small_idle");
        smallIdle.frameList = {{ch + "_small_idle", 0.15f}};
        am.addAnimation(smallIdle);

        Animation smallWalk(ch + "_small_walk");
        smallWalk.frameList = {{ch + "_small_walk_0", 0.15f}, {ch + "_small_walk_1", 0.15f}};
        am.addAnimation(smallWalk);

        Animation smallRun(ch + "_small_run");
        smallRun.frameList = {{ch + "_small_run_0", 0.10f}, {ch + "_small_run_1", 0.10f}};
        am.addAnimation(smallRun);
    }

    for (std::string color : {"brown", "red", "blue", "grey"}) {
        Animation gMove("goomba_" + color + "_move");
        gMove.frameList = {{"goomba_" + color + "_move_0", 0.15f}, {"goomba_" + color + "_move_1", 0.15f}};
        am.addAnimation(gMove);
    }
}

int main() {
    std::cout << "[TEST] Launching All Entities Visual Test Harness..." << std::endl;

    sf::RenderWindow window(sf::VideoMode(sf::Vector2u(1280, 720)), "All Entities Full Render Test Harness");
    window.setFramerateLimit(60);

    if (!ImGui::SFML::Init(window)) {
        std::cerr << "[TEST] Failed to initialize ImGui-SFML!" << std::endl;
        return 1;
    }

    // Load textures & sprite sheets
    ResourceManager& rm = ResourceManager::getInstance();
    rm.loadTexture("player", "assets/spriteSheet/player/player.png");
    rm.loadTexture("item", "assets/spriteSheet/item/item.png");
    rm.loadTexture("enemy_projectile", "assets/spriteSheet/enemy_projectile/enemy_projectile.png");
    rm.loadTexture("world_scenery_item", "assets/spriteSheet/world_scenery_item/world_scenery_item.png");
    rm.loadTexture("particles", "assets/spriteSheet/particles/particles.png");

    SpriteSheet playerSheet("player", "assets/spriteSheet/player/player.json");
    SpriteSheet itemSheet("item", "assets/spriteSheet/item/item.json");
    SpriteSheet enemySheet("enemy_projectile", "assets/spriteSheet/enemy_projectile/enemy_projectile.json");
    SpriteSheet scenerySheet("world_scenery_item", "assets/spriteSheet/world_scenery_item/world_scenery_item.json");

    registerAnimations();

    // Create entity containers
    std::vector<std::pair<std::string, std::unique_ptr<Player>>> players;
    players.push_back({"Mario", std::make_unique<Mario>(sf::Vector2f(100, 200))});
    players.push_back({"Luigi", std::make_unique<Luigi>(sf::Vector2f(250, 200))});
    players.push_back({"Toad", std::make_unique<Toad>(sf::Vector2f(400, 200))});
    players.push_back({"Peach", std::make_unique<Peach>(sf::Vector2f(550, 200))});

    for (auto& p : players) {
        p.second->setupAnimations(&playerSheet);
    }

    std::vector<std::pair<std::string, std::unique_ptr<Enemy>>> enemies;
    enemies.push_back({"Goomba", std::make_unique<Goomba>(sf::Vector2f(100, 200))});
    enemies.push_back({"KoopaTroopa", std::make_unique<KoopaTroopa>(sf::Vector2f(200, 200))});
    enemies.push_back({"Boo", std::make_unique<Boo>(sf::Vector2f(300, 200))});
    enemies.push_back({"KoopaParatroopa", std::make_unique<KoopaParatroopa>(sf::Vector2f(400, 180))});
    enemies.push_back({"PiranhaPlant", std::make_unique<PiranhaPlant>(sf::Vector2f(550, 200))});
    enemies.push_back({"BulletBill", std::make_unique<BulletBill>(sf::Vector2f(650, 200))});
    enemies.push_back({"HammerBro", std::make_unique<HammerBro>(sf::Vector2f(750, 200))});
    enemies.push_back({"Thwomp", std::make_unique<Thwomp>(sf::Vector2f(850, 180))});
    enemies.push_back({"ChainChomp", std::make_unique<ChainChomp>(sf::Vector2f(950, 200))});
    enemies.push_back({"Lakitu", std::make_unique<Lakitu>(sf::Vector2f(1050, 150))});
    enemies.push_back({"Spiny", std::make_unique<Spiny>(sf::Vector2f(1150, 200))});

    for (auto& e : enemies) {
        e.second->setupAnimations(&enemySheet);
    }

    std::vector<std::pair<std::string, std::unique_ptr<Item>>> items;
    items.push_back({"Mushroom", std::make_unique<Mushroom>(sf::Vector2f(100, 200))});
    items.push_back({"FireFlower", std::make_unique<FireFlower>(sf::Vector2f(180, 200))});
    items.push_back({"Star", std::make_unique<Star>(sf::Vector2f(260, 200))});
    items.push_back({"Coin", std::make_unique<Coin>(sf::Vector2f(340, 200))});
    items.push_back({"1Up Mushroom", std::make_unique<OneUpMushroom>(sf::Vector2f(420, 200))});
    items.push_back({"Cape Feather", std::make_unique<CapeFeather>(sf::Vector2f(500, 200))});
    items.push_back({"Mega Mushroom", std::make_unique<MegaMushroom>(sf::Vector2f(580, 200))});
    items.push_back({"Mini Mushroom", std::make_unique<MiniMushroom>(sf::Vector2f(660, 200))});
    items.push_back({"POW Block", std::make_unique<POWBlock>(sf::Vector2f(740, 200))});
    items.push_back({"P-Switch", std::make_unique<PSwitch>(sf::Vector2f(820, 200))});
    items.push_back({"Trampoline", std::make_unique<Trampoline>(sf::Vector2f(900, 200))});
    items.push_back({"Star Coin", std::make_unique<StarCoin>(sf::Vector2f(980, 200))});

    for (auto& i : items) {
        i.second->setupAnimations(&itemSheet);
    }

    std::vector<std::pair<std::string, std::unique_ptr<Block>>> blocks;
    blocks.push_back({"Question Block", std::make_unique<QuestionBlock>(sf::Vector2f(100, 200))});
    blocks.push_back({"Brick Block", std::make_unique<BrickBlock>(sf::Vector2f(200, 200))});
    blocks.push_back({"Pipe", std::make_unique<Pipe>(sf::Vector2f(300, 200))});
    blocks.push_back({"Flagpole", std::make_unique<Flagpole>(sf::Vector2f(420, 100))});
    blocks.push_back({"Hidden Block", std::make_unique<HiddenBlock>(sf::Vector2f(500, 200))});
    blocks.push_back({"Moving Platform", std::make_unique<MovingPlatform>(sf::Vector2f(600, 200), sf::Vector2f(700, 200), 100.0f)});
    blocks.push_back({"Falling Platform", std::make_unique<FallingPlatform>(sf::Vector2f(750, 200))});
    blocks.push_back({"Ice Block", std::make_unique<IceBlock>(sf::Vector2f(870, 200))});
    blocks.push_back({"Conveyor Belt", std::make_unique<ConveyorBelt>(sf::Vector2f(970, 200))});

    for (auto& b : blocks) {
        if (b.first == "Moving Platform" || b.first == "Falling Platform") {
            b.second->setupAnimations(&itemSheet);
        } else {
            b.second->setupAnimations(&scenerySheet);
        }
    }

    std::unordered_map<Entity*, sf::Vector2f> baseSizes;
    for (auto& p : players) baseSizes[p.second.get()] = p.second->getTargetSize();
    for (auto& e : enemies) baseSizes[e.second.get()] = e.second->getTargetSize();
    for (auto& i : items) baseSizes[i.second.get()] = i.second->getTargetSize();
    for (auto& b : blocks) baseSizes[b.second.get()] = b.second->getTargetSize();

    sf::Clock deltaClock;
    bool showAABB = true;
    float entityScale = 1.0f;
    int currentTab = 0;

    while (window.isOpen()) {
        while (const auto event = window.pollEvent()) {
            ImGui::SFML::ProcessEvent(window, *event);
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
        }

        sf::Time dtTime = deltaClock.restart();
        float dt = dtTime.asSeconds();

        ImGui::SFML::Update(window, dtTime);

        // Update all active entities
        for (auto& p : players) p.second->update(dt);
        for (auto& e : enemies) e.second->update(dt);
        for (auto& i : items) i.second->update(dt);
        for (auto& b : blocks) b.second->update(dt);

        EntityDeathEffect::getInstance().update(dt, 720.0f);
        ParticleSystem::getInstance().update(dt);

        // ImGui Control Panel
        ImGui::Begin("Full Entity Rendering Test Harness");
        ImGui::Checkbox("Show AABB Bounding Box Overlay", &showAABB);
        if (ImGui::SliderFloat("Entity Scale Factor", &entityScale, 0.25f, 4.0f, "%.2fx")) {
            for (auto& p : players) p.second->setTargetSize(baseSizes[p.second.get()] * entityScale);
            for (auto& e : enemies) e.second->setTargetSize(baseSizes[e.second.get()] * entityScale);
            for (auto& i : items) i.second->setTargetSize(baseSizes[i.second.get()] * entityScale);
            for (auto& b : blocks) b.second->setTargetSize(baseSizes[b.second.get()] * entityScale);
        }
        ImGui::Separator();

        if (ImGui::Button("Players (4)")) currentTab = 0;
        ImGui::SameLine();
        if (ImGui::Button("Enemies (11)")) currentTab = 1;
        ImGui::SameLine();
        if (ImGui::Button("Items (12)")) currentTab = 2;
        ImGui::SameLine();
        if (ImGui::Button("Blocks (9)")) currentTab = 3;

        ImGui::Separator();

        if (currentTab == 0) {
            ImGui::Text("Active Tab: Players (Full Object Render)");
            for (auto& p : players) {
                if (ImGui::Button(("Bounce Trampoline on " + p.first).c_str())) {
                    items[10].second->activate(*p.second);
                }
            }
        } else if (currentTab == 1) {
            ImGui::Text("Active Tab: Enemies (Full Object Render)");
            for (auto& e : enemies) {
                if (ImGui::Button(("Fireball Hit " + e.first).c_str())) {
                    e.second->onHitByFireball();
                }
            }
        } else if (currentTab == 2) {
            ImGui::Text("Active Tab: Items (Full Object Render)");
            if (ImGui::Button("Bounce Trampoline")) {
                items[10].second->activate(*players[0].second);
            }
        } else if (currentTab == 3) {
            ImGui::Text("Active Tab: Blocks (Full Object Render)");
            for (auto& b : blocks) {
                if (ImGui::Button(("Bump " + b.first).c_str())) {
                    b.second->onHitFromBelow(*players[0].second);
                }
            }
        }

        ImGui::End();

        window.clear(sf::Color(50, 60, 80));

        // Render current active tab entities
        if (currentTab == 0) {
            for (auto& p : players) {
                p.second->render(window);
                if (showAABB) {
                    sf::RectangleShape box(sf::Vector2f(p.second->getBoundingBox().width, p.second->getBoundingBox().height));
                    box.setPosition(sf::Vector2f(p.second->getBoundingBox().x, p.second->getBoundingBox().y));
                    box.setFillColor(sf::Color::Transparent);
                    box.setOutlineColor(sf::Color::Green);
                    box.setOutlineThickness(1.0f);
                    window.draw(box);
                }
            }
        } else if (currentTab == 1) {
            for (auto& e : enemies) {
                e.second->render(window);
                if (showAABB) {
                    sf::RectangleShape box(sf::Vector2f(e.second->getBoundingBox().width, e.second->getBoundingBox().height));
                    box.setPosition(sf::Vector2f(e.second->getBoundingBox().x, e.second->getBoundingBox().y));
                    box.setFillColor(sf::Color::Transparent);
                    box.setOutlineColor(sf::Color::Red);
                    box.setOutlineThickness(1.0f);
                    window.draw(box);
                }
            }
        } else if (currentTab == 2) {
            for (auto& i : items) {
                i.second->render(window);
                if (showAABB) {
                    sf::RectangleShape box(sf::Vector2f(i.second->getBoundingBox().width, i.second->getBoundingBox().height));
                    box.setPosition(sf::Vector2f(i.second->getBoundingBox().x, i.second->getBoundingBox().y));
                    box.setFillColor(sf::Color::Transparent);
                    box.setOutlineColor(sf::Color::Yellow);
                    box.setOutlineThickness(1.0f);
                    window.draw(box);
                }
            }
        } else if (currentTab == 3) {
            for (auto& b : blocks) {
                b.second->render(window);
                if (showAABB) {
                    sf::RectangleShape box(sf::Vector2f(b.second->getBoundingBox().width, b.second->getBoundingBox().height));
                    box.setPosition(sf::Vector2f(b.second->getBoundingBox().x, b.second->getBoundingBox().y));
                    box.setFillColor(sf::Color::Transparent);
                    box.setOutlineColor(sf::Color::Cyan);
                    box.setOutlineThickness(1.0f);
                    window.draw(box);
                }
            }
        }

        EntityDeathEffect::getInstance().render(window);
        window.draw(ParticleSystem::getInstance());

        ImGui::SFML::Render(window);
        window.display();
    }

    ImGui::SFML::Shutdown();
    return 0;
}
