#include "Entities/ShadowMario.hpp"

#include "Core/InputManager.hpp"
#include "Graphics/Animator.hpp"
#include "Graphics/SpriteColorFilter.hpp"
#include "Utils/Constants.hpp"

#include <SFML/Graphics/RectangleShape.hpp>
#include <algorithm>
#include <cmath>

namespace {
// How often a ghost afterimage is dropped, and how long one lives. Six images
// over 0.36s reads as a smear behind the shadow without turning into a solid
// second body.
constexpr float kAfterimageInterval = 0.06f;
constexpr float kAfterimageLifetime = 0.36f;
constexpr std::size_t kMaxAfterimages = 6;

// The shadow's colour. Dark purple at ~65% alpha: distinct from every character
// in the atlas, and translucent enough that the player can see the geometry they
// are about to be cornered against.
constexpr std::uint8_t kShadowR = 90;
constexpr std::uint8_t kShadowG = 30;
constexpr std::uint8_t kShadowB = 140;
constexpr std::uint8_t kShadowA = 165;
} // namespace

ShadowMario::ShadowMario(sf::Vector2f startPos) : Player(startPos) {
    position = startPos;
    speed = Constants::WALK_SPEED;
    jumpForce = std::sqrt(2.0f * Constants::GRAVITY * Constants::GRAVITY_SCALE *
                          Constants::JUMP_HEIGHT);
    health = 1;
    boundingBox = AABB{startPos.x, startPos.y, 32.0f, 32.0f};
    setStartingForm(Form::Small);

    // A shadow cannot be killed, but it inherits Player's whole state machine,
    // and several paths in there reach for lives. Immortality keeps those paths
    // from ever concluding that this entity has died.
    setImmortal(true);
}

void ShadowMario::setupAnimations(const SpriteSheet* spriteSheet) {
    // Mario's frames, tinted at draw time. A shadow that used its own sprite set
    // would stop being recognisable as "you, three seconds ago", which is the
    // entire read of the mode.
    Player::setupCharacterAnimations(spriteSheet, "mario_small");
}

void ShadowMario::setDelay(float seconds) {
    m_delaySeconds = std::clamp(seconds, 0.5f, 10.0f);
}

void ShadowMario::resetChase(sf::Vector2f startPos) {
    m_historyBuffer.clear();
    m_afterimages.clear();
    m_activeInput = PlayerFramePacket{};
    m_started = false;
    m_afterimageTimer = 0.0f;
    m_contactTimer = 0.0f;
    m_gameTime = 0.0f;

    setPosition(startPos);
    setVelocity({0.0f, 0.0f});
    boundingBox.x = startPos.x;
    boundingBox.y = startPos.y;
    setGrounded(false);
    clearMovementRequests();
}

void ShadowMario::setCorrection(float threshold, float factor) {
    m_correctionThreshold = std::max(0.0f, threshold);
    m_correctionFactor = std::clamp(factor, 0.0f, 1.0f);
}

void ShadowMario::recordFrame(float gameTime, const Player& target) {
    m_gameTime = gameTime;

    // Intent is read from the input layer rather than from the Player, because
    // "jump" leaves no lasting flag on the character — jump() is an impulse. The
    // cape state reads the bound jump key exactly the same way, so this is the
    // established route from an entity to "is this action held".
    const InputManager& input = InputManager::getInstance();
    const int pad = target.getPlayerIndex();

    PlayerFramePacket packet;
    packet.timestamp = gameTime;
    packet.position  = target.getPosition();
    packet.moveLeft  = input.isActionHeld("left", pad);
    packet.moveRight = input.isActionHeld("right", pad);
    packet.jump      = input.isActionHeld("jump", pad);
    packet.run       = input.isActionHeld("run", pad);
    packet.crouch    = input.isActionHeld("crouch", pad);

    m_historyBuffer.push_back(packet);

    // The queue is self-limiting through the delay check in update(), but a
    // pathological frame rate must not let it grow without bound. At 60Hz a
    // 3-second delay holds 180 packets; twice the nominal capacity is generous
    // and still fixed.
    const std::size_t cap =
        static_cast<std::size_t>(m_delaySeconds * 2.0f * 60.0f) + 120;
    while (m_historyBuffer.size() > cap) {
        m_historyBuffer.pop_front();
    }
}

float ShadowMario::secondsBehind() const {
    if (m_historyBuffer.empty()) return 0.0f;
    return m_gameTime - m_historyBuffer.front().timestamp;
}

void ShadowMario::applyInput(const PlayerFramePacket& packet) {
    if (packet.moveLeft)  moveLeft();
    if (packet.moveRight) moveRight();
    if (packet.run)       run();
    if (packet.crouch)    crouch();

    // Jump on the rising edge only. The recorded flag is the key's *held* state,
    // so replaying it directly would turn one press into a jump attempt every
    // frame the human held the button — the shadow would bunny-hop where the
    // player made a single leap.
    if (packet.jump && !m_activeInput.jump) {
        jump();
    }

    m_activeInput = packet;
}

