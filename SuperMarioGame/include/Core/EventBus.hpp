#pragma once

#include <any>
#include <functional>
#include <unordered_map>
#include <vector>

enum class EventType {
  CoinCollected,
  EnemyDefeated,
  PlayerDied,
  PowerUpCollected,
  LevelComplete,
  ComboHit,
  AchievementUnlocked,
  StarCoinCollected,
  PSwitchActivated,
  BossDefeated,
  CheckpointActivated,
  PlayerDamaged,
  BlockBroken,
  TimeWarning,
  GameOver,
  GameStart,
  StateChanged,
  PauseToggled,
  PlayerShotFireball,
  MinimapToggled,
  POWBlockHit,
  ThwompSlam,
  GroundPoundSlam,
  ScreenShakeTriggered
};

struct GameEvent {
  EventType type;
  std::any data;
};

class EventBus {
public:
  using Callback = std::function<void(const GameEvent &)>;
  using SubscriptionId = size_t;

  // Delete copy/move semantics for Singleton
  EventBus(const EventBus &) = delete;
  EventBus &operator=(const EventBus &) = delete;
  EventBus(EventBus &&) = delete;
  EventBus &operator=(EventBus &&) = delete;

  // Singleton Instance
  static EventBus &getInstance();

  // Event operations
  SubscriptionId subscribe(EventType type, Callback callback);
  void unsubscribe(SubscriptionId id);
  void publish(const GameEvent &event);

private:
  EventBus() = default;
  ~EventBus() = default;

  struct Subscription {
    SubscriptionId id;
    Callback callback;
  };

  std::unordered_map<EventType, std::vector<Subscription>> m_subscribers;
  SubscriptionId m_nextId = 0;
};
