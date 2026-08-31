#include "Core/AchievementManager.hpp"
#include "Core/StatisticsTracker.hpp"
#include "Utils/Serializer.hpp"
#include <any>
#include <iostream>
#include <algorithm>

AchievementManager& AchievementManager::getInstance() {
    static AchievementManager instance;
    return instance;
}

void AchievementManager::init() {
    if (m_initialized) return;

    setupDefaultAchievementsList();

    auto sub = [this](EventType type) {
        return EventBus::ScopedSubscription(type, [this](const GameEvent& ev) { handleEvent(ev); });
    };
    m_subscriptions.push_back(sub(EventType::CoinCollected));
    m_subscriptions.push_back(sub(EventType::EnemyDefeated));
    m_subscriptions.push_back(sub(EventType::PlayerDied));
    m_subscriptions.push_back(sub(EventType::LevelComplete));
    m_subscriptions.push_back(sub(EventType::ComboHit));
    m_subscriptions.push_back(sub(EventType::StarCoinCollected));
    m_subscriptions.push_back(sub(EventType::BossDefeated));
    m_subscriptions.push_back(sub(EventType::HiddenBlockFound));
    m_subscriptions.push_back(sub(EventType::PlayerDamaged));

    m_initialized = true;
}

void AchievementManager::shutdown() {
    m_subscriptions.clear();
    m_initialized = false;
}

void AchievementManager::reset() {
    for (auto& ach : m_achievements) {
        ach.unlocked = false;
    }
    m_activeToasts.clear();
    m_coinsThisRun = 0;
    m_levelTime = 0.0f;
    m_takenDamageThisLevel = false;
    m_enemiesDefeatedWithShell = 0;
    m_hiddenBlocksFound = 0;
    m_starCoinsCount = 0;
}

void AchievementManager::unlockAchievement(const std::string& id) {
    auto it = std::find_if(m_achievements.begin(), m_achievements.end(),
        [&id](const Achievement& a) { return a.id == id; });

    if (it != m_achievements.end() && !it->unlocked) {
        it->unlocked = true;
        std::cout << "ACHIEVEMENT UNLOCKED: " << it->name << " (" << it->condition << ")" << std::endl;

        // Queue visual toast notification
        AchievementToast toast;
        toast.id = it->id;
        toast.name = it->name;
        toast.icon = it->icon;
        toast.timer = 0.0f;
        toast.alpha = 1.0f;
        m_activeToasts.push_back(toast);

        // Publish event so HUD or SoundManager can play a SFX
        EventBus::getInstance().publish({EventType::AchievementUnlocked, it->id});

        // Persist immediately. Unlocking a character is the kind of thing a
        // player will quit to go and try, and waiting for a clean shutdown to
        // write it means a force-quit silently takes it away again.
        Serializer::saveProfile();
    }
}

bool AchievementManager::isUnlocked(const std::string& id) const {
    auto it = std::find_if(m_achievements.begin(), m_achievements.end(),
        [&id](const Achievement& a) { return a.id == id; });
    return (it != m_achievements.end()) && it->unlocked;
}

void AchievementManager::setUnlockedAchievements(const std::vector<std::string>& unlockedIds) {
    // First lock all
    for (auto& ach : m_achievements) {
        ach.unlocked = false;
    }
    // Unlock matching
    for (const auto& id : unlockedIds) {
        auto it = std::find_if(m_achievements.begin(), m_achievements.end(),
            [&id](const Achievement& a) { return a.id == id; });
        if (it != m_achievements.end()) {
            it->unlocked = true;
        }
    }
    m_activeToasts.clear();
}

std::vector<std::string> AchievementManager::getUnlockedIds() const {
    std::vector<std::string> ids;
    for (const auto& ach : m_achievements) {
        if (ach.unlocked) {
            ids.push_back(ach.id);
        }
    }
    return ids;
}

void AchievementManager::update(float dt) {
    // Progress toast notification durations
    for (auto it = m_activeToasts.begin(); it != m_activeToasts.end(); ) {
        it->timer += dt;
        if (it->timer >= 3.0f) {
            it = m_activeToasts.erase(it);
        } else {
            if (it->timer > 2.5f) {
                it->alpha = (3.0f - it->timer) / 0.5f;
            }
            ++it;
        }
    }
    
    m_levelTime += dt;
}

void AchievementManager::registerDamageTaken() {
    m_takenDamageThisLevel = true;
}

