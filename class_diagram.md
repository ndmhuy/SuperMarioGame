# Super Mario Game - Full Codebase Class Diagrams

This document contains the complete class structure of the `SuperMarioGame` codebase, separated into logical subdiagrams for easy reading and printing. 

## 1. Core Infrastructure & Managers
This diagram covers the Singleton managers, the game loop, and the state machine.

```mermaid
classDiagram
    class Game {
        -sf::RenderWindow m_window
        -GameStateManager m_gsm
        -bool m_isRunning
        +getInstance() Game$
        +run() void
        +quit() void
        +pushState(std::unique_ptr~IGameState~ state) void
        +popState() void
        +changeState(std::unique_ptr~IGameState~ state) void
        -initWindow() void
        -initImGui() void
        -shutdown() void
    }

    class GameStateManager {
        -std::vector~GameStatePtr~ m_states
        +pushState(std::unique_ptr~IGameState~ state) void
        +popState() void
        +changeState(std::unique_ptr~IGameState~ state) void
        +handleInput(sf::Event event) void
        +update(float dt) void
        +render(sf::RenderTarget target) void
    }

    class IGameState {
        <<interface>>
        +init() void*
        +handleInput(sf::Event event) void*
        +update(float dt) void*
        +render(sf::RenderTarget target) void*
        +pause() void*
        +resume() void*
    }

    class MenuState {
        +init() void
        +handleInput(sf::Event event) void
        +update(float dt) void
        +render(sf::RenderTarget target) void
        +pause() void
        +resume() void
    }

    class PlayingState {
        -PhysicsEngine m_physics
        -TileMap m_tileMap
        -std::vector~EntityPtr~ m_entities
        +init() void
        +handleInput(sf::Event event) void
        +update(float dt) void
        +render(sf::RenderTarget target) void
        +pause() void
        +resume() void
    }

    class ResourceManager {
        -std::unordered_map~string, sf::Texture~ m_textures
        -std::unordered_map~string, sf::Font~ m_fonts
        -std::unordered_map~string, sf::SoundBuffer~ m_soundBuffers
        +getInstance() ResourceManager$
        +loadTexture(string id, string path) bool
        +getTexture(string id) sf::Texture
        +loadFont(string id, string path) bool
        +getFont(string id) sf::Font
        +loadSoundBuffer(string id, string path) bool
        +getSoundBuffer(string id) sf::SoundBuffer
        +clear() void
    }

    class SoundManager {
        -sf::Music m_music
        -std::vector~sf::Sound~ m_soundPool
        -float m_sfxVolume
        -float m_musicVolume
        +getInstance() SoundManager$
        +playSound(string id) void
        +playMusic(string id, bool loop) void
        +pauseMusic() void
        +resumeMusic() void
        +shutdown() void
        +setSFXVolume(float volume) void
        +setMusicVolume(float volume) void
    }

    class EventBus {
        -std::unordered_map~EventType, SubscriptionList~ m_subscribers
        -SubscriptionId m_nextId
        +getInstance() EventBus$
        +subscribe(EventType type, Callback callback) SubscriptionId
        +unsubscribe(SubscriptionId id) void
        +publish(GameEvent event) void
    }

    Game --> GameStateManager
    GameStateManager o-- IGameState
    IGameState <|-- MenuState
    IGameState <|-- PlayingState
```

---

## 2. Input & Command System
This diagram covers the `InputManager` and the Command pattern implementation.

```mermaid
classDiagram
    class InputManager {
        -Character m_players
        -std::unordered_map~Key, CommandPtr~ m_pressMappings
        -std::unordered_map~Key, CommandPtr~ m_holdMappings
        +getInstance() InputManager$
        +handleInput(sf::Event event, Character character) void
        +update(Character character) void
        +registerPlayer(Character character, int playerIndex) void
        -loadDefaultBindings() void
    }

    class ICommand {
        <<interface>>
        +execute(Character character) void*
    }

    class CompositeCommand {
        -std::vector~CommandPtr~ m_commands
        +addCommand(std::shared_ptr~ICommand~ cmd) void
        +execute(Character character) void
    }

    class JumpCommand { +execute(Character character) void }
    class MoveLeftCommand { +execute(Character character) void }
    class MoveRightCommand { +execute(Character character) void }
    class FireCommand { +execute(Character character) void }
    class RunCommand { +execute(Character character) void }
    class CrouchCommand { +execute(Character character) void }
    class GroundPoundCommand { +execute(Character character) void }
    class WallJumpCommand { +execute(Character character) void }

    InputManager --> ICommand
    ICommand <|-- CompositeCommand
    ICommand <|-- JumpCommand
    ICommand <|-- MoveLeftCommand
    ICommand <|-- MoveRightCommand
    ICommand <|-- FireCommand
    ICommand <|-- RunCommand
    ICommand <|-- CrouchCommand
    ICommand <|-- GroundPoundCommand
    ICommand <|-- WallJumpCommand
    CompositeCommand o-- ICommand
```

