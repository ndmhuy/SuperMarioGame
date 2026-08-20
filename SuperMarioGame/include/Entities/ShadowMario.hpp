#pragma once

#include "Entities/Player.hpp"
#include <deque>

// One frame of the human player's intent, plus where they actually were.
//
// The position is not redundant with the inputs. Replaying inputs alone drifts:
// the physics is float-based, the entity list the collision pass walks changes
// from frame to frame as things spawn and are pruned, and the enemy strategies
// read a Game singleton. So the inputs drive the shadow — that is what makes it
// animate and land like a real character rather than slide along a spline — and
// the recorded position is the leash that stops the drift accumulating.
struct PlayerFramePacket {
    float timestamp = 0.0f;
    sf::Vector2f position{0.0f, 0.0f};
    bool moveLeft = false;
    bool moveRight = false;
    bool jump = false;
    bool run = false;
    bool crouch = false;
};

// Bonus C — Shadow Mario: the player's own path, three seconds late.
//
// Shadow Mario is a Player subclass because it has to move like one: the same
// physics, the same states, the same animations. What it is *not* is a
// participant. It has no lives and no score, it cannot win, it cannot be hurt,
// and it does not compete for items — which is why it reports itself as a
// contact hazard and the collision resolver skips it for everything except
// touching the human.
class ShadowMario : public Player {
public:
    explicit ShadowMario(sf::Vector2f startPos);
    ~ShadowMario() override = default;

    std::string getTypeName() const override { return "shadow_mario"; }
    std::string getCharacterName() const override { return "shadow"; }

    void update(float dt) override;
    void render(sf::RenderTarget& target) override;
    void setupAnimations(const SpriteSheet* spriteSheet) override;

    // Sample `target`'s current intent and position into the delay queue. Called
    // once per simulated frame by PlayingState, before this entity is updated,
    // so a packet is never both recorded and consumed on the same frame.
    void recordFrame(float gameTime, const Player& target);

    // Put the chase back to its opening state at `startPos`, forgetting every
    // recorded frame.
    //
    // Required on respawn. Without it the buffer still held the dead life's path
    // while the player restarted at the checkpoint, so the recorded positions
    // the correction lerp was pulling towards were hundreds of pixels away from
    // where the player now was — the shadow flew back across the level and
    // killed them again on arrival, which cost the second life about as fast as
    // the first.
    void resetChase(sf::Vector2f startPos);

    // Seconds until the shadow reaches where the player is standing now — the
    // number the HUD gauge shows. Equals the configured delay once the queue has
    // filled; shorter while it is still filling at the start of a level.
    float secondsBehind() const;

    // How far behind the shadow replays. Exposed for the dev panel's slider:
    // three seconds is the spec's value, but it is the single number that
    // decides whether the mode is tense or trivial, so it has to be tunable
    // while the game is running.
    float getDelay() const { return m_delaySeconds; }
    void setDelay(float seconds);

    // Position-correction tuning, also on a dev slider. `threshold` is the drift
    // in pixels tolerated before correcting; `factor` is how much of the error is
    // taken back per frame.
    void setCorrection(float threshold, float factor);

    // A moving hazard, not a competitor: contact damages the human, and every
    // other collision pairing ignores it.
    bool isContactHazard() const override { return true; }

    // Inert until it has something to replay.
    //
    // The shadow spawns on top of the player — it has to, because for the first
    // three seconds the path it is replaying starts exactly where the player
    // started. Being collidable during that window meant it damaged the player
    // on frame one and again on respawn: a Shadow Chase run ended in a game over
    // about two seconds in, without the player having pressed a key. Tile
    // collision is a separate question (collidesWithTiles), so the shadow still
    // stands on the ground while it waits.
    bool isCollidable() const override { return m_started && !m_dying; }

    // Nothing hurts a shadow — not enemies, not fireballs, not lava.
    void takeDamage(int) override {}

    // The collision resolver tells the shadow when it was the thing that hurt
    // the player, which is what lets the game-over screen say so truthfully.
    void onContactWithPlayer() override { m_contactTimer = kContactMemory; }
    bool caughtPlayerRecently() const { return m_contactTimer > 0.0f; }

    // True once the queue has produced at least one packet, i.e. the shadow has
    // started moving. Before that it stands at the spawn point and the HUD says
    // so rather than showing a misleading 0.0s gap.
    bool hasStarted() const { return m_started; }

private:
    // How long a contact stays attributable. Death is a sequence — the hit lands,
    // then the player pops up and falls — so the cause has to outlive the frame
    // it happened on for killPlayer() to still be able to name it.
    static constexpr float kContactMemory = 1.0f;
    float m_contactTimer = 0.0f;

    std::deque<PlayerFramePacket> m_historyBuffer;
    float m_delaySeconds = 3.0f;
    // Clock this shadow shares with the recorder. Advanced by recordFrame(), so
    // it counts simulated time and stops while the game is paused.
    float m_gameTime = 0.0f;
    bool m_started = false;

    // Drift correction. 4px and 0.1 come from docs/two_player_ai_plan.md §3.2:
    // large enough that ordinary float noise does not trigger it, gentle enough
    // that a correction is not visible as a teleport.
    float m_correctionThreshold = 4.0f;
    float m_correctionFactor = 0.1f;

    // The last packet applied, kept so update() can keep holding a movement key
    // between the 60Hz packets and any slower frame the renderer runs at.
    PlayerFramePacket m_activeInput;

    // Trailing ghost afterimages: position and the age that fades them out.
    struct Afterimage {
        sf::Vector2f position;
        bool facingRight = true;
        float age = 0.0f;
    };
    std::deque<Afterimage> m_afterimages;
    float m_afterimageTimer = 0.0f;

    void applyInput(const PlayerFramePacket& packet);
};
