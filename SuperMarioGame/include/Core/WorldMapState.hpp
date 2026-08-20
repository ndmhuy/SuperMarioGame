#pragma once

#include "Core/IGameState.hpp"
#include "Graphics/SpriteSheet.hpp"
#include "Graphics/UiRenderer.hpp"
#include "Utils/CampaignProgress.hpp"
#include <memory>
#include <vector>

// Task 7.3 — the world map.
//
// Sits between character select and play: the campaign as a path of nodes, with
// completion marks, per-level star coins, and sequential unlocking. Finishing a
// level still rolls straight into the next one, so the map is the place you
// choose *where to start* and see what you have cleared, not a screen you are
// bounced back to after every flagpole.
class WorldMapState : public IGameState {
public:
    explicit WorldMapState(int characterIndex = 0);
    ~WorldMapState() override = default;

    void enter() override;
    void exit() override;
    void handleInput(const sf::Event& event) override;
    void update(float dt) override;
    void render(sf::RenderTarget& target) override;

private:
    struct Node {
        sf::Vector2f position;   // screen-space centre
        int levelIndex = 0;
    };

    void moveSelection(int delta);
    void confirmSelection();
    void buildNodes();
    void drawPath(sf::RenderTarget& target) const;
    void drawNode(sf::RenderTarget& target, const Node& node) const;

    int m_characterIndex = 0;
    int m_selected = 0;
    float m_elapsed = 0.0f;
    bool m_dismissed = false;

    std::vector<Node> m_nodes;
    // Read once on entry: the map is not live-updated while it is open.
    std::vector<LevelProgress> m_progress;

    // Star-coin pips are drawn from the real atlas when it is available.
    std::unique_ptr<SpriteSheet> m_scenerySheet;
    std::unique_ptr<SpriteSheet> m_playerSheet;
};
