#include "Entities/Boss.hpp"
#include "Core/EventBus.hpp"
#include "Core/Game.hpp"
#include "Core/SoundManager.hpp"
#include "Entities/Player.hpp"
#include "Physics/CollisionDetector.hpp"
#include "Utils/Constants.hpp"
#include "Utils/TileMap.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <utility>

namespace {
// Seconds of immunity after a hit lands. Long enough that one stomp is one hit
// even though the overlap persists for several frames.
constexpr float HIT_INVULNERABILITY = 1.0f;
// How long the defeat sequence plays before the boss is removed.
constexpr float DEFEAT_DURATION = 2.0f;
}

namespace {
// A boss's health bar is where difficulty is felt most, so it is scaled here
// rather than at every call site. Always at least one hit.
int scaledHealth(int maxHealth) {
    const float scale = Game::getInstance().difficulty().bossHealthScale();
    return std::max(1, static_cast<int>(std::lround(static_cast<float>(maxHealth) * scale)));
}
}

Boss::Boss(sf::Vector2f position, std::string displayName, int maxHealth,
           int scoreValue, sf::Vector2f size)
    : Enemy(position, scoreValue, size),
      m_arenaSpawn(position),
      m_displayName(std::move(displayName)),
      m_maxHealth(scaledHealth(std::max(1, maxHealth))),
      m_health(m_maxHealth) {
    m_phase = phaseForHealth(m_health);
}

void Boss::returnToArenaSpawn() {
    setPosition(m_arenaSpawn);
    setVelocity({0.0f, 0.0f});
    m_health = m_maxHealth;
    m_phase = phaseForHealth(m_health);
    // Neither timer should survive the trip: i-frames earned before the fall
    // would otherwise make the returning boss briefly unhittable, and a stagger
    // would hand the player a free opening they did not earn.
    m_invulnerableTimer = 0.0f;
    m_staggerTimer = 0.0f;
}

bool Boss::onLeftLevel() {
    // Already dying: let the ordinary prune take it, so the defeat sequence
    // that is mid-flight is not undone by a respawn.
    if (isDefeated()) return true;
    // A boss burning down in the lava has not wandered off — he is exactly where
    // the axe put him. Putting him back at full health would undo the death the
    // player just earned, which is what this guard would otherwise do: the drain
    // leaves him alive (health > 0) for a couple of seconds, and he is sitting
    // below the bridge with nothing under him.
    if (isDyingInLava()) return false;
    returnToArenaSpawn();
    std::cout << "[PlayingState] " << m_displayName
              << " left the level; returned to the arena spawn at full health."
              << std::endl;
    return false;
}

int Boss::phaseForHealth(int health) const {
    // Two phases, switching at half health — the split the SPEC gives Bowser.
    return (health * 2 <= m_maxHealth) ? 2 : 1;
}

bool Boss::takeHit(int amount) {
    if (isDefeated() || isInvulnerable() || amount <= 0) return false;

    m_health = std::max(0, m_health - amount);
    m_invulnerableTimer = HIT_INVULNERABILITY;
    SoundManager::getInstance().playSound("bump");
    onTookHit();

    const int newPhase = phaseForHealth(m_health);
    if (newPhase != m_phase) {
        m_phase = newPhase;
        std::cout << "[Boss] " << m_displayName << " entering phase " << m_phase << std::endl;
        onPhaseChanged(m_phase);
    }

    if (isDefeated()) {
        std::cout << "[Boss] " << m_displayName << " defeated." << std::endl;
        m_defeatTimer = DEFEAT_DURATION;
        // Stop dead: the defeat sequence should not carry momentum.
        velocity = {0.0f, 0.0f};
        onDefeated();
    }
    return true;
}

void Boss::onStomped() {
    takeHit();
}

void Boss::onHitByFireball() {
    // Fire does the same damage as a stomp. Overriding this is what makes a
    // boss immune to fire; Bowser and Boom Boom both take it.
    takeHit();
}

