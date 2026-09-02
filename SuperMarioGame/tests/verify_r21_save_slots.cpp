// verify_r21_save_slots.cpp — three save slots that are actually three slots,
// and a delete that frees one.
//
// Two shipped defects, both of which reduced three slots to one:
//
//  1. SAVE GAME ALWAYS WROTE SLOT 1. PlayingState registers the pause action as
//     `saveToSlot(Game::getActiveSlot())`, and nothing in the game ever changed
//     the active slot — it is 1 on every launch. So the player had three slots
//     and could only ever write one, and every save silently destroyed the
//     previous one. The pause menu now picks the slot (and sets it) before
//     running that same callback. What is checked here is the layer underneath
//     that choice: slots 2 and 3 are genuinely independent files whose state
//     comes back as its own, which is the property the picker is worthless
//     without.
//
//  2. NO WAY TO DELETE A SAVE. Serializer::deleteSlot() existed but the only
//     callers were the ImGui dev panel and two harness teardowns; nothing a
//     player can reach ever called it, so an occupied slot could never be
//     freed. It is now wired into the LOAD GAME page behind a confirmation.
//
// The path assertion is the important one. A delete that hand-rolls
// "saves/slot_n.json" instead of going through getSaveFilePath()/saveDirectory()
// is the exact bug guard_asset_single_source exists about, and the one time
// this suite resolved a save path itself it deleted the developer's real
// files. So the checks below never name a filename: they observe the directory
// the save actually wrote into and require the delete to have removed exactly
// the file that appeared there. A second path resolution fails that whether it
// removes nothing or removes the wrong thing.
//
// Run via:  ctest -R r21_save_slots --output-on-failure

#include "Core/AchievementManager.hpp"
#include "Core/Game.hpp"
#include "Core/PauseState.hpp"
#include "Core/StatisticsTracker.hpp"
#include "Entities/Mario.hpp"
#include "Entities/Luigi.hpp"
#include "Entities/Player.hpp"
#include "Utils/Serializer.hpp"

#include "TestSaveSandbox.hpp"

#include <SFML/Window/Event.hpp>

#include <filesystem>
#include <iostream>
#include <memory>
#include <set>
#include <string>
#include <vector>