---

## 3. Entity Hierarchy
This diagram maps the core `Entity` tree, showing abstract bases and specific concrete classes.

```mermaid
classDiagram
    class Entity {
        <<abstract>>
        #sf::Vector2f position
        #sf::Vector2f velocity
        #AABB boundingBox
        #bool active
        +update(float dt) void*
        +render(sf::RenderTarget target) void*
        +getBoundingBox() AABB
        +getPosition() sf::Vector2f
        +getVelocity() sf::Vector2f
        +isActive() bool
    }

    class Character {
        <<abstract>>
        #int health
        #float speed
        #float jumpForce
        #bool onGround
        #bool onWall
        #bool facingRight
        +moveLeft() void*
        +moveRight() void*
        +jump() void*
        +takeDamage(int amount) void*
        +getHealth() int
        +getSpeed() float
        +getJumpForce() float
        +isOnGround() bool
        +isOnWall() bool
        +isFacingRight() bool
    }

    class Player {
        #std::unique_ptr~IPlayerState~ m_currentState
        #int lives
        #int coins
        #int score
        #float invincibilityTimer
        #int coyoteFramesLeft
        #int jumpBufferFramesLeft
        #int comboCounter
        +run() void*
        +wallJump() void*
        +groundPound() void*
        +crouch() void*
        +slide() void*
        +shootFireball() void*
        +powerUp(int itemType) void
        +powerDown() void
        +getCurrentState() IPlayerState
        +changeState(std::unique_ptr~IPlayerState~ state) void
        +addCoins(int amount) void
        +addScore(int amount) void
        +gainLife() void
        +loseLife() void
        +resetCombo() void
        +incrementCombo() void
        +getLives() int
        +getCoins() int
        +getScore() int
    }

    class Enemy {
        <<abstract>>
        +onStomped() void*
        +onHitByFireball() void*
    }

    class Item {
        <<abstract>>
        #bool collected
        +activate(Player player) void*
        +collect() void*
        +isCollected() bool
    }

    class Block {
        <<abstract>>
        +onHitFromBelow(Player player) void*
    }

    Entity <|-- Character
    Entity <|-- Item
    Entity <|-- Block
    Character <|-- Player
    Character <|-- Enemy

    Player <|-- Mario
    Player <|-- Luigi
    Player <|-- Peach
    Player <|-- Toad

    Item <|-- Mushroom
    Item <|-- FireFlower
    Item <|-- Coin
    Item <|-- Star
    Item <|-- OneUpMushroom
    Item <|-- CapeFeather
    Item <|-- MegaMushroom
    Item <|-- MiniMushroom
```

---

## 4. Player State Machine (State + Decorator Patterns)
This diagram maps the complex state behaviors of the player.

```mermaid
classDiagram
    class IPlayerState {
        <<interface>>
        +enter(Player player) void*
        +exit(Player player) void*
        +handleInput(Player player, sf::Event event) void*
        +update(Player player, float dt) void*
        +getSize() sf::Vector2f*
    }

    class SmallState { +update(Player player, float dt) void }
    class SuperState { +update(Player player, float dt) void }
    class FireState { +update(Player player, float dt) void }
    class CapeState { +update(Player player, float dt) void }
    class MiniState { +update(Player player, float dt) void }

    class PlayerStateDecorator {
        <<abstract>>
        #std::unique_ptr~IPlayerState~ m_wrappedState
        +enter(Player player) void
        +exit(Player player) void
        +handleInput(Player player, sf::Event event) void
        +update(Player player, float dt) void
        +getSize() sf::Vector2f
    }

    class StarDecorator {
        +update(Player player, float dt) void
    }

    class MegaDecorator {
        +update(Player player, float dt) void
        +getSize() sf::Vector2f
    }

    IPlayerState <|-- SmallState
    IPlayerState <|-- SuperState
    IPlayerState <|-- FireState
    IPlayerState <|-- CapeState
    IPlayerState <|-- MiniState
    IPlayerState <|-- PlayerStateDecorator
    PlayerStateDecorator <|-- StarDecorator
    PlayerStateDecorator <|-- MegaDecorator
    PlayerStateDecorator o-- IPlayerState : "Wraps"
```