bool Boss::onPlayerTouch(Player& player, const CollisionInfo& info, bool stomped) {
    (void)info;
    // A boss burning down in the lava is out of the fight. He neither hurts the
    // player nor pays out, and the contact is reported as consumed so the
    // resolver's generic stomp path cannot claim him either — that path would
    // award his 5000 points a second time, on top of the ones the drain's own
    // defeat is about to pay.
    if (isDyingInLava()) return true;

    // `stomped` is deliberately ignored. It is a *positional* test, true on
    // every frame the player's feet are near the enemy's top, and the generic
    // path it drives never separates the two boxes — so standing on BoomBoom
    // held it true indefinitely. That paid score and combo every frame, landed
    // a real hit every time the one-second i-frames lapsed, and never hurt the
    // player, because takeDamage is only in the else branch. Three seconds of
    // standing still won the fight. A boss asks for a real descent instead.
    (void)stomped;

    const bool descending = player.getVelocity().y > STOMP_MIN_DESCENT_SPEED;

    // Bounce the player clear of the boss's box, so the next frame is not
    // another contact frame. Without this the player never leaves and the whole
    // cycle repeats.
    auto bounceClear = [this, &player] {
        player.setVelocity({player.getVelocity().x, -Constants::STOMP_BOUNCE_FORCE});
        sf::Vector2f pos = player.getPosition();
        pos.y = getBoundingBox().y - player.getBoundingBox().height - 1.0f;
        player.setPosition(pos);
    };

    // Contact that is not an attack: knocked away from the boss and hurt. Named
    // once because three different situations end here — resting on the boss,
    // touching it during its i-frames, and (for Bowser) landing on a guard that
    // has not been broken yet.
    auto hurtPlayer = [this, &player] {
        const float dx = player.getBoundingBox().getCenter().x -
                         getBoundingBox().getCenter().x;
        const float direction = (dx >= 0.0f) ? 1.0f : -1.0f;
        player.setVelocity({direction * Constants::KNOCKBACK_FORCE_X,
                            -Constants::KNOCKBACK_FORCE_Y});
        player.takeDamage(1);
    };

    // A staggered boss has dropped its guard: contact does not hurt, and any
    // contact at all lands a hit. This is the opening the fight is built around
    // — requiring a fast descending stomp *and* a stagger would mean the player
    // has to solve both problems in the same three seconds, which is the
    // difficulty the stagger exists to remove.
    if (isStaggered()) {
        const bool landed = tryStomp();
        bounceClear();
        if (landed) {
            player.incrementCombo();
            player.addScore(getScoreValue() * player.getComboCounter());
        }
        return true;
    }

    // Resting on the boss is not an attack. While its i-frames run, or while the
    // player is not actually falling onto it, contact is contact — it hurts,
    // exactly as touching its side does.
    //
    // And for a boss whose guard is the fight, a genuine descending stomp is not
    // an attack either while that guard is up: Bowser answers
    // canBeStompedWhileGuarded() with false, so jumping on an unstaggered Bowser
    // now costs the player a hit rather than costing him one. Without this the
    // fireball mechanic his own comments describe was optional — five stomps won
    // the fight and nobody ever needed a flower.
    if (!descending || isInvulnerable() || !canBeStompedWhileGuarded()) {
        hurtPlayer();
        return true;
    }

    // A genuine descending impact. Pay out only if the hit actually landed —
    // tryStomp() reports that, and it was once ignored.
    const bool landed = tryStomp();
    bounceClear();
    if (landed) {
        player.incrementCombo();
        player.addScore(getScoreValue() * player.getComboCounter());
    }
    return true;
}

void Boss::stagger(float seconds) {
    if (isDefeated() || seconds <= 0.0f) return;

    const bool wasStaggered = isStaggered();
    m_staggerTimer = std::max(m_staggerTimer, seconds);
    // The i-frames go with it. A stagger that still leaves the boss invulnerable
    // is an opening the player can see and cannot use, which is worse than no
    // opening at all — CollisionResolver reads isInvulnerable() to decide
    // whether a stomp lands.
    m_invulnerableTimer = 0.0f;
    // Nothing carries momentum through a stagger; it is a stop, not a slow.
    velocity.x = 0.0f;

    if (!wasStaggered) {
        std::cout << "[Boss] " << m_displayName << " staggered for " << seconds
                  << "s." << std::endl;
        onStaggerBegan();
    }
}

void Boss::defeatNow() {
    if (isDefeated()) return;
    // Both guards cleared first: takeHit() refuses while the i-frames run, and a
    // boss that happened to be mid-stagger would otherwise survive the bridge.
    m_invulnerableTimer = 0.0f;
    m_staggerTimer = 0.0f;
    takeHit(m_health);
}

void Boss::endStagger() {
    if (!isStaggered()) return;
    m_staggerTimer = 0.0f;
    onStaggerEnded();
}

bool Boss::isPhysicsDriven() const {
    // Falling is ordinary gravity — that is the whole point of the state, and
    // lava is not solid so nothing stops him on the way down. Draining is not:
    // see the declaration.
    if (m_lavaDeath == LavaDeath::Draining) return false;
    return Enemy::isPhysicsDriven();
}

float Boss::lavaDrainInterval() const {
    return LAVA_DRAIN_SECONDS / static_cast<float>(std::max(1, m_maxHealth));
}

void Boss::beginLavaDeath() {
    if (isDefeated() || isDyingInLava()) return;

    // Both guards go, for exactly the reason defeatNow() drops them: a boss who
    // happened to be mid-stagger or inside his i-frames must not survive the
    // bridge, and the drain below writes m_health directly.
    m_invulnerableTimer = 0.0f;
    m_staggerTimer = 0.0f;
    // He drops rather than steps off. Keeping his walking momentum would carry
    // him sideways onto the ledge the player is standing on.
    velocity = {0.0f, 0.0f};

    m_lavaDeath = LavaDeath::Falling;
    m_lavaFallTimer = 0.0f;
    // Armed now so the first point does not come off the bar the instant he
    // lands: the fall reads as the cause, and the bar as the consequence.
    m_lavaDrainTimer = lavaDrainInterval();

    std::cout << "[Boss] " << m_displayName
              << " lost the bridge and is falling into the lava." << std::endl;
}