namespace {

int g_failures = 0;
int g_checks   = 0;

void check(bool condition, const std::string& what) {
    ++g_checks;
    std::cout << (condition ? "  [ ok ] " : "  [FAIL] ") << what << "\n";
    if (!condition) ++g_failures;
}

void section(const std::string& title) {
    std::cout << "\n=== " << title << " ===\n";
}

// One key press plus one frame, the way Game::run() feeds a state. Same shape
// as verify_frontend_states' helper, because it is the same contract.
void press(IGameState& state, sf::Keyboard::Key code) {
    sf::Event::KeyPressed pressed;
    pressed.code = code;
    state.handleInput(sf::Event(pressed));
    state.update(1.0f / 60.0f);
}

// Filenames directly inside `dir`. Used to observe where a save landed without
// this harness ever deciding what a slot file is called — see the header
// comment on why that matters.
std::set<std::string> filesIn(const std::filesystem::path& dir) {
    std::set<std::string> names;
    std::error_code ec;
    for (const auto& entry : std::filesystem::directory_iterator(dir, ec)) {
        if (entry.is_regular_file(ec)) names.insert(entry.path().filename().string());
    }
    return names;
}

// --- 1. Three slots are three files -------------------------------------
//
// A save to slot 2 and a save to slot 3 must be distinguishable on disk and
// must load back as themselves. If both had gone to one file the previews
// would agree, and the picker would be offering a choice that does not exist.
void testSlotsAreIndependent(const std::filesystem::path& saveDir) {
    section("slots 2 and 3 are independent");

    std::unique_ptr<Player> two = std::make_unique<Mario>(sf::Vector2f(320.0f, 96.0f));
    two->addScore(4200);
    const std::vector<bool> starsTwo = {true, false, false};
    check(Serializer::saveGame(2, *two, 2, "Underground", 240.0f, 320.0f, 96.0f, starsTwo),
          "saveGame(slot 2) succeeds");

    std::unique_ptr<Player> three = std::make_unique<Luigi>(sf::Vector2f(64.0f, 160.0f));
    three->addScore(9100);
    const std::vector<bool> starsThree = {true, true, true};
    check(Serializer::saveGame(3, *three, 3, "Castle", 180.0f, 64.0f, 160.0f, starsThree),
          "saveGame(slot 3) succeeds");

    // Two distinct files appeared, and both inside the directory Serializer
    // resolved — not one file written twice.
    const std::set<std::string> present = filesIn(saveDir);
    check(present.size() >= 2, "two save files exist after saving two slots");

    const SaveSlotPreview p2 = Serializer::getSlotPreview(2);
    const SaveSlotPreview p3 = Serializer::getSlotPreview(3);
    check(p2.exists, "slot 2 previews as occupied");
    check(p3.exists, "slot 3 previews as occupied");
    check(p2.score == 4200 && p3.score == 9100,
          "each preview reports its own score, not a shared one");
    check(p2.levelId == 2 && p3.levelId == 3,
          "each preview reports its own level");
    check(p2.character == "mario" && p3.character == "luigi",
          "each preview reports its own character");
    check(p2.starCoinsCount == 1 && p3.starCoinsCount == 3,
          "each preview counts its own star coins");

    // And each loads back as itself. A preview reads the same JSON the loader
    // does, so agreeing previews alone would not prove the load path keeps the
    // slots apart.
    std::unique_ptr<Player> loaded;
    int levelId = 0;
    std::string levelName;
    float timeRemaining = 0.0f, checkX = 0.0f, checkY = 0.0f;
    std::vector<bool> stars;

    check(Serializer::loadGame(2, loaded, levelId, levelName, timeRemaining, checkX, checkY, stars),
          "loadGame(slot 2) succeeds");
    check(loaded && loaded->getScore() == 4200 && levelName == "Underground",
          "slot 2 loads back its own score and level");
    check(stars == starsTwo, "slot 2 loads back its own star coins");

    loaded.reset();
    check(Serializer::loadGame(3, loaded, levelId, levelName, timeRemaining, checkX, checkY, stars),
          "loadGame(slot 3) succeeds");
    check(loaded && loaded->getScore() == 9100 && levelName == "Castle",
          "slot 3 loads back its own score and level");
    check(stars == starsThree, "slot 3 loads back its own star coins");

    // Slot 1 was never written. The old behaviour wrote it no matter what was
    // asked for, so this is the check that the fix is not cosmetic.
    check(!Serializer::getSlotPreview(1).exists,
          "slot 1 stays empty when only slots 2 and 3 were saved");
}

// --- 2. The delete frees exactly the file the save wrote ----------------
//
// This is the assertion that catches a hand-rolled path. The save's file is
// identified by watching the directory, so the harness asserts about the file
// Serializer chose rather than one it guessed.
void testDeleteResolvesTheSamePath(const std::filesystem::path& saveDir) {
    section("deleteSlot removes the file saveGame wrote");

    // Start from a slot that is definitely free.
    Serializer::deleteSlot(2);
    const std::set<std::string> before = filesIn(saveDir);
    check(before.find("__none__") == before.end(), "baseline directory listing taken");

    std::unique_ptr<Player> player = std::make_unique<Mario>(sf::Vector2f(200.0f, 128.0f));
    player->addScore(777);
    check(Serializer::saveGame(2, *player, 1, "Overworld", 300.0f, 200.0f, 128.0f, {false, false, false}),
          "saveGame(slot 2) succeeds");

    const std::set<std::string> afterSave = filesIn(saveDir);
    std::vector<std::string> created;
    for (const std::string& name : afterSave) {
        if (before.find(name) == before.end()) created.push_back(name);
    }
    check(created.size() == 1, "the save created exactly one new file in Serializer's own directory");

    check(Serializer::deleteSlot(2), "deleteSlot(2) reports true for an occupied slot");

    const std::set<std::string> afterDelete = filesIn(saveDir);
    if (created.size() == 1) {
        check(afterDelete.find(created.front()) == afterDelete.end(),
              "the file the save created is gone — the delete resolved the same path");
    } else {
        check(false, "cannot identify the saved file, so the path check is inconclusive");
    }
    check(afterDelete == before,
          "nothing else in the save directory was touched");

    check(!Serializer::getSlotPreview(2).exists,
          "the deleted slot previews as EMPTY, which is what the page redraws from");
    check(SaveSlotPreview{}.summary() == "EMPTY",
          "an absent slot summarises as EMPTY on both the load page and the picker");
}

// --- 3. Deleting an empty slot is a reported no-op ----------------------
//
// The LOAD GAME page refuses DEL on an empty row, but the API underneath must
// be honest anyway: the return value is what tells the caller whether anything
// was freed, and it must not claim a success it did not have.
void testDeleteEmptySlotIsHarmless(const std::filesystem::path& saveDir) {
    section("deleting an empty slot");

    Serializer::deleteSlot(1);
    check(!Serializer::getSlotPreview(1).exists, "slot 1 is empty to begin with");

    const std::set<std::string> before = filesIn(saveDir);
    check(!Serializer::deleteSlot(1), "deleteSlot on an empty slot reports false");
    check(filesIn(saveDir) == before, "and removed nothing");

    // Repeating it must stay false rather than becoming true or throwing.
    check(!Serializer::deleteSlot(1), "a second delete of the same empty slot still reports false");
}

// --- 4. Deleting one slot leaves the others alone -----------------------
//
// The player's reason for deleting is to free a slot, not to lose the run in
// the next one. A delete that resolved the wrong path would pass every check
// above about its own slot and fail this.
void testDeleteLeavesNeighboursIntact() {
    section("a delete is confined to its own slot");

    std::unique_ptr<Player> a = std::make_unique<Mario>(sf::Vector2f(32.0f, 32.0f));
    a->addScore(111);
    std::unique_ptr<Player> b = std::make_unique<Mario>(sf::Vector2f(64.0f, 32.0f));
    b->addScore(222);
    std::unique_ptr<Player> c = std::make_unique<Mario>(sf::Vector2f(96.0f, 32.0f));
    c->addScore(333);

    Serializer::saveGame(1, *a, 1, "One",   300.0f, 32.0f, 32.0f, {false, false, false});
    Serializer::saveGame(2, *b, 2, "Two",   300.0f, 64.0f, 32.0f, {false, false, false});
    Serializer::saveGame(3, *c, 3, "Three", 300.0f, 96.0f, 32.0f, {false, false, false});

    check(Serializer::deleteSlot(2), "deleteSlot(2) reports true");
    check(!Serializer::getSlotPreview(2).exists, "slot 2 is now EMPTY");
    check(Serializer::getSlotPreview(1).exists && Serializer::getSlotPreview(1).score == 111,
          "slot 1 survives with its own score");
    check(Serializer::getSlotPreview(3).exists && Serializer::getSlotPreview(3).score == 333,
          "slot 3 survives with its own score");

    // The freed slot is writable again, which is the point of freeing it.
    std::unique_ptr<Player> d = std::make_unique<Luigi>(sf::Vector2f(128.0f, 32.0f));
    d->addScore(444);
    check(Serializer::saveGame(2, *d, 2, "Two Again", 300.0f, 128.0f, 32.0f, {true, false, false}),
          "the freed slot can be saved into again");
    const SaveSlotPreview reused = Serializer::getSlotPreview(2);
    check(reused.exists && reused.score == 444 && reused.character == "luigi",
          "and reads back as the new save, not the deleted one");
}

// --- 5. The summary line both screens share -----------------------------
//
// LOAD GAME and the pause picker must describe a slot identically: the picker's
// only job is to tell the player what they would overwrite, and it is not
// trustworthy if it disagrees with the page they just read the same slot on.
// One formatter is what makes that structurally true, so it is checked here
// rather than left to inspection.
void testSummaryIsShared() {
    section("one slot summary for both screens");

    const SaveSlotPreview absent;
    check(absent.summary() == "EMPTY", "an empty slot reads EMPTY");

    SaveSlotPreview filled;
    filled.exists = true;
    filled.character = "luigi";
    filled.levelId = 3;
    filled.score = 4200;
    filled.starCoinsCount = 2;
    filled.playTime = 187.0f;   // 3:07
    const std::string summary = filled.summary();
    check(summary.find("LUIGI") != std::string::npos, "the character is named, upper-cased");
    check(summary.find("L3") != std::string::npos, "the level is named");
    check(summary.find("4200PT") != std::string::npos, "the score is shown");
    check(summary.find("2/3") != std::string::npos, "the star coins are shown");
    check(summary.find("3:07") != std::string::npos, "the play time is m:ss, zero-padded");
}

// --- 6. The pause menu's slot chooser, driven by the keyboard -----------
//
// PauseState is reachable from main() (PlayingState pushes it on Escape), and
// these are the guarantees the player is owed by a menu that overwrites files:
// SAVE GAME must not write anything by itself, an occupied slot must ask first,
// and ESC must get out without saving. Driven through handleInput() exactly as
// the game does, so none of it is asserted about a method nobody calls.
void testPickerKeyboardFlow() {
    section("the pause menu's slot chooser");

    // A clean board: slot 1 empty, slot 2 occupied.
    Serializer::deleteSlot(1);
    Serializer::deleteSlot(3);
    std::unique_ptr<Player> occupant = std::make_unique<Mario>(sf::Vector2f(48.0f, 64.0f));
    occupant->addScore(5000);
    Serializer::saveGame(2, *occupant, 2, "Underground", 200.0f, 48.0f, 64.0f,
                         {true, false, false});

    // The callback stands in for PlayingState's saveToSlot(getActiveSlot()) —
    // the real one is registered at PlayingState.cpp's pause push. Recording
    // the active slot at call time is the whole contract between them: the
    // picker communicates the chosen slot by setting it before calling.
    int savedToSlot = 0;
    int saveCalls = 0;
    auto onSave = [&savedToSlot, &saveCalls]() {
        savedToSlot = Game::getInstance().getActiveSlot();
        ++saveCalls;
    };

    Game::getInstance().setActiveSlot(1);

    {
        PauseState pause([]() {}, onSave, []() {});
        // Down from RESUME lands on SAVE GAME.
        press(pause, sf::Keyboard::Key::Down);
        press(pause, sf::Keyboard::Key::Enter);
        check(saveCalls == 0, "SAVE GAME opens the chooser instead of writing a slot");

        // The chooser opens on the active slot (1), which is empty — nothing to
        // destroy, so it saves without a confirmation step.
        press(pause, sf::Keyboard::Key::Enter);
        check(saveCalls == 1 && savedToSlot == 1,
              "choosing an empty slot saves straight away");
    }

    // Now the destructive case: pick the occupied slot 2 and confirm.
    Game::getInstance().setActiveSlot(1);
    saveCalls = 0;
    savedToSlot = 0;
    {
        PauseState pause([]() {}, onSave, []() {});
        press(pause, sf::Keyboard::Key::Down);      // SAVE GAME
        press(pause, sf::Keyboard::Key::Enter);     // chooser, on slot 1
        press(pause, sf::Keyboard::Key::Down);      // slot 2 — occupied
        press(pause, sf::Keyboard::Key::Enter);
        check(saveCalls == 0, "an occupied slot asks before it is overwritten");

        // The confirm page defaults to KEEP, so Enter pressed out of habit
        // cancels rather than destroys.
        press(pause, sf::Keyboard::Key::Enter);
        check(saveCalls == 0, "the confirmation defaults to keeping the save");

        // Back on the chooser, still on slot 2. Down onto OVERWRITE, then
        // confirm for real.
        press(pause, sf::Keyboard::Key::Enter);     // ask again
        press(pause, sf::Keyboard::Key::Down);      // OVERWRITE
        press(pause, sf::Keyboard::Key::Enter);
        check(saveCalls == 1 && savedToSlot == 2,
              "confirming writes the chosen slot, not the active one");
    }

    // ESC out of the chooser saves nothing, and leaves the active slot alone.
    Game::getInstance().setActiveSlot(1);
    saveCalls = 0;
    {
        PauseState pause([]() {}, onSave, []() {});
        press(pause, sf::Keyboard::Key::Down);      // SAVE GAME
        press(pause, sf::Keyboard::Key::Enter);     // chooser
        press(pause, sf::Keyboard::Key::Down);      // slot 2
        press(pause, sf::Keyboard::Key::Escape);    // cancel the chooser
        check(saveCalls == 0, "ESC out of the chooser saves nothing");
        check(Game::getInstance().getActiveSlot() == 1,
              "and does not move the active slot on its way out");
    }

    // ESC out of the confirmation drops back to the chooser rather than saving.
    saveCalls = 0;
    {
        PauseState pause([]() {}, onSave, []() {});
        press(pause, sf::Keyboard::Key::Down);      // SAVE GAME
        press(pause, sf::Keyboard::Key::Enter);     // chooser, slot 1
        press(pause, sf::Keyboard::Key::Down);      // slot 2 — occupied
        press(pause, sf::Keyboard::Key::Enter);     // confirmation
        press(pause, sf::Keyboard::Key::Escape);    // back to the chooser
        check(saveCalls == 0, "ESC out of the confirmation saves nothing");
        // Proof it landed on the chooser and not the pause menu: Enter here
        // asks about slot 2 again instead of re-opening the chooser.
        press(pause, sf::Keyboard::Key::Enter);
        check(saveCalls == 0, "and lands back on the chooser, not on a save");
    }
}

} // namespace

