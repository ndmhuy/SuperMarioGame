#include "Utils/TileMap.hpp"
#include "Utils/EditorCommands.hpp"
#include "Utils/LevelCatalog.hpp"
#include "Utils/LevelLoader.hpp"
#include "Entities/Entity.hpp"
#include "Entities/EntityCatalogue.hpp"
#include "Entities/EntityFactory.hpp"
#include "Entities/Mushroom.hpp"
#include "Entities/MovingPlatform.hpp"
#include "Entities/Pipe.hpp"
#include "Entities/QuestionBlock.hpp"
#include "Entities/Coin.hpp"
#include <iostream>
#include <cassert>
#include <memory>
#include <vector>
#include <filesystem>
#include "Utils/Serializer.hpp"
#include "TestSaveSandbox.hpp"

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

    // An item, which is one of the sixteen types the old hardcoded if-chain
    // handled.
    PlaceEntityCommand cmd1(entities, EntityType::Mushroom, 64.0f, 96.0f);
    cmd1.execute();
    assert(entities.size() == 1);
    assert(entities[0]->getTypeName() == "mushroom");
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

// The two categories that placed NOTHING at all.
//
// PlaceEntityCommand::execute() used to be an if/else-if chain over sixteen
// hardcoded type-name strings and never called EntityFactory. Every enemy and
// every block in the palette fell off the end of that chain, left `entity`
// null, and the command silently did nothing - no log, no error, no feedback.
// This is the case that would have failed then and passes now.
void testEnemyAndBlockPlacement() {
    std::cout << "Running testEnemyAndBlockPlacement..." << std::endl;

    std::vector<std::unique_ptr<Entity>> entities;

    PlaceEntityCommand goomba(entities, EntityType::Goomba, 32.0f, 64.0f);
    goomba.execute();
    assert(entities.size() == 1);
    assert(entities[0]->getTypeName() == "goomba");

    PlaceEntityCommand question(entities, EntityType::QuestionBlock, 96.0f, 64.0f);
    question.execute();
    assert(entities.size() == 2);
    assert(entities[1]->getTypeName() == "question_block");

    PlaceEntityCommand pipe(entities, EntityType::Pipe, 160.0f, 64.0f);
    pipe.execute();
    assert(entities.size() == 3);
    assert(entities[2]->getTypeName() == "pipe");

    // Undo parks the entity rather than destroying it, so redo puts back the
    // SAME object - which is what keeps a MoveEntityCommand stacked above a
    // Place from holding a dangling pointer.
    Entity* placedPipe = pipe.placedEntity();
    pipe.undo();
    assert(entities.size() == 2);
    pipe.execute();
    assert(entities.size() == 3);
    assert(pipe.placedEntity() == placedPipe);

    std::cout << "testEnemyAndBlockPlacement PASSED!" << std::endl;
}

// Every placeable palette entry, through the command the palette actually uses.
//
// verify_regressions already asserted that EntityFactory::create handles every
// catalogue entry, but placement did not go through the factory - so that test
// passed for the whole time the palette was broken. This one exercises the
// placement path itself.
void testEveryPaletteEntryPlaces() {
    std::cout << "Running testEveryPaletteEntryPlaces..." << std::endl;

    int placed = 0;
    int offered = 0;
    for (EntityCatalogue::Category category : EntityCatalogue::placeableCategories()) {
        for (const EntityCatalogue::Entry* entry : EntityCatalogue::inCategory(category)) {
            ++offered;
            std::vector<std::unique_ptr<Entity>> entities;
            PlaceEntityCommand cmd(entities, entry->type, 64.0f, 64.0f);
            cmd.execute();
            if (entities.size() == 1 && entities[0]->getTypeName() == entry->name) {
                ++placed;
            } else {
                std::cerr << "  palette entry places nothing or the wrong thing: "
                          << entry->name << std::endl;
            }
        }
    }
    std::cout << "  " << placed << " / " << offered << " palette entries place correctly"
              << std::endl;
    assert(offered >= 35);
    assert(placed == offered);

    std::cout << "testEveryPaletteEntryPlaces PASSED!" << std::endl;
}

