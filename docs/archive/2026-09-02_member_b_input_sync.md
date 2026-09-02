# ISSUE: Sync Game Keybindings Settings with InputManager Mappings

## Status
* **Assignee**: Member B (Input, Sound & Gameplay Systems)
* **Priority**: High
* **Files Involved**:
  * [InputManager.hpp](../../SuperMarioGame/include/Core/InputManager.hpp)
  * [InputManager.cpp](../../SuperMarioGame/src/Core/InputManager.cpp)
  * [Game.hpp](../../SuperMarioGame/include/Core/Game.hpp)
  * [Game.cpp](../../SuperMarioGame/src/Core/Game.cpp)

---

## 1. Problem Description

Currently, there is a disconnect between the **configuration layer** (`Game`) and the **execution layer** (`InputManager`):
1. `Game` loads settings from config (e.g. `settings.json` slot via `Serializer::loadSettings`) and stores them inside `m_keyBindings` as string maps (e.g., `{"jump": "W", "left": "A", "right": "D", "fire": "F"}`).
2. However, `InputManager::loadDefaultBindings()` hardcodes Player 1 keys using SFML key enums directly:
   ```cpp
   m_pressMappings[0][sf::Keyboard::Key::W] = compositeJumpCmd;
   m_holdMappings[0][sf::Keyboard::Key::A] = leftCmd;
   ```
3. When custom bindings are loaded or changed in the pause menu (by calling `Game::setKeyBinding`), the `InputManager` is **never updated**. The player continues to control the character using the hardcoded default keys, ignoring the loaded/saved key configurations.

---

## 2. Requirements

To make the input mappings fully dynamic and synchronized, implement the following steps:

### A. Create String-to-Key Mapper
Inside `InputManager.cpp`, implement a helper function that converts a string representation of a key into the corresponding `sf::Keyboard::Key` enum value:
```cpp
static sf::Keyboard::Key stringToKey(const std::string& str) {
    if (str == "W" || str == "w") return sf::Keyboard::Key::W;
    if (str == "A" || str == "a") return sf::Keyboard::Key::A;
    if (str == "S" || str == "s") return sf::Keyboard::Key::S;
    if (str == "D" || str == "d") return sf::Keyboard::Key::D;
    if (str == "F" || str == "f") return sf::Keyboard::Key::F;
    if (str == "Space" || str == "space") return sf::Keyboard::Key::Space;
    if (str == "LShift" || str == "lshift") return sf::Keyboard::Key::LShift;
    // (Optional: add Arrow keys and others if Player 1 supports them)
    return sf::Keyboard::Key::Unknown;
}
```

### B. Implement Dynamic Binding Method
Add a public method to `InputManager`:
```cpp
// include/Core/InputManager.hpp
void updateBindingsFromGame();
```
In the implementation (`src/Core/InputManager.cpp`), perform the following:
1. Re-initialize defaults by calling `loadDefaultBindings()`.
2. Retrieve the string map from `Game::getInstance().getKeyBindings()`.
3. If the settings are configured, clear P1 default mappings (`m_pressMappings[0].clear()`, `m_holdMappings[0].clear()`).
4. Read `"jump"`, `"left"`, `"right"`, `"fire"` settings and dynamically map the parsed enums to command objects (e.g. `CompositeCommand`, `MoveLeftCommand`, `MoveRightCommand`, `FireCommand`).
5. Re-bind auxiliary keys like crouch (`S`), ground pound (`S`), and run (`LShift`) if they are not overridden.

### C. Add Synchronization Hooks
Wire notifications so that when bindings change, the mappings are updated:
1. **At Initialization**: Call `InputManager::getInstance().updateBindingsFromGame();` inside `Game::run()` immediately after `Serializer::loadSettings()` on line 24.
2. **At Modification**: Call `InputManager::getInstance().updateBindingsFromGame();` inside `Game::setKeyBinding()` whenever a key is edited.

---

## 3. Verification Plan

Verify the synchronization by running the settings suite:
1. Compile the verification target: `cd build && make verify_save_load`.
2. In the test suite, save custom settings:
   ```cpp
   std::unordered_map<std::string, std::string> bindings = { {"jump", "Space"}, {"left", "Left"}, {"right", "Right"} };
   Serializer::saveSettings(..., bindings, ...);
   ```
3. Load settings in `Game` and assert that `InputManager::getInstance().updateBindingsFromGame()` successfully maps the new enums (e.g., checks that pressing `Space` triggers `jump`).
