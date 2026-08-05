#include <iostream>
#include <filesystem>
#include <unordered_map>
#include <memory>
#include <string>
#include <vector>
#include <cmath>
#include <SFML/Graphics.hpp>
#include <imgui.h>
#include <imgui-SFML.h>

#include "Core/ResourceManager.hpp"
#include "Graphics/SpriteSheet.hpp"
#include "Graphics/Animation.hpp"
#include "Graphics/Animator.hpp"
#include "Utils/Constants.hpp"

// We define a simple visualizer category structure
struct EnemyCategory {
    std::string name;
    std::vector<std::string> subTypes;
    std::vector<std::pair<std::string, std::string>> actions; // {label, animKeySuffix}
};

int main() {
    std::cout << "[VISUAL TEST] Launching Enemy & Projectile Animation Visualizer..." << std::endl;

    sf::RenderWindow window(sf::VideoMode(sf::Vector2u(1280, 720)), "Enemy & Projectile Animation Visualizer");
    window.setFramerateLimit(60);

    if (!ImGui::SFML::Init(window)) {
        std::cerr << "[VISUAL TEST] Failed to initialize ImGui-SFML!" << std::endl;
        return 1;
    }

    // Resolve resource paths
    ResourceManager& rm = ResourceManager::getInstance();
    std::string pngPath = "assets/spriteSheet/enemy_projectile/enemy_projectile.png";
    std::string jsonPath = "assets/spriteSheet/enemy_projectile/enemy_projectile.json";
    
    if (!std::filesystem::exists(pngPath)) pngPath = "../assets/spriteSheet/enemy_projectile/enemy_projectile.png";
    if (!std::filesystem::exists(pngPath)) pngPath = "../../assets/spriteSheet/enemy_projectile/enemy_projectile.png";
    if (!std::filesystem::exists(pngPath)) pngPath = "SuperMarioGame/assets/spriteSheet/enemy_projectile/enemy_projectile.png";

    if (!std::filesystem::exists(jsonPath)) jsonPath = "../assets/spriteSheet/enemy_projectile/enemy_projectile.json";
    if (!std::filesystem::exists(jsonPath)) jsonPath = "../../assets/spriteSheet/enemy_projectile/enemy_projectile.json";
    if (!std::filesystem::exists(jsonPath)) jsonPath = "SuperMarioGame/assets/spriteSheet/enemy_projectile/enemy_projectile.json";

    if (!rm.loadTexture("enemyTexture", pngPath)) {
        std::cerr << "[VISUAL TEST] Failed to load enemy texture from " << pngPath << std::endl;
        return 1;
    }

    SpriteSheet enemySheet("enemyTexture", jsonPath);
    Animator animator(&enemySheet);

    // Build Animations Dictionary
    std::unordered_map<std::string, Animation> anims;

    // 1. Goombas
    for (std::string color : {"brown", "blue", "grey"}) {
        anims["goomba_" + color + "_move"] = Animation("goomba_" + color + "_move");
        anims["goomba_" + color + "_move"].frameList = {
            {"goomba_" + color + "_move_0", 0.15f},
            {"goomba_" + color + "_move_1", 0.15f}
        };
        anims["goomba_" + color + "_squished"] = Animation("goomba_" + color + "_squished");
        anims["goomba_" + color + "_squished"].frameList = {
            {"goomba_" + color + "_squished", 0.15f}
        };
    }

    // 2. Koopas & Paratroopas
    for (std::string color : {"green", "red", "blue"}) {
        for (std::string dir : {"left", "right"}) {
            anims["koopa_" + color + "_move_" + dir] = Animation("koopa_" + color + "_move_" + dir);
            anims["koopa_" + color + "_move_" + dir].frameList = {
                {"koopa_" + color + "_move_" + dir + "_0", 0.15f},
                {"koopa_" + color + "_move_" + dir + "_1", 0.15f}
            };
            anims["koopa_" + color + "_fly_" + dir] = Animation("koopa_" + color + "_fly_" + dir);
            anims["koopa_" + color + "_fly_" + dir].frameList = {
                {"koopa_" + color + "_fly_" + dir + "_0", 0.15f},
                {"koopa_" + color + "_fly_" + dir + "_1", 0.15f}
            };
        }
        anims["koopa_" + color + "_shell"] = Animation("koopa_" + color + "_shell");
        anims["koopa_" + color + "_shell"].frameList = {
            {"koopa_" + color + "_shell", 0.15f}
        };
        anims["koopa_" + color + "_shell_leg_popout"] = Animation("koopa_" + color + "_shell_leg_popout");
        anims["koopa_" + color + "_shell_leg_popout"].frameList = {
            {"koopa_" + color + "_shell_leg_popout", 0.15f}
        };
    }

    // 3. Beetles
    for (std::string color : {"black", "blue", "grey"}) {
        for (std::string dir : {"left", "right"}) {
            anims["beetle_" + color + "_move_" + dir] = Animation("beetle_" + color + "_move_" + dir);
            anims["beetle_" + color + "_move_" + dir].frameList = {
                {"beetle_" + color + "_move_" + dir + "_0", 0.15f},
                {"beetle_" + color + "_move_" + dir + "_1", 0.15f}
            };
        }
        std::string shellKey = (color == "black") ? "beetle_black_hide" : "beetle_" + color + "_shell";
        anims["beetle_" + color + "_shell"] = Animation("beetle_" + color + "_shell");
        anims["beetle_" + color + "_shell"].frameList = {
            {shellKey, 0.15f}
        };
    }

    // 4. Boos
    anims["boo_move"] = Animation("boo_move");
    anims["boo_move"].frameList = {{"boo_move_0", 0.15f}, {"boo_move_1", 0.15f}};
    anims["boo_attack"] = Animation("boo_attack");
    anims["boo_attack"].frameList = {{"boo_attack_0", 0.15f}, {"boo_attack_1", 0.15f}};
    anims["boo_seen"] = Animation("boo_seen");
    anims["boo_seen"].frameList = {{"boo_seen_0", 0.15f}, {"boo_seen_1", 0.15f}};
    anims["boo_funny"] = Animation("boo_funny");
    anims["boo_funny"].frameList = {{"boo_funny_0", 0.15f}, {"boo_funny_1", 0.15f}};

    // 5. Thwomps
    anims["thwomp_dormant"] = Animation("thwomp_dormant");
    anims["thwomp_dormant"].frameList = {{"thwomper_dormant", 0.15f}};
    anims["thwomp_active"] = Animation("thwomp_active");
    anims["thwomp_active"].frameList = {{"thwomper_active", 0.15f}};

    // 6. Chain Chomps
    for (std::string dir : {"left", "right"}) {
        anims["chomp_head_" + dir] = Animation("chomp_head_" + dir);
        anims["chomp_head_" + dir].frameList = {
            {"chained_chomp_head_" + dir + "_0", 0.15f},
            {"chained_chomp_head_" + dir + "_1", 0.15f}
        };
    }
    anims["chomp_chain"] = Animation("chomp_chain");
    anims["chomp_chain"].frameList = {{"chained_chomp_chain", 0.15f}};
    anims["chomp_chain_squished"] = Animation("chomp_chain_squished");
    anims["chomp_chain_squished"].frameList = {{"chained_chomp_chain_squished", 0.15f}};

    // 7. Piranha Plants
    for (std::string color : {"green", "blue"}) {
        anims["piranha_" + color] = Animation("piranha_" + color);
        anims["piranha_" + color].frameList = {
            {"pirhana_" + color + "_0", 0.15f},
            {"pirhana_" + color + "_1", 0.15f}
        };
    }

    // 8. Lakitus
    anims["lakitu_left"] = Animation("lakitu_left");
    anims["lakitu_left"].frameList = {{"lakitu_left", 0.15f}};
    anims["lakitu_right"] = Animation("lakitu_right");
    anims["lakitu_right"].frameList = {{"lakitu_right", 0.15f}};
    anims["lakitu_hide"] = Animation("lakitu_hide");
    anims["lakitu_hide"].frameList = {{"lakitu_hide", 0.15f}};

    // 9. Spinies
    for (std::string dir : {"left", "right"}) {
        anims["spiny_move_" + dir] = Animation("spiny_move_" + dir);
        anims["spiny_move_" + dir].frameList = {
            {"spiny_move_" + dir + "_0", 0.15f},
            {"spiny_move_" + dir + "_1", 0.15f}
        };
    }
    anims["spiny_ball"] = Animation("spiny_ball");
    anims["spiny_ball"].frameList = {{"spiny_ball_0", 0.12f}, {"spiny_ball_1", 0.12f}};

    // 10. Cheep Cheeps
    for (std::string color : {"red", "green", "grey"}) {
        for (std::string dir : {"left", "right"}) {
            anims["cheep_" + color + "_" + dir] = Animation("cheep_" + color + "_" + dir);
            anims["cheep_" + color + "_" + dir].frameList = {
                {"cheep_cheep_" + color + "_move_" + dir + "_0", 0.15f},
                {"cheep_cheep_" + color + "_move_" + dir + "_1", 0.15f}
            };
        }
    }

    // 11. Blooper
    anims["blooper_move"] = Animation("blooper_move");
    anims["blooper_move"].frameList = {{"squid_move_0", 0.15f}, {"squid_move_1", 0.15f}};

    // 12. Bowser
    for (std::string dir : {"left", "right"}) {
        anims["bowser_move_" + dir] = Animation("bowser_move_" + dir);
        anims["bowser_move_" + dir].frameList = {
            {"bowser_move_" + dir + "_0", 0.15f},
            {"bowser_move_" + dir + "_1", 0.15f},
            {"bowser_move_" + dir + "_2", 0.15f},
            {"bowser_move_" + dir + "_3", 0.15f}
        };
        anims["bowser_fire_" + dir] = Animation("bowser_fire_" + dir);
        anims["bowser_fire_" + dir].frameList = {
            {"bowser_fire_" + dir + "_0", 0.15f},
            {"bowser_fire_" + dir + "_1", 0.15f}
        };
    }

    // 13. Projectiles
    anims["flower_fireball"] = Animation("flower_fireball");
    anims["flower_fireball"].frameList = {
        {"flower_fireball_0", 0.08f}, {"flower_fireball_1", 0.08f},
        {"flower_fireball_2", 0.08f}, {"flower_fireball_3", 0.08f}
    };
    anims["flower_fireball_hit"] = Animation("flower_fireball_hit");
    anims["flower_fireball_hit"].frameList = {
        {"flower_fireball_hit_0", 0.08f},
        {"flower_fireball_hit_1", 0.08f},
        {"flower_fireball_hit_2", 0.08f}
    };
    anims["hammer_black"] = Animation("hammer_black");
    anims["hammer_black"].frameList = {
        {"hammer_black_0", 0.08f}, {"hammer_black_1", 0.08f},
        {"hammer_black_2", 0.08f}, {"hammer_black_3", 0.08f}
    };
    anims["hammer_grey"] = Animation("hammer_grey");
    anims["hammer_grey"].frameList = {
        {"hammer_grey_0", 0.08f}, {"hammer_grey_1", 0.08f},
        {"hammer_grey_2", 0.08f}, {"hammer_grey_3", 0.08f}
    };
    anims["lava_fireball_up"] = Animation("lava_fireball_up");
    anims["lava_fireball_up"].frameList = {{"lava_fireball_up", 0.15f}};
    anims["lava_fireball_down"] = Animation("lava_fireball_down");
    anims["lava_fireball_down"].frameList = {{"lava_fireball_down", 0.15f}};

    // 14. Bullet Bill & Blasters
    anims["bullet_bill_left"] = Animation("bullet_bill_left");
    anims["bullet_bill_left"].frameList = {{"bullet_bill_bullet_left", 0.15f}};
    anims["bullet_bill_right"] = Animation("bullet_bill_right");
    anims["bullet_bill_right"].frameList = {{"bullet_bill_bullet_right", 0.15f}};
    anims["bullet_bill_grey"] = Animation("bullet_bill_grey");
    anims["bullet_bill_grey"].frameList = {{"bullet_bill_grey", 0.15f}};
    anims["blaster_body"] = Animation("blaster_body");
    anims["blaster_body"].frameList = {{"bullet_bill_body", 0.15f}};
    anims["blaster_combined"] = Animation("blaster_combined");
    anims["blaster_combined"].frameList = {{"bullet_bill_combined", 0.15f}};
    anims["blaster_head"] = Animation("blaster_head");
    anims["blaster_head"].frameList = {{"bullet_bill_head", 0.15f}};
    anims["blaster_neck"] = Animation("blaster_neck");
    anims["blaster_neck"].frameList = {{"bullet_bill_neck", 0.15f}};

    // Categories definition
    std::vector<EnemyCategory> categories = {
        {
            "Goombas",
            {"brown", "blue", "grey"},
            {{"Walk cycle", "move"}, {"Squished", "squished"}}
        },
        {
            "Koopas & Paratroopas",
            {"green", "red", "blue"},
            {
                {"Walk Left", "move_left"},
                {"Walk Right", "move_right"},
                {"Fly Left", "fly_left"},
                {"Fly Right", "fly_right"},
                {"Shell Idle", "shell"},
                {"Shell Leg Popout", "shell_leg_popout"}
            }
        },
        {
            "Buzzy Beetles",
            {"black", "blue", "grey"},
            {
                {"Walk Left", "move_left"},
                {"Walk Right", "move_right"},
                {"Shell / Hide", "shell"}
            }
        },
        {
            "Spinies & Plants",
            {"green", "blue"}, // custom subtypes for plants/spinies
            {
                {"Piranha Green/Blue", "piranha"},
                {"Spiny Walk Left", "spiny_move_left"},
                {"Spiny Walk Right", "spiny_move_right"},
                {"Spiny Ball", "spiny_ball"}
            }
        },
        {
            "Aquatic (Cheeps & Blooper)",
            {"red", "green", "grey"},
            {
                {"Cheep Swim Left", "cheep_left"},
                {"Cheep Swim Right", "cheep_right"},
                {"Blooper swim cycle", "blooper_move"}
            }
        },
        {
            "Bosses & Hazards",
            {"standard"},
            {
                {"Bowser Move Left", "bowser_move_left"},
                {"Bowser Move Right", "bowser_move_right"},
                {"Bowser Fire Left", "bowser_fire_left"},
                {"Bowser Fire Right", "bowser_fire_right"},
                {"Boo Fly", "boo_move"},
                {"Boo Attack", "boo_attack"},
                {"Boo Seen", "boo_seen"},
                {"Boo Laugh/Funny", "boo_funny"},
                {"Thwomp Dormant", "thwomp_dormant"},
                {"Thwomp Slamming", "thwomp_active"},
                {"Chain Chomp Left", "chomp_head_left"},
                {"Chain Chomp Right", "chomp_head_right"},
                {"Chomp Chain Link", "chomp_chain"},
                {"Chomp Squished Link", "chomp_chain_squished"},
                {"Lakitu Left", "lakitu_left"},
                {"Lakitu Right", "lakitu_right"},
                {"Lakitu Hide", "lakitu_hide"}
            }
        },
        {
            "Projectiles & Blasters",
            {"standard"},
            {
                {"Flower Fireball", "flower_fireball"},
                {"Fireball Explosion", "flower_fireball_hit"},
                {"Hammer Black", "hammer_black"},
                {"Hammer Grey", "hammer_grey"},
                {"Lava Bubble UP", "lava_fireball_up"},
                {"Lava Bubble DOWN", "lava_fireball_down"},
                {"Bullet Bill Left", "bullet_bill_left"},
                {"Bullet Bill Right", "bullet_bill_right"},
                {"Bullet Bill Grey", "bullet_bill_grey"},
                {"Blaster Body", "blaster_body"},
                {"Blaster Neck", "blaster_neck"},
                {"Blaster Head", "blaster_head"},
                {"Blaster Combined", "blaster_combined"}
            }
        }
    };

    // State indicators
    int activeCatIdx = 0;
    int activeSubIdx = 0;
    int activeActIdx = 0;

    float renderScale = 4.0f;
    bool showBBox = true;
    bool flipX = false;
    sf::Vector2f renderPos(640.f, 360.f);

    sf::Clock deltaClock;
    while (window.isOpen()) {
        while (const auto event = window.pollEvent()) {
            ImGui::SFML::ProcessEvent(window, *event);
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
        }

        float dt = deltaClock.restart().asSeconds();
        ImGui::SFML::Update(window, sf::seconds(dt));

        // Category / Subtype selection
        const auto& cat = categories[activeCatIdx];
        std::string activeSub = cat.subTypes[activeSubIdx];
        std::string actionSuff = cat.actions[activeActIdx].second;

        // Construct Animation Key
        std::string animKey = "";
        if (cat.name == "Goombas") {
            animKey = "goomba_" + activeSub + "_" + actionSuff;
        } else if (cat.name == "Koopas & Paratroopas") {
            if (actionSuff == "shell" || actionSuff == "shell_leg_popout") {
                animKey = "koopa_" + activeSub + "_" + actionSuff;
            } else {
                animKey = "koopa_" + activeSub + "_" + actionSuff;
            }
        } else if (cat.name == "Buzzy Beetles") {
            if (actionSuff == "shell") {
                animKey = "beetle_" + activeSub + "_shell";
            } else {
                animKey = "beetle_" + activeSub + "_" + actionSuff;
            }
        } else if (cat.name == "Spinies & Plants") {
            if (actionSuff == "piranha") {
                animKey = "piranha_" + activeSub; // green or blue
            } else {
                animKey = actionSuff; // e.g. spiny_ball or spiny_move_left
            }
        } else if (cat.name == "Aquatic (Cheeps & Blooper)") {
            if (actionSuff == "blooper_move") {
                animKey = "blooper_move";
            } else {
                animKey = "cheep_" + activeSub + "_" + (actionSuff == "cheep_left" ? "left" : "right");
            }
        } else {
            // Hazards, Bosses & Projectiles
            animKey = actionSuff;
        }

        // Fallback checks
        if (anims.find(animKey) == anims.end()) {
            animKey = "goomba_brown_move";
        }

        // Play active animation
        animator.play(&anims[animKey]);
        animator.update(dt);

        // GUI Window
        ImGui::Begin("Enemy & Projectile Animation Tester");
        ImGui::Text("Browse and verify all SMB1 enemy assets:");
        ImGui::Separator();

        // 1. Category Selection dropdown
        if (ImGui::BeginCombo("Category", cat.name.c_str())) {
            for (size_t i = 0; i < categories.size(); ++i) {
                bool isSel = (activeCatIdx == static_cast<int>(i));
                if (ImGui::Selectable(categories[i].name.c_str(), isSel)) {
                    activeCatIdx = static_cast<int>(i);
                    activeSubIdx = 0;
                    activeActIdx = 0;
                }
            }
            ImGui::EndCombo();
        }

        // 2. Subtype Selection dropdown
        const auto& activeCat = categories[activeCatIdx];
        if (activeCat.subTypes.size() > 1 || activeCat.subTypes[0] != "standard") {
            if (ImGui::BeginCombo("Variant / Palette", activeSub.c_str())) {
                for (size_t i = 0; i < activeCat.subTypes.size(); ++i) {
                    bool isSel = (activeSubIdx == static_cast<int>(i));
                    if (ImGui::Selectable(activeCat.subTypes[i].c_str(), isSel)) {
                        activeSubIdx = static_cast<int>(i);
                    }
                }
                ImGui::EndCombo();
            }
        }

        // 3. Action Selection dropdown
        if (ImGui::BeginCombo("State / Action", activeCat.actions[activeActIdx].first.c_str())) {
            for (size_t i = 0; i < activeCat.actions.size(); ++i) {
                bool isSel = (activeActIdx == static_cast<int>(i));
                if (ImGui::Selectable(activeCat.actions[i].first.c_str(), isSel)) {
                    activeActIdx = static_cast<int>(i);
                }
            }
            ImGui::EndCombo();
        }

        ImGui::Separator();
        ImGui::SliderFloat("Scale", &renderScale, 1.f, 8.f, "%.1fx");
        ImGui::Checkbox("Show Bounding Box Overlay", &showBBox);
        ImGui::Checkbox("Flip Horizontally (Manual Override)", &flipX);

        ImGui::Separator();
        ImGui::Text("Active Frame: %s", animKey.c_str());
        sf::FloatRect bounds = animator.getSprite().getLocalBounds();
        ImGui::Text("Sprite size: %.1f x %.1f px", bounds.size.x, bounds.size.y);

        ImGui::End();

        // Clear window
        window.clear(sf::Color(50, 60, 70));

        // Draw active sprite
        sf::Sprite sprite = animator.getSprite();
        sprite.setOrigin(sf::Vector2f(bounds.size.x * 0.5f, bounds.size.y * 0.5f));
        sprite.setPosition(renderPos);
        sprite.setScale(sf::Vector2f(flipX ? -renderScale : renderScale, renderScale));
        window.draw(sprite);

        // Draw bounding box
        if (showBBox) {
            sf::RectangleShape bbox(sf::Vector2f(bounds.size.x * renderScale, bounds.size.y * renderScale));
            bbox.setOrigin(sf::Vector2f(bounds.size.x * renderScale * 0.5f, bounds.size.y * renderScale * 0.5f));
            bbox.setPosition(renderPos);
            bbox.setFillColor(sf::Color::Transparent);
            bbox.setOutlineColor(sf::Color::Yellow);
            bbox.setOutlineThickness(1.5f);
            window.draw(bbox);
        }

        ImGui::SFML::Render(window);
        window.display();
    }

    ImGui::SFML::Shutdown();
    return 0;
}