void ShadowMario::update(float dt) {
    if (m_contactTimer > 0.0f) m_contactTimer -= dt;

    // Drain every packet that has come due. Normally that is exactly one per
    // frame once the buffer has filled, but a frame-rate hitch can leave several
    // owing, and skipping them would let the shadow fall permanently behind its
    // own recording.
    bool applied = false;
    while (!m_historyBuffer.empty() &&
           (m_gameTime - m_historyBuffer.front().timestamp) >= m_delaySeconds) {
        const PlayerFramePacket packet = m_historyBuffer.front();
        m_historyBuffer.pop_front();
        applyInput(packet);
        applied = true;
        m_started = true;

        // Position correction, from the plan's §3.2. Only past the threshold, so
        // ordinary float noise is left alone, and only a fraction of the error
        // per frame, so a correction reads as the shadow drifting back onto its
        // path rather than snapping to it.
        const sf::Vector2f error = packet.position - position;
        if (std::abs(error.x) > m_correctionThreshold ||
            std::abs(error.y) > m_correctionThreshold) {
            position += error * m_correctionFactor;
            boundingBox.x = position.x;
            boundingBox.y = position.y;
        }
    }

    // Between packets the held keys stay held: the recording is at simulation
    // rate, but nothing guarantees update() is called at exactly that rate.
    if (!applied && m_started) {
        if (m_activeInput.moveLeft)  moveLeft();
        if (m_activeInput.moveRight) moveRight();
        if (m_activeInput.run)       run();
        if (m_activeInput.crouch)    crouch();
    }

    Player::update(dt);

    // Ghost trail.
    m_afterimageTimer += dt;
    if (m_started && m_afterimageTimer >= kAfterimageInterval) {
        m_afterimageTimer = 0.0f;
        m_afterimages.push_back({position, facingRight, 0.0f});
        while (m_afterimages.size() > kMaxAfterimages) {
            m_afterimages.pop_front();
        }
    }
    for (auto& image : m_afterimages) {
        image.age += dt;
    }
    while (!m_afterimages.empty() && m_afterimages.front().age > kAfterimageLifetime) {
        m_afterimages.pop_front();
    }
}

void ShadowMario::render(sf::RenderTarget& target) {
    if (!active) return;

    if (!m_animator || !m_hasAnimation) {
        // Same fallback shape every character draws when the atlas is missing,
        // in the shadow's colour so a missing sprite is still recognisable as
        // the shadow rather than as a stray red box.
        sf::RectangleShape rect(sf::Vector2f(boundingBox.width, boundingBox.height));
        rect.setPosition(sf::Vector2f(boundingBox.x, boundingBox.y));
        rect.setFillColor(sf::Color(kShadowR, kShadowG, kShadowB, kShadowA));
        rect.setOutlineColor(sf::Color(200, 140, 255));
        rect.setOutlineThickness(1.0f);
        target.draw(rect);
        return;
    }

    // Trail first, so the shadow itself draws over its own history.
    //
    // SpriteColorFilter has existed since the graphics phase with exactly the
    // tinting API this needs, and nothing had ever called it — it was one of the
    // five finished-but-unwired graphics subsystems recorded as audit item B-9.
    const sf::Vector2f realPosition = position;
    const AABB realBox = boundingBox;
    const bool realFacing = facingRight;

    for (const auto& image : m_afterimages) {
        const float remaining = 1.0f - (image.age / kAfterimageLifetime);
        if (remaining <= 0.0f) continue;

        sf::Sprite ghost = m_animator->getSprite();
        SpriteColorFilter::applyColorFilter(
            ghost, sf::Color(kShadowR, kShadowG, kShadowB,
                             static_cast<std::uint8_t>(kShadowA * 0.35f * remaining)));

        // drawSprite anchors against the bounding box, so the box is moved to
        // the afterimage's position for the duration of the draw and put back
        // immediately. Cheaper and less error-prone than duplicating the
        // aspect-fit maths that audit X-6 consolidated into drawSprite.
        position = image.position;
        boundingBox.x = image.position.x;
        boundingBox.y = image.position.y;
        facingRight = image.facingRight;
        drawSprite(target, ghost, SpriteAnchor::BottomCenter, /*flipX=*/facingRight);
    }

    position = realPosition;
    boundingBox = realBox;
    facingRight = realFacing;

    sf::Sprite sprite = m_animator->getSprite();
    SpriteColorFilter::applyColorFilter(
        sprite, sf::Color(kShadowR, kShadowG, kShadowB, kShadowA));
    drawSprite(target, sprite, SpriteAnchor::BottomCenter, /*flipX=*/facingRight);
}