int main() {
    // Every save path in this process points at a throwaway directory, so
    // nothing here can read or delete real save data (g-rule-13). This harness
    // needs it more than most: its whole subject is a function that deletes
    // save files. See TestSaveSandbox.hpp for what went wrong without it.
    TestSaveSandbox sandbox("r21_save_slots");

    // saveGame() serializes the statistics and achievement singletons, so they
    // have to exist before the first save.
    StatisticsTracker::getInstance().init();
    AchievementManager::getInstance().init();

    // Everything below observes this directory rather than naming files in it.
    // It is also proof the sandbox took: if setSaveDirectory() had not been
    // honoured, saveDirectory() would be the repo's real saves/ and this
    // harness would be about to delete from it.
    const std::filesystem::path saveDir = Serializer::saveDirectory();
    std::cout << "Save directory under test: " << saveDir << "\n";
    if (saveDir != sandbox.path()) {
        std::cerr << "FATAL: Serializer is not pointing at the sandbox (" << sandbox.path()
                  << "). Refusing to run a delete test against " << saveDir << ".\n";
        return 1;
    }

    testSlotsAreIndependent(saveDir);
    testDeleteResolvesTheSamePath(saveDir);
    testDeleteEmptySlotIsHarmless(saveDir);
    testDeleteLeavesNeighboursIntact();
    testSummaryIsShared();
    testPickerKeyboardFlow();

    std::cout << "\n" << (g_checks - g_failures) << "/" << g_checks << " checks passed\n";
    if (g_failures > 0) {
        std::cout << "verify_r21_save_slots FAILED (" << g_failures << " failures)\n";
        return 1;
    }
    std::cout << "verify_r21_save_slots PASSED\n";
    return 0;
}
