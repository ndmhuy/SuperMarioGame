#pragma once

#include "Core/IGameState.hpp"
#include "Graphics/UiRenderer.hpp"
#include "Graphics/SpriteSheet.hpp"
#include "Utils/MapGenerator.hpp"
#include <memory>
#include <string>
#include <vector>

// Task 7.2 — Character Select.
//
// Mario and Luigi are always available. Toad and Peach are gated behind the
// "toad" and "peach" achievements, which AchievementManager already tracks and
// unlocks; this screen only reads them.
class CharacterSelectState : public IGameState {
public:
    // `startInEditor` / `isProcedural` / `genConfig` are forwarded verbatim to the
    // PlayingState this screen creates, so the menu's editor and generator entry
    // points can route through character select without special-casing.
    explicit CharacterSelectState(bool startInEditor = false,
                                  bool isProcedural = false,
                                  const MapGeneratorConfig& genConfig = MapGeneratorConfig());
    ~CharacterSelectState() override = default;

    void enter() override;
    void exit() override;
    void handleInput(const sf::Event& event) override;
    void update(float dt) override;
    void render(sf::RenderTarget& target) override;

private:
    struct CharacterSlot {
        std::string id;             // "mario" — also the sprite-frame prefix
        std::string displayName;
        std::string blurb;          // one-line description of the character's handling
        std::string achievementId;  // empty when always available
        std::string unlockHint;     // what to do to unlock it, shown on locked cards
    };

    void moveSelection(int delta);
    void confirmSelection();
    bool isUnlocked(const CharacterSlot& slot) const;

    std::vector<CharacterSlot> m_slots;
    int m_selected = 0;
    float m_elapsed = 0.0f;
    bool m_dismissed = false;

    bool m_startInEditor = false;
    bool m_isProcedural = false;
    MapGeneratorConfig m_genConfig;

    // Portraits come from the same atlas the player uses, so a character never
    // shows a preview that does not match how it renders in-game.
    std::unique_ptr<SpriteSheet> m_playerSheet;
};