---

## 5. Physics Engine
This diagram covers the collision detection and resolution modules.

```mermaid
classDiagram
    class PhysicsEngine {
        -CollisionDetector m_detector
        -CollisionResolver m_resolver
        -SpatialHash m_spatialHash
        +update(std::vector~EntityPtr~ entities, TileMap tileMap, float dt) void
        -applyGravity(Entity entity, float dt) void
        -integrateVelocity(Entity entity, float dt) void
    }

    class CollisionDetector {
        +checkEntityVsEntity(Entity e1, Entity e2) CollisionInfo
        +checkEntityVsTileMap(Entity entity, TileMap tileMap) std::vector~CollisionInfo~
    }

    class CollisionResolver {
        +resolveEntityVsTile(Entity entity, CollisionInfo info) void
        +resolveEntityVsEntity(Entity e1, Entity e2, CollisionInfo info) void
        +resolvePlayerVsEnemy(Player player, Enemy enemy, CollisionInfo info) void
        +resolvePlayerVsItem(Player player, Item item, CollisionInfo info) void
        +resolvePlayerVsPlayer(Player p1, Player p2, CollisionInfo info) void
    }

    class SpatialHash {
        -std::unordered_map~GridCoord, EntityList~ m_grid
        +insert(Entity entity, AABB box) void
        +clear() void
        +query(AABB box) std::vector~EntityPtr~
    }

    class AABB {
        +float x
        +float y
        +float width
        +float height
        +intersects(AABB other) bool
        +getOverlap(AABB other) AABB
        +contains(float px, float py) bool
        +getCenter() sf::Vector2f
    }

    class CollisionInfo {
        <<struct>>
        +bool collided
        +sf::Vector2f overlap
        +sf::Vector2f normal
        +Entity other
    }

    PhysicsEngine --> CollisionDetector
    PhysicsEngine --> CollisionResolver
    PhysicsEngine --> SpatialHash
    CollisionDetector ..> CollisionInfo
    CollisionResolver ..> CollisionInfo
    SpatialHash ..> AABB
```

---

## 6. Utilities & Graphics
This diagram covers level data, tiles, and rendering abstractions.

```mermaid
classDiagram
    class TileMap {
        -int m_width
        -int m_height
        -std::vector~TileRow~ m_grid
        +getInfo(TileType type) TileInfo$
        +render(sf::RenderTarget target, Camera camera) void
        +initialize(int width, int height) void
        +setTile(int gx, int gy, TileType type) void
        +getTileAt(float px, float py) TileType
        +worldToGrid(float px, float py) sf::Vector2i
        +gridToWorld(int gx, int gy) sf::Vector2f
        +getTileSurfaceType(float px, float py) TileType
        +swapBricksAndCoins() void
    }

    class TileInfo {
        <<struct>>
        +TileType type
        +bool isSolid
        +sf::Color debugColor
        +string name
    }

    class Camera {
        -sf::View m_view
        -AABB m_bounds
        -sf::Vector2f m_position
        -float m_shakeIntensity
        -float m_shakeDuration
        +follow(sf::Vector2f target, float dt) void
        +setBounds(AABB bounds) void
        +getView() sf::View
        +getVisibleBounds() AABB
        +triggerScreenShake(float intensity, float duration) void
        +update(float dt) void
    }

    class LevelLoader {
        +loadFromFile(string path, TileMap map, std::vector~EntityPtr~ entities) bool$
        +loadFromSave(string path, TileMap map, std::vector~EntityPtr~ entities) bool$
    }

    TileMap ..> TileInfo
```
