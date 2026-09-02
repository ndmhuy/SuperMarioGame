#include "Entities/Castle.hpp"
#include "Utils/Constants.hpp"

#include <SFML/Graphics/Sprite.hpp>
#include <algorithm>
#include <cmath>

namespace {
// castle_0 is 148x176 in the atlas and is drawn at 1:1 tile scale, so the
// target size is the frame size.
constexpr float CASTLE_W = Castle::WIDTH_TILES  * Constants::TILE_SIZE;   // 148
constexpr float CASTLE_H = Castle::HEIGHT_TILES * Constants::TILE_SIZE;   // 176

// Seconds for the flag to climb once the level is complete.
constexpr float FLAG_RISE_SECONDS = 1.2f;
}

Castle::Castle(sf::Vector2f position) : Block(position, {CASTLE_W, CASTLE_H}) {
    m_breakable = false;
    setTargetSize({CASTLE_W, CASTLE_H});
    boundingBox = AABB{position.x, position.y, CASTLE_W, CASTLE_H};
}

void Castle::setupAnimations(const SpriteSheet* spriteSheet) {
    Block::setupAnimations(spriteSheet);
    m_sheet = spriteSheet;
    applyFrame();
}

void Castle::setFrame(const std::string& frameKey) {
    if (frameKey.empty() || frameKey == m_frameKey) return;
    // Recorded even before the atlas is attached: a procedurally generated level
    // styles its castle (settleEndOfLevelScenery) before it wires entities up
    // (admitEntity), and the frame has to survive that ordering.
    if (m_sheet && !m_sheet->hasFrame(frameKey)) return;
    m_frameKey = frameKey;
    applyFrame();
}

void Castle::applyFrame() {
    if (!m_sheet || !m_sheet->hasFrame(m_frameKey)) return;

    // A single frame, not an animation — the castle does not move. It goes
    // through the Animator anyway so it is wired the same way every other block
    // is and Block::render can draw it without a special case.
    m_animation = Animation("castle");
    m_animation.frameList = {{m_frameKey, 1.0f}};
    m_animation.isLooping = false;

    // The box follows the art rather than the art being squashed into the box —
    // see setFrame's contract in the header.
    const sf::FloatRect bounds = m_sheet->getSprite(m_frameKey).getLocalBounds();
    if (bounds.size.x > 0.0f && bounds.size.y > 0.0f) {
        setTargetSize({bounds.size.x, bounds.size.y});
        boundingBox.width  = bounds.size.x;
        boundingBox.height = bounds.size.y;
    }

    if (m_animator) {
        m_animator->play(&m_animation);
        m_hasAnimation = true;
    }
}

void Castle::raiseFlag() {
    m_flagRaised = true;
}

void Castle::update(float dt) {
    Block::update(dt);
    if (!m_flagRaised || m_flagRise >= 1.0f) return;
    m_flagRise = std::min(1.0f, m_flagRise + dt / FLAG_RISE_SECONDS);
}

void Castle::render(sf::RenderTarget& target) {
    if (!active) return;

    Block::render(target);

    // The flag, once it has been raised. Drawn here rather than as a second
    // entity: it has no behaviour of its own, it only exists relative to this
    // castle's battlements, and a separate entity would need the castle's
    // position anyway.
    if (!m_flagRaised || m_flagRise <= 0.0f || !m_sheet) return;

    sf::Sprite flag = m_sheet->getSprite("castle_flag");
    const sf::FloatRect bounds = flag.getLocalBounds();
    if (bounds.size.x <= 0.0f || bounds.size.y <= 0.0f) return;

    // castle_flag is 13x14; doubled it reads at the same weight as the
    // brickwork, which is drawn at 2x from 16px source tiles.
    constexpr float FLAG_SCALE = 2.0f;
    flag.setScale({FLAG_SCALE, FLAG_SCALE});

    // The mast sits above the centre of the gatehouse. The flag travels the
    // height of one tile as it rises, ending just below the battlement line.
    // The box, not CASTLE_W: setFrame() can widen it (castle_white is 153px).
    const float mastX = position.x + boundingBox.width * 0.5f - bounds.size.x * FLAG_SCALE * 0.5f;
    const float travel = Constants::TILE_SIZE;
    const float restY = position.y + Constants::TILE_SIZE * 1.25f;
    const float y = restY + travel * (1.0f - m_flagRise);

    flag.setPosition({std::round(mastX), std::round(y)});
    target.draw(flag);
}
