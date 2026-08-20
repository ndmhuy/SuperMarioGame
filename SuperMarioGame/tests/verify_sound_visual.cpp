#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <imgui.h>
#include <imgui-SFML.h>
#include <algorithm>
#include <cctype>
#include <iostream>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include "Core/ResourceManager.hpp"
#include "Core/SoundManager.hpp"

struct SoundEntry {
    std::string id;          // Sound ID for SoundManager
    std::string filepath;    // Path relative to execution working directory
    std::string category;    // Category tab name
    std::string description; // Friendly description
};

int main() {
    sf::RenderWindow window(sf::VideoMode({960, 680}), "Super Mario Sound Verification Suite");
    window.setFramerateLimit(60);

    if (!ImGui::SFML::Init(window)) {
        std::cerr << "Failed to initialize ImGui-SFML" << std::endl;
        return -1;
    }

    sf::Clock deltaClock;

    // Comprehensive list of all game sound effects
    std::vector<SoundEntry> sounds = {
        // Movement & Actions
        {"jump", "assets/sfx/jump_small.wav", "Movement", "Small Mario Jump"},
        {"jump_super", "assets/sfx/jump_super.wav", "Movement", "Super/Fire Mario Jump"},
        {"stomp", "assets/sfx/stomp.wav", "Movement", "Stomp Enemy"},
        {"kick", "assets/sfx/kick.wav", "Movement", "Kick Shell / Object"},
        {"fireball", "assets/sfx/fireball.wav", "Movement", "Fireball Throw"},
        {"boing", "assets/sfx/boing.WAV", "Movement", "Trampoline / Spring Bounce"},
        {"bubble", "assets/sfx/bubble.wav", "Movement", "Underwater Bubble"},
        {"walljump", "assets/sfx/walljump.wav", "Movement", "Wall Jump (Missing)"},
        {"groundpound", "assets/sfx/groundpound.wav", "Movement", "Ground Pound Slam (Missing)"},

        // Items & Blocks
        {"coin", "assets/sfx/coin.wav", "Items & Blocks", "Coin Collect"},
        {"powerup", "assets/sfx/power_up.wav", "Items & Blocks", "Power-up Collect"},
        {"powerup_appears", "assets/sfx/mushroom_fireflower_appears.wav", "Items & Blocks", "Power-up / Item Spawn"},
        {"oneup", "assets/sfx/one_up.wav", "Items & Blocks", "1-UP Extra Life"},
        {"bump", "assets/sfx/bump.wav", "Items & Blocks", "Block Head Bump"},
        {"break_block", "assets/sfx/break_brick_block.wav", "Items & Blocks", "Brick Block Shatter"},
        {"pipe", "assets/sfx/pipe.wav", "Items & Blocks", "Pipe Warp / Enter"},
        {"vine_grow", "assets/sfx/vine_grow.wav", "Items & Blocks", "Vine Growth"},
        {"pswitch", "assets/sfx/pswitch.wav", "Items & Blocks", "P-Switch Active (Missing)"},
        {"pow", "assets/sfx/pow.wav", "Items & Blocks", "POW Block Strike (Missing)"},

        // Level & Game Lifecycle
        {"powerdown", "assets/sfx/damage.wav", "Lifecycle", "Player Take Damage / Shrink"},
        {"death", "assets/sfx/lost_life.wav", "Lifecycle", "Player Lose Life"},
        {"flagpole", "assets/sfx/flagpole.wav", "Lifecycle", "Flagpole Slide"},
        {"enter_level", "assets/sfx/enter_level.wav", "Lifecycle", "Level Enter Fanfare"},
        {"time_warning", "assets/sfx/time_warning.wav", "Lifecycle", "Timer Warning (< 50s)"},
        {"pause", "assets/sfx/pause.wav", "Lifecycle", "Game Pause"},
        {"stage_clear", "assets/sfx/stage_clear.wav", "Lifecycle", "Stage Clear Fanfare"},
        {"world_clear", "assets/sfx/world_clear.wav", "Lifecycle", "World Clear Fanfare"},
        {"game_over", "assets/sfx/game_over.wav", "Lifecycle", "Game Over Screen"},
        {"bowser_fall", "assets/sfx/bowserfall.wav", "Lifecycle", "Bowser Defeated / Fall"},
        {"thwomp", "assets/sfx/thwomp.wav", "Lifecycle", "Thwomp Slam Impact"},

        // Surface Footsteps
        {"footstep_floor", "assets/sfx/footstep_floor.WAV", "Footsteps", "Standard Tile / Floor"},
        {"footstep_grass", "assets/sfx/footstep_grass.wav", "Footsteps", "Grass / Soil Surface"},
        {"footstep_metalcap", "assets/sfx/footstep_metalcap.wav", "Footsteps", "Metal Cap / Armor Surface"},
        {"footstep_stone", "assets/sfx/footstep_stone.wav", "Footsteps", "Stone / Castle (Missing)"},
        {"footstep_ice", "assets/sfx/footstep_ice.wav", "Footsteps", "Ice Block (Missing)"},
        {"footstep_metal", "assets/sfx/footstep_metal.wav", "Footsteps", "Conveyor / Metal (Missing)"},

        // UI & Combat
        {"menu_click", "assets/sfx/click.wav", "UI & Combat", "Menu Highlight (Missing)"},
        {"menu_confirm", "assets/sfx/confirm.wav", "UI & Combat", "Menu Select (Missing)"},
        {"achievement", "assets/sfx/achievement.wav", "UI & Combat", "Achievement Unlocked (Missing)"},
        {"splash", "assets/sfx/splash.wav", "UI & Combat", "Water Fall / Splash (Missing)"},
        {"cannon", "assets/sfx/cannon.wav", "UI & Combat", "Bill Blaster Cannon (Missing)"},
        {"fireball_hit", "assets/sfx/fireball_hit.wav", "UI & Combat", "Fireball Wall Hit (Missing)"}
    };

    std::map<std::string, bool> isMissing;
    int presentCount = 0;
    int missingCount = 0;

    // Load all sounds into ResourceManager
    for (const auto& sound : sounds) {
        bool success = ResourceManager::getInstance().loadSoundBuffer(sound.id, sound.filepath);
        isMissing[sound.id] = !success;
        if (success) {
            presentCount++;
        } else {
            missingCount++;
        }
    }

    float sfxVolume = 100.0f;
    SoundManager::getInstance().setSFXVolume(sfxVolume);

    char filterBuffer[128] = "";
    std::string lastPlayedSound = "None";

    while (window.isOpen()) {
        while (const std::optional<sf::Event> eventOpt = window.pollEvent()) {
            sf::Event event = *eventOpt;
            ImGui::SFML::ProcessEvent(window, event);

            if (event.is<sf::Event::Closed>()) {
                window.close();
            }
        }

        ImGui::SFML::Update(window, deltaClock.restart());

        // Position window to cover frame nicely
        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(940, 660), ImGuiCond_FirstUseEver);

        ImGui::Begin("Super Mario Sound Verification Board", nullptr, ImGuiWindowFlags_MenuBar);

        // Header / Summary Stats
        ImGui::Text("Total Registered Sounds: %d  | ", (int)sounds.size());
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.2f, 1.0f, 0.2f, 1.0f));
        ImGui::Text("Present: %d", presentCount);
        ImGui::PopStyleColor();
        ImGui::SameLine();
        ImGui::Text(" | ");
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
        ImGui::Text("Missing (Synth Beep Fallback): %d", missingCount);
        ImGui::PopStyleColor();

        ImGui::Separator();

        // Controls Bar
        if (ImGui::SliderFloat("Master SFX Volume", &sfxVolume, 0.0f, 100.0f, "%.0f%%")) {
            SoundManager::getInstance().setSFXVolume(sfxVolume);
        }

        ImGui::InputText("Filter / Search", filterBuffer, sizeof(filterBuffer));
        ImGui::SameLine();
        if (ImGui::Button("Clear Filter")) {
            filterBuffer[0] = '\0';
        }

        ImGui::Text("Last Played: ");
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.84f, 0.0f, 1.0f));
        ImGui::Text("%s", lastPlayedSound.c_str());
        ImGui::PopStyleColor();

        ImGui::Separator();

        // Group sounds by Category in Tabs
        std::vector<std::string> categories = {"All", "Movement", "Items & Blocks", "Lifecycle", "Footsteps", "UI & Combat"};

        if (ImGui::BeginTabBar("SoundCategoryTabs")) {
            for (const auto& cat : categories) {
                if (ImGui::BeginTabItem(cat.c_str())) {
                    
                    ImGui::BeginChild(("Child_" + cat).c_str(), ImVec2(0, 0), true);

                    // Filter logic
                    std::string filterStr = filterBuffer;
                    std::transform(filterStr.begin(), filterStr.end(), filterStr.begin(), ::tolower);

                    for (const auto& sound : sounds) {
                        if (cat != "All" && sound.category != cat) {
                            continue;
                        }

                        // Search filter matching ID or Description
                        std::string idLower = sound.id;
                        std::string descLower = sound.description;
                        std::transform(idLower.begin(), idLower.end(), idLower.begin(), ::tolower);
                        std::transform(descLower.begin(), descLower.end(), descLower.begin(), ::tolower);

                        if (!filterStr.empty() && 
                            idLower.find(filterStr) == std::string::npos && 
                            descLower.find(filterStr) == std::string::npos) {
                            continue;
                        }

                        ImGui::PushID(sound.id.c_str());

                        bool missing = isMissing[sound.id];
                        if (missing) {
                            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
                            ImGui::Text("[MISSING]");
                            ImGui::PopStyleColor();
                        } else {
                            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.2f, 1.0f, 0.2f, 1.0f));
                            ImGui::Text("[  OK  ] ");
                            ImGui::PopStyleColor();
                        }

                        ImGui::SameLine();
                        if (ImGui::Button("Play")) {
                            SoundManager::getInstance().playSound(sound.id);
                            lastPlayedSound = sound.id + " (" + sound.description + ")";
                        }

                        ImGui::SameLine();
                        ImGui::Text("%-20s - %s", sound.id.c_str(), sound.description.c_str());

                        if (ImGui::IsItemHovered()) {
                            ImGui::SetTooltip("File: %s\nCategory: %s\nStatus: %s", 
                                sound.filepath.c_str(), 
                                sound.category.c_str(), 
                                missing ? "Missing (Using Fallback Beep)" : "Present");
                        }

                        ImGui::PopID();
                    }

                    ImGui::EndChild();
                    ImGui::EndTabItem();
                }
            }
            ImGui::EndTabBar();
        }

        ImGui::End();

        window.clear(sf::Color(30, 30, 30));
        ImGui::SFML::Render(window);
        window.display();
    }

    ImGui::SFML::Shutdown();
    return 0;
}
