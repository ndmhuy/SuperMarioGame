# CS202 Final Project: Super Mario Bros.

## 1. Reference Games
* **Super Mario Bros. (1985)** (NES)
* **New Super Mario Bros.** (DS)

## 2. Project Description

### a) Objectives
* Develop a 2D Mario-style game using C++ with an emphasis on Object-Oriented Programming (OOP) principles.
* Implement inheritance, encapsulation, polymorphism, and abstraction.
* Incorporate at least 5 design patterns (e.g., Factory, Singleton, Observer) to create a modular, extensible, and well-structured game.

### b) Requirements

**Inheritance and Polymorphism:**
* **Character System:** * Create a base `Character` class defining common attributes/methods (position, movement, jump).
    * Derive specific classes like `Mario`, `Luigi`, and `Enemy`.
    * Implement unique behaviors (e.g., Mario grows with power-ups, enemies have distinct movement patterns).
* **Items and Power-ups:**
    * Create a base `Item` class with methods like `activate()` and `collect()`.
    * Derive classes like `Mushroom`, `Coin`, `FireFlower`, etc.
    * Ensure each item interacts with the player differently (affecting size, speed, or abilities).
* **Polymorphism:**
    * Handle different characters and items in a unified way (e.g., use an array/vector of `Character` pointers).

**Game Design Patterns:**
* **Factory Pattern:** Instantiate characters and items dynamically based on the level.
* **Singleton Pattern:** Manage the game state or sound controller to ensure only one instance exists.
* *Plus at least 3 other patterns of your choice.*

**Levels:**
* Develop 3 levels with increasing difficulty (can mirror the sample games).

**User Input and Interaction:**
* Capture keyboard/controller inputs for walking, jumping, and interacting.
* Implement collision detection for enemies, items, and level boundaries.

**Environment, Graphics and Sound:**
* 2D/3D graphics game.
* Implement basic sound effects (jumping, collecting items, defeating enemies).
* **Suggested Game Engines/Libraries:**
    * SFML: https://www.sfml-dev.org/learn.php
    * SDL: https://www.libsdl.org/
    * raylib: https://www.raylib.com/games.html
    * Box2d: https://github.com/erincatto/box2d

**Game State Management:**
* Manage states (Start, Pause, End) using OOP principles.
* Store/update player progress (score, remaining lives).
* **Save/Load Game:** Implement file handling in C++ to save/load progress.

### c) Advanced & Bonus Features

**Advanced Features:**
* **AI for Enemies:** Basic AI for enemies (Goombas, Koopas) to move automatically and interact based on proximity to Mario.
* **Multiple Player Characters:** Switch between characters (Mario, Luigi) with different abilities (e.g., Luigi jumps higher but runs slower). Add a character selection screen.

**Bonus Features:**
* **Level Editor:** Allow players to design their own levels, serialize them, save to files, and load them later.

### d) Deliverables
1.  **Source Code:** Well-documented C++ code.
2.  **Design Documentation:**
    * Class diagrams.
    * Sequence diagrams explaining design patterns (optional).
    * Description of OOP principles and design pattern applications.
3.  **Demo Video.**

### e) Evaluation Criteria
* Correct/efficient implementation of OOP concepts.
* Effective use of 5 design patterns.
* Code readability, modularity, and C++ standards adherence.
* Functionality and playability.
* Creativity and originality.

## 3. Rubric
**Functionality (65 points)**
* Player Inputs, Movement and Collision (20 pts)
* Enemy Behavior (10 pts)
* Power-Ups and Items (10 pts)
* 3 Level Completion (15 pts)
* Sounds: effect, background (10 pts)

**Design and Implementation (35 points)**
* Object-Oriented Design (10 pts)
* Check 5 design patterns (25 pts)

**Additional Requirements (15 points)**
* AI (5 pts)
* Multiple players (5 pts)
* 3D Game (5 pts)

*Google Sheet Link:* [Rubric Sheet](https://docs.google.com/spreadsheets/d/11P1AgDmAio1BZS3azFMiLdnxrxHj0iNQifvrhir50/edit?usp=sharing)

---