void AchievementManager::registerBlockFound() {
    m_hiddenBlocksFound++;
    if (m_hiddenBlocksFound >= 5) {
        unlockAchievement("secret_finder");
    }
}

void AchievementManager::registerShellKickDefeat() {
    m_enemiesDefeatedWithShell++;
    if (m_enemiesDefeatedWithShell >= 3) {
        unlockAchievement("shell_shocker");
    }
}

void AchievementManager::setLevelTime(float time) {
    m_levelTime = time;
}

void AchievementManager::handleEvent(const GameEvent& event) {
    try {
        switch (event.type) {
            case EventType::CoinCollected: {
                int amount = 1;
                if (event.data.has_value() && event.data.type() == typeid(int)) {
                    amount = std::any_cast<int>(event.data);
                }
                m_coinsThisRun += amount;
                if (m_coinsThisRun >= 100) {
                    unlockAchievement("100_coins");
                }
                break;
            }
            case EventType::EnemyDefeated: {
                unlockAchievement("first_stomp");
                break;
            }
            case EventType::PlayerDied: {
                m_coinsThisRun = 0;
                m_takenDamageThisLevel = true; // resets untouchable parameter
                break;
            }
            case EventType::ComboHit: {
                if (event.data.has_value() && event.data.type() == typeid(int)) {
                    int combo = std::any_cast<int>(event.data);
                    if (combo >= 8) {
                        unlockAchievement("combo_king");
                    }
                }
                break;
            }
            case EventType::StarCoinCollected: {
                m_starCoinsCount++;
                if (m_starCoinsCount >= 9) {
                    unlockAchievement("star_hoarder");
                }
                break;
            }
            case EventType::BossDefeated: {
                unlockAchievement("beat_bowser");
                break;
            }
            case EventType::HiddenBlockFound: {
                // The real signal, published by HiddenBlock::onHitFromBelow.
                // This case used to be EventType::BlockBroken with the comment
                // "here mocked": every brick a Super player smashed counted as a
                // secret, and revealing an actual hidden block counted for
                // nothing, so "Find all hidden blocks" measured the opposite of
                // what it claims.
                registerBlockFound();
                break;
            }
            case EventType::PlayerDamaged: {
                registerDamageTaken();
                break;
            }
            case EventType::LevelComplete: {
                // Check level timers
                if (m_levelTime < 120.0f) {
                    unlockAchievement("speed_demon");
                }
                // Check damage
                if (!m_takenDamageThisLevel) {
                    unlockAchievement("untouchable");
                }
                // Level completion triggers characters unlock:
                if (event.data.has_value() && event.data.type() == typeid(int)) {
                    int finishedLevelId = std::any_cast<int>(event.data);
                    if (finishedLevelId >= 3) {
                        unlockAchievement("toad");
                        const auto& stats = StatisticsTracker::getInstance().getStats();
                        if (stats.totalDeaths == 0) {
                            unlockAchievement("peach");
                            unlockAchievement("no_deaths");
                        }
                    }
                }
                // Reset level counters for the next zone
                m_takenDamageThisLevel = false;
                m_levelTime = 0.0f;
                m_enemiesDefeatedWithShell = 0;
                break;
            }
            default:
                break;
        }
    } catch (...) {
        // Safe catch
    }
}

void AchievementManager::setupDefaultAchievementsList() {
    m_achievements = {
        { "first_stomp", "First Stomp", "Defeat first enemy by stomping", "Boot" },
        { "100_coins", "Coin Collector", "Collect 100 coins in a single run", "Gold coin" },
        { "speed_demon", "Speed Demon", "Complete any level in under 120 seconds", "Clock" },
        { "untouchable", "Untouchable", "Complete any level without taking damage", "Shield" },
        { "combo_king", "Combo King", "Achieve an x8 combo", "Star burst" },
        { "shell_shocker", "Shell Shocker", "Defeat 3 enemies with a single shell kick", "Shell" },
        { "beat_bowser", "Dragon Slayer", "Defeat Bowser", "Crown" },
        { "star_hoarder", "Star Hoarder", "Collect all 9 star coins", "Star" },
        { "no_deaths", "No Deaths", "Complete all 3 levels without dying", "Heart" },
        { "secret_finder", "Secret Finder", "Find all hidden blocks", "Magnifying glass" },
        { "toad", "Toad Unlocked", "Complete all 3 levels", "Toad" },
        { "peach", "Peach Unlocked", "Complete all 3 levels without dying", "Peach" }
    };
}
