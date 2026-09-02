#pragma once

#include <functional>
#include <string>

#include "Entities/Block.hpp"

class Flagpole : public Block {
public:
    // The default is the height of the art. pole_flag_green and
    // full_flag_pole_0..4 are all 24x168 in the atlas, and the sprite is drawn
    // at that size — but the default used to be 300, which is what the COLLISION
    // box was built from. So the box was 132px taller than the pole anyone could
    // see: settling the box onto the floor left the drawn flag hanging four
    // tiles up, and the catch-height score was measured against a pole that did
    // not exist. One number now, used for both.
    explicit Flagpole(sf::Vector2f position, float poleHeight = 168.0f);
    ~Flagpole() override = default;

    std::string getTypeName() const override { return "flagpole"; }

    void update(float dt) override;
    void onHitFromBelow(Player& player) override;
    BlockTouch onCharacterTouch(Character& character, const CollisionInfo& info) override;
    void render(sf::RenderTarget& target) override;
    void setupAnimations(const SpriteSheet* spriteSheet) override;

    // Calculates completion score based on player's collision Y coordinate
    void onPlayerCollision(Player& player, float collisionY);

    // Whether a touch right now would actually finish the level.
    //
    // A flagpole used to latch m_triggered, award the points and publish
    // LevelComplete on the first touch, unconditionally -- and PlayingState's
    // handler then REFUSED that completion while a boss was still alive
    // ("no escape until defeated", SPEC 6.4). The latch stayed set, so the
    // flagpole was spent and the level could never be finished afterwards:
    // a soft lock, reported after a Boom Boom fight. level_2's arena is tiles
    // 176-192 and its flagpole stands at 193, one tile past the clamp the
    // player is pressed against during the fight, so brushing it was easy --
    // and easy to miss, which is why one player hit it and another could not
    // reproduce it.
    //
    // The decision is PlayingState's, so it installs it here rather than the
    // flagpole guessing. Unset means "always allowed", which is what a bare
    // Flagpole in a test or the level editor should do.
    void setCompletionGate(std::function<bool()> gate) { m_completionGate = std::move(gate); }
    bool canCompleteNow() const { return !m_completionGate || m_completionGate(); }

    float getPoleHeight() const { return m_poleHeight; }
    bool isTriggered() const { return m_triggered; }
    // How far the flag has slid down the pole, in pixels from the top. Zero
    // until the player touches it, m_poleHeight once the descent finishes.
    float getFlagY() const { return m_flagY; }

private:
    // Two clips: the flag waiting at the top, and the one-shot slide down the
    // pole played when the player touches it. Block::m_animation holds a copy of
    // whichever is current; these are kept as the definitions to copy from.
    Animation m_raisedAnimation;
    Animation m_descentAnimation;

    float m_poleHeight = 168.0f;
    bool m_triggered = false;
    std::function<bool()> m_completionGate;
    // These three were placeholders for a slide-down that was never written —
    // m_flagY stayed at zero forever, so getFlagY() always answered "at the top"
    // and two of them were dead weight (-Wunused-private-field). They now track
    // the real descent animation.
    float m_flagY = 0.0f;
    float m_targetFlagY = 0.0f;
    float m_animTimer = 0.0f;
};
