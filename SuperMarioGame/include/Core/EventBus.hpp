#pragma once

#include <any>
#include <functional>
#include <unordered_map>
#include <vector>

enum class EventType {
  CoinCollected,
  EnemyDefeated,
  PlayerDied,
  PowerUpCollected,   // a player consumed a power-up (notification)
  PowerUpRequested,   // a block wants an item spawned above it (request)
  EntitySpawnRequested, // an entity wants another entity created (Lakitu, HammerBro)
  LevelComplete,
  ComboHit,
  AchievementUnlocked,
  StarCoinCollected,
  PSwitchActivated,
  BossDefeated,
  CheckpointActivated,
  PlayerWarped,
  PlayerDamaged,
  BlockBroken,
  // A hidden block was just revealed. Distinct from BlockBroken: a hidden block
  // is not destroyed and must not sound or spark like a shattering brick, but it
  // is the only thing the "Secret Finder" achievement is supposed to count.
  // AchievementManager used to infer this from BlockBroken ("here mocked"), so
  // every brick a Super player smashed counted as a secret and no hidden block
  // ever did.
  HiddenBlockFound,
  TimeWarning,
  GameOver,
  GameStart,
  StateChanged,
  PauseToggled,
  PlayerShotFireball,
  MinimapToggled,
  POWBlockHit,
  // The axe at the end of Bowser's bridge was reached. PlayingState drops the
  // bridge span into the lava and takes the boss down with it.
  BridgeChopped,
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

  // RAII handle for a subscription (audit X-7).
  //
  // Every long-lived subscriber currently stores a raw SubscriptionId and has to
  // remember to unsubscribe in its destructor. PlayingState holds nine of them;
  // forgetting one leaves a callback pointing into a destroyed object, which is
  // a use-after-free the next time that event fires. A token cannot be
  // forgotten.
  class ScopedSubscription {
  public:
    ScopedSubscription() = default;
    ScopedSubscription(EventType type, Callback callback);
    ~ScopedSubscription();

    // Move-only: two owners would unsubscribe the same id twice.
    ScopedSubscription(const ScopedSubscription &) = delete;
    ScopedSubscription &operator=(const ScopedSubscription &) = delete;
    ScopedSubscription(ScopedSubscription &&other) noexcept;
    ScopedSubscription &operator=(ScopedSubscription &&other) noexcept;

    void reset();
    bool active() const { return m_active; }
    SubscriptionId id() const { return m_id; }

  private:
    SubscriptionId m_id = 0;
    bool m_active = false;
  };

private:
  EventBus() = default;
  ~EventBus();

  struct Subscription {
    SubscriptionId id;
    Callback callback;
    // Cancelled subscriptions are tombstoned rather than erased, so a handler
    // that unsubscribes while an event is being delivered cannot invalidate the
    // iteration underneath it.
    bool alive = true;
  };

  void compact();

  std::unordered_map<EventType, std::vector<Subscription>> m_subscribers;
  SubscriptionId m_nextId = 0;
  // Non-zero while publish() is on the stack, including re-entrantly: a handler
  // that publishes another event must not let the inner call compact the vector
  // the outer one is walking.
  int m_publishDepth = 0;
  bool m_needsCompaction = false;
};
