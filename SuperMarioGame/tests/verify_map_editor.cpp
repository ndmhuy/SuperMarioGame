#include "Utils/TileMap.hpp"
#include "Utils/EditorCommands.hpp"
#include "Utils/LevelLoader.hpp"
#include "Entities/Entity.hpp"
#include "Entities/Mushroom.hpp"
#include "Entities/Coin.hpp"
#include <iostream>
#include <cassert>
#include <memory>
#include <vector>
#include <filesystem>

void testCommands() {
    std::cout << "Running testCommands..." << std::endl;

    TileMap tileMap;
    tileMap.initialize(10, 10);

    // Place a ground tile at (2, 3)
    PlaceTileCommand cmd1(tileMap, 2, 3, TileType::Ground);
    cmd1.execute();
    assert(tileMap.getTileType(2, 3) == TileType::Ground);

    // Place a brick tile at (2, 3) over ground
    PlaceTileCommand cmd2(tileMap, 2, 3, TileType::Brick);
    cmd2.execute();
    assert(tileMap.getTileType(2, 3) == TileType::Brick);

    // Undo brick placement
    cmd2.undo();
    assert(tileMap.getTileType(2, 3) == TileType::Ground);

    // Undo ground placement
    cmd1.undo();
    assert(tileMap.getTileType(2, 3) == TileType::Empty);

    // Erase command test
    tileMap.setTile(4, 4, TileType::Question);
    EraseTileCommand eraseCmd(tileMap, 4, 4);
    eraseCmd.execute();
    assert(tileMap.getTileType(4, 4) == TileType::Empty);
    
    eraseCmd.undo();
    assert(tileMap.getTileType(4, 4) == TileType::Question);

    std::cout << "testCommands PASSED!" << std::endl;
}

void testEntityCommands() {
    std::cout << "Running testEntityCommands..." << std::endl;

    std::vector<std::unique_ptr<Entity>> entities;

    // Spawn a Mushroom
    PlaceEntityCommand cmd1(entities, "mushroom", 64.0f, 96.0f);
    cmd1.execute();
    assert(entities.size() == 1);
    assert(entities[0]->getPosition().x == 64.0f);
    assert(entities[0]->getPosition().y == 96.0f);

    // Erase the spawned Mushroom
    Entity* target = entities[0].get();
    EraseEntityCommand cmd2(entities, target);
    cmd2.execute();
    assert(entities.empty());

    // Undo erase
    cmd2.undo();
    assert(entities.size() == 1);
    assert(entities[0]->getPosition().x == 64.0f);

    // Undo spawn
    cmd1.undo();
    assert(entities.empty());

    std::cout << "testEntityCommands PASSED!" << std::endl;
}

void testSerialization() {
    std::cout << "Running testSerialization..." << std::endl;

    TileMap tileMap;
    tileMap.initialize(20, 10);
    tileMap.setTile(0, 9, TileType::Ground);
    tileMap.setTile(1, 9, TileType::Ground);
    tileMap.setTile(2, 8, TileType::Brick);

    std::vector<std::unique_ptr<Entity>> entities;
    entities.push_back(std::make_unique<Mushroom>(sf::Vector2f(32.f, 64.f)));
    entities.push_back(std::make_unique<Coin>(sf::Vector2f(64.f, 64.f)));

    LevelLoader loader;
    std::string testPath = "saves/test_editor_save.json";

    // Save
    bool saved = loader.saveLevel(testPath, tileMap, entities, "Test Edit Level");
    assert(saved);
    assert(std::filesystem::exists(testPath));

    // Load back
    TileMap loadedMap;
    LevelData loadedData;
    bool loaded = loader.loadLevel(testPath, loadedMap, loadedData);
    assert(loaded);

    assert(loadedData.name == "Test Edit Level");
    assert(loadedMap.getWidth() == 20);
    assert(loadedMap.getHeight() == 10);
    assert(loadedMap.getTileType(0, 9) == TileType::Ground);
    assert(loadedMap.getTileType(2, 8) == TileType::Brick);

    // Mushroom and Coin should be spawned
    assert(loadedData.entities.size() == 2);
    
    // Cleanup
    std::filesystem::remove(testPath);

    std::cout << "testSerialization PASSED!" << std::endl;
}

int main() {
    testCommands();
    testEntityCommands();
    testSerialization();
    std::cout << "\nAll Map Editor verification tests PASSED successfully!" << std::endl;
    return 0;
}