// Rect fill is one undo step, and does not grow the map by ten columns.
//
// TileMap::setTile auto-expands by gx+10, so a fill that reached the right edge
// through setTile would silently widen the saved level every time.
void testRectFillIsOneStep() {
    std::cout << "Running testRectFillIsOneStep..." << std::endl;

    TileMap tileMap;
    tileMap.initialize(20, 12);

    FillRectCommand fill(tileMap, 2, 3, 5, 6, TileType::Ground);
    fill.execute();
    for (int y = 3; y <= 6; ++y) {
        for (int x = 2; x <= 5; ++x) assert(tileMap.getTileType(x, y) == TileType::Ground);
    }
    assert(tileMap.getWidth() == 20);

    fill.undo();
    for (int y = 3; y <= 6; ++y) {
        for (int x = 2; x <= 5; ++x) assert(tileMap.getTileType(x, y) == TileType::Empty);
    }

    std::cout << "testRectFillIsOneStep PASSED!" << std::endl;
}

// The Inspector's fields, through the command the Inspector issues.
//
// itemType, pipeId/isEntrance/targetLevel/exit and the boss arena have all been
// in the level schema since it was written and none of them were authorable.
void testInspectorPropertiesRoundTrip() {
    std::cout << "Running testInspectorPropertiesRoundTrip..." << std::endl;

    using Property = SetEntityPropertyCommand::Property;

    QuestionBlock block({0.0f, 0.0f}, QuestionBlock::Content::Coin);
    SetEntityPropertyCommand setItem(&block, Property::QuestionBlockItem,
                                     static_cast<float>(QuestionBlock::Content::FireFlower));
    setItem.execute();
    assert(block.getContainedItemType() == QuestionBlock::Content::FireFlower);
    setItem.undo();
    assert(block.getContainedItemType() == QuestionBlock::Content::Coin);

    Pipe pipe({0.0f, 0.0f});
    SetEntityPropertyCommand setEntrance(&pipe, Property::PipeIsEntrance, 1.0f);
    setEntrance.execute();
    assert(pipe.isEntrance());
    SetEntityPropertyCommand setTarget(&pipe, Property::PipeTargetLevel, 0.0f,
                                       "assets/levels/level_1_sub.json");
    setTarget.execute();
    assert(pipe.getTargetLevel() == "assets/levels/level_1_sub.json");
    // The other three warp fields survive a write to the fourth: Pipe exposes
    // them as one operation, and writing one must not blank the rest.
    assert(pipe.isEntrance());
    setTarget.undo();
    assert(pipe.getTargetLevel().empty());
    assert(pipe.isEntrance());

    std::cout << "testInspectorPropertiesRoundTrip PASSED!" << std::endl;
}