void Boss::updateLavaDeath(float dt) {
    const TileMap* map = Game::getInstance().getTileMap();

    // Has any part of his underside reached lava? Sampled along the bottom edge
    // a tile at a time, always finishing on the far corner, so a body wider than
    // a tile cannot straddle the one column it never samples — the same shape
    // TerrainProbe::overlapsSolid uses, but asking about lava rather than
    // solidity, which is the opposite property (lava holds nothing up).
    auto inLava = [this, map] {
        if (!map) return false;
        const AABB box = getBoundingBox();
        const float y = box.y + box.height - 1.0f;
        const float right = box.x + box.width - 1.0f;
        for (float x = box.x + 1.0f;;) {
            if (map->getTileAt(x, y) == TileType::Lava) return true;
            if (x >= right) break;
            x = std::min(x + Constants::TILE_SIZE, right);
        }
        return false;
    };

    auto settleIntoLava = [this](float sink) {
        m_lavaDeath = LavaDeath::Draining;
        velocity = {0.0f, 0.0f};
        m_lavaSinkLeft = sink;
        SoundManager::getInstance().playSound("bowserfall");
        std::cout << "[Boss] " << m_displayName
                  << " is burning up in the lava." << std::endl;
    };

    if (m_lavaDeath == LavaDeath::Falling) {
        m_lavaFallTimer += dt;
        if (inLava()) {
            settleIntoLava(LAVA_SINK_DEPTH);
        } else if (m_lavaFallTimer >= LAVA_FALL_GRACE) {
            // No lava found. Die here anyway rather than fall forever — see
            // LAVA_FALL_GRACE. No sink, because there is nothing to sink into.
            settleIntoLava(0.0f);
        }
        return;
    }

    if (m_lavaSinkLeft > 0.0f) {
        const float step = std::min(m_lavaSinkLeft, LAVA_SINK_SPEED * dt);
        setPosition({position.x, position.y + step});
        m_lavaSinkLeft -= step;
    }

    m_lavaDrainTimer -= dt;
    if (m_lavaDrainTimer > 0.0f) return;
    m_lavaDrainTimer = lavaDrainInterval();

    if (m_health > 1) {
        // Decremented directly rather than through takeHit(): takeHit() arms a
        // full second of i-frames, which would swallow every tick after the
        // first and stall the bar at one point below full. The phase change it
        // also announces belongs to a fight, and this fight is already decided.
        --m_health;
        SoundManager::getInstance().playSound("bump");
        return;
    }

    // The last point goes through the ordinary defeat, so the score, the
    // BossDefeated event and the defeat animation are the same ones a fifth
    // stomp would have produced. isDyingInLava() stays true through it, which is
    // what keeps the arena — and the bar the player is watching — up until the
    // sequence finishes.
    defeatNow();
}

void Boss::update(float dt) {
    if (!active) return;

    if (m_invulnerableTimer > 0.0f) {
        m_invulnerableTimer = std::max(0.0f, m_invulnerableTimer - dt);
    }

    // Ticked before the defeat check, so a boss killed during its own stagger
    // still runs its defeat sequence rather than staying frozen.
    if (m_staggerTimer > 0.0f) {
        m_staggerTimer = std::max(0.0f, m_staggerTimer - dt);
        if (m_staggerTimer <= 0.0f) onStaggerEnded();
    }

    // The bridge went out from under him. He is neither fighting nor yet dead,
    // so neither the behaviour below nor the defeat sequence beneath applies:
    // this is its own sequence, and it ends by entering that one.
    if (isDyingInLava() && !isDefeated()) {
        updateLavaDeath(dt);
        if (m_animator && m_hasAnimation) {
            m_animator->update(dt);
        }
        return;
    }

    if (isDefeated()) {
        m_defeatTimer -= dt;

        // Score and the BossDefeated event fire once, at the start of the
        // sequence, so the celebration music and the achievement do not wait
        // for the animation.
        if (!m_defeatAnnounced) {
            m_defeatAnnounced = true;
            if (Player* player = Game::getInstance().getNearestPlayer(getPosition())) {
                player->addScore(getScoreValue());
            }
            EventBus::getInstance().publish({EventType::BossDefeated, getScoreValue()});
        }

        if (m_animator && m_hasAnimation) {
            m_animator->update(dt);
        }
        if (m_defeatTimer <= 0.0f) {
            destroy();
        }
        return;
    }

    // A staggered boss does not act. This is the whole point of the window: it
    // stops walking into the player and stops attacking, so the opening is a
    // real one rather than a moment the player has to survive as well as use.
    if (!isStaggered()) {
        updateBehaviour(dt);
    } else {
        velocity.x = 0.0f;
    }

    if (m_animator && m_hasAnimation) {
        m_animator->update(dt);
    }
}