// A moved entity is one undoable step, not one per frame of the drag.
void testMoveCommand() {
    std::cout << "Running testMoveCommand..." << std::endl;

    std::vector<std::unique_ptr<Entity>> entities;
    entities.push_back(std::make_unique<Coin>(sf::Vector2f(32.0f, 32.0f)));
    Entity* coin = entities[0].get();

    MoveEntityCommand move(coin, {32.0f, 32.0f}, {160.0f, 64.0f});
    move.execute();
    assert(coin->getPosition().x == 160.0f);
    move.undo();
    assert(coin->getPosition().x == 32.0f);

    std::cout << "testMoveCommand PASSED!" << std::endl;
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
    // Into the sandbox, not a "saves/" relative to whatever directory this
    // binary was launched from - which is how test artefacts ended up beside
    // a developer's real save slots.
    std::string testPath = Serializer::saveDirectory() + "/test_editor_save.json";

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

// The metadata the editor authors has to survive a save/load round trip.
//
// The spawn point was hardcoded to (2, 18) on every save, so a level whose
// spawn had been placed came back with the player somewhere else; and a moving
// platform came back with the loader's four-tile default, which is the D5
// defect the rangeX/rangeY field was added to fix.
void testAuthoredMetadataRoundTrips() {
    std::cout << "Running testAuthoredMetadataRoundTrips..." << std::endl;

    TileMap tileMap;
    tileMap.initialize(30, 20);
    for (int x = 0; x < 30; ++x) tileMap.setTile(x, 19, TileType::Ground);

    std::vector<std::unique_ptr<Entity>> entities;
    entities.push_back(std::make_unique<MovingPlatform>(
        sf::Vector2f(10.0f * 32.0f, 12.0f * 32.0f), sf::Vector2f(2.0f * 32.0f, 0.0f)));

    LevelData meta;
    meta.name = "Authored Round Trip";
    meta.theme = "ice";
    meta.spawnPoint = sf::Vector2f(7.0f * 32.0f, 15.0f * 32.0f);

    LevelLoader loader;
    const std::string path = Serializer::saveDirectory() + "/test_authored_meta.json";
    assert(loader.saveLevel(path, tileMap, entities, meta));

    TileMap back;
    LevelData loaded;
    assert(loader.loadLevel(path, back, loaded));
    assert(loaded.name == "Authored Round Trip");
    assert(loaded.theme == "ice");
    assert(loaded.spawnPoint.x == 7.0f * 32.0f);
    assert(loaded.spawnPoint.y == 15.0f * 32.0f);

    assert(loaded.entities.size() == 1);
    auto* platform = dynamic_cast<MovingPlatform*>(loaded.entities[0].get());
    assert(platform != nullptr);
    // Two tiles, as authored - not the loader's four-tile default.
    assert(platform->getTravelRange().x == 2.0f * 32.0f);

    std::filesystem::remove(path);
    std::cout << "testAuthoredMetadataRoundTrips PASSED!" << std::endl;
}

// Custom levels go beside the shipped ones, never into saves/, and never over a
// file that ships with the game.
void testCustomLevelPaths() {
    std::cout << "Running testCustomLevelPaths..." << std::endl;

    assert(LevelCatalog::customDirectory() == "assets/levels/custom");
    assert(LevelCatalog::toFileStem("My Cool Level!") == "my_cool_level");
    assert(LevelCatalog::toFileStem("") == "untitled_level");

    assert(LevelCatalog::isBuiltIn("assets/levels/level_1.json"));
    assert(LevelCatalog::isBuiltIn("/somewhere/else/bonus_1.json"));
    assert(!LevelCatalog::isBuiltIn("assets/levels/custom/my_cool_level.json"));

    const std::string path = LevelCatalog::customPathFor("My Cool Level");
    assert(path.find("custom") != std::string::npos);
    assert(path.find("my_cool_level.json") != std::string::npos);
    // Not into the save directory: saves/ is per-player progress, and putting
    // level content there is why an authored level could never be loaded.
    assert(path.find(Serializer::saveDirectory()) == std::string::npos);

    std::cout << "testCustomLevelPaths PASSED!" << std::endl;
}

int main() {
    // Every save path in this process now points at a throwaway
    // directory, so nothing here can read or delete real save data
    // (g-rule-13). See TestSaveSandbox.hpp for what went wrong without it.
    TestSaveSandbox sandbox("map_editor");

    testCommands();
    testEntityCommands();
    testEnemyAndBlockPlacement();
    testEveryPaletteEntryPlaces();
    testRectFillIsOneStep();
    testInspectorPropertiesRoundTrip();
    testMoveCommand();
    testSerialization();
    testAuthoredMetadataRoundTrips();
    testCustomLevelPaths();
    std::cout << "\nAll Map Editor verification tests PASSED successfully!" << std::endl;
    return 0;
}
