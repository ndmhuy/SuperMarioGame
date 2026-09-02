// verify_r21_ui_layout.cpp — R21 defects 6 and 12.
//
// Defect 6 was text printing straight through panel borders on six screens, and
// off the window entirely in the HUD. The fix is a shared fitting mechanism in
// UiRenderer rather than per-screen tuning, so this harness tests the mechanism
// and then the one layout that was worst — the LOAD GAME slot summary, which
// ran 81 to 153 px past the right edge of its frame on every save that existed.
//
// Defect 12 was six ImGui developer windows with no flag, no compile guard and
// no keybinding, drawn in every state of a release build. The fix is a
// persisted, default-off Game::m_debugMode with an OPTIONS row; this harness
// checks the default, the round trip, and that the row drives the right flag.
//
// Run via:  ctest -R r21_ui_layout --output-on-failure
//
// Reachability note: the fitting functions are called from MenuState,
// OptionsState, PauseState, CharacterSelectState and WorldMapState render
// paths, and m_debugMode gates Game::run()'s Dev Tools block. This harness
// proves the arithmetic and the persistence, not that the screens are reached.

#include "Core/Game.hpp"
#include "Core/OptionsState.hpp"
#include "Core/ResourceManager.hpp"
#include "Core/SoundManager.hpp"
#include "Graphics/UiRenderer.hpp"
#include "Utils/Constants.hpp"
#include "Utils/Serializer.hpp"

#include <SFML/Graphics/Image.hpp>
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "TestSaveSandbox.hpp"

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

sf::Event keyEvent(sf::Keyboard::Key code) {
    sf::Event::KeyPressed pressed;
    pressed.code = code;
    return sf::Event(pressed);
}

void step(IGameState& state, sf::Keyboard::Key code) {
    state.handleInput(keyEvent(code));
    state.update(1.0f / 60.0f);
}

// The two font properties fitCharSize() actually relies on, pinned here so a
// font swap fails by name instead of as text through a border six screens away.
//
// PROPERTY 1 (relied on): the font is monospace — every glyph at a given size
// advances the same width, whatever the character.
//
// PROPERTY 2 (deliberately NOT relied on): that the advance equals the
// character size. PressStart2P is one em per advance by design (unitsPerEm
// 1000, every advance 1000), but FreeType hints the advance to whole pixels and
// does not always round down: this font measures 13px per character at size 12
// and 21px at size 20. floor(maxWidth / length) is therefore an estimate that
// can be too large, and fitCharSize is only correct because it verifies that
// estimate against measureTextWidth. The section below asserts both halves.
float advancePerChar(unsigned int size) {
    // Difference of two lengths cancels the last glyph's bearing, so this is the
    // pure advance rather than the ink width.
    return UiRenderer::measureTextWidth(std::string(21, 'M'), size) -
           UiRenderer::measureTextWidth(std::string(11, 'M'), size);
}

void testTheFontMetricFitCharSizeAssumes() {
    section("defect 6 — the font properties fitCharSize relies on");

    for (unsigned int size : {8u, 11u, 12u, 15u, 20u, 24u}) {
        const float advance = advancePerChar(size) / 10.0f;

        // Monospace, stated the way it is actually usable. getLocalBounds()
        // reports ink, not advance, so two 14-character strings are NOT
        // bit-identical — at size 12, "MARIO  L1  0PT" is 172px and 14 M's are
        // 180px. What IS true for every string is that its width never exceeds
        // length * advance, and that is the half the estimate depends on: the
        // content cannot make a string wider than its cell count. A proportional
        // font would break this immediately.
        bool boundedByCells = true;
        for (const char* sample : {"MARIO  L1  0PT", "MMMMMMMMMMMMMM",
                                   "99999  9/3  0:", "WWWWWWWWWWWWWW"}) {
            if (UiRenderer::measureTextWidth(sample, size) > 14.0f * advance) {
                boundedByCells = false;
            }
        }
        check(boundedByCells,
              "at size " + std::to_string(size) +
              ", no 14-character string exceeds 14 cells, whatever it is made of");

        // And the advance is uniform along the string, so width is linear in the
        // character count — which is what makes a single division meaningful.
        bool linear = true;
        for (std::size_t n = 1; n <= 40; ++n) {
            const float expected = UiRenderer::measureTextWidth(std::string(1, 'M'), size) +
                                   advance * static_cast<float>(n - 1);
            if (std::fabs(UiRenderer::measureTextWidth(std::string(n, 'M'), size) - expected) > 0.5f) {
                linear = false;
            }
        }
        check(linear, "and width is linear in the character count over 1-40 characters");
    }

    // The half the brief got wrong, recorded so nobody re-derives fitCharSize as
    // a pure closed form and quietly reintroduces the overflow at size 12 — the
    // size the LOAD GAME page draws its slot summaries at.
    check(advancePerChar(12) / 10.0f > 12.0f,
          "size 12 hints UP to more than 12px per character, so floor(W/n) alone "
          "OVERESTIMATES what fits and must be verified against the real metric");
    check(std::fabs(advancePerChar(24) / 10.0f - 24.0f) <= 0.01f,
          "while size 24 lands on its nominal advance — the rounding is per size, "
          "not a constant fudge factor");
}

void testFitCharSizeArithmetic() {
    section("defect 6 — fitCharSize clamps between minimum and preferred");

    check(UiRenderer::fitCharSize("MARIO", 12, 400.0f) == 12,
          "a string with room to spare keeps the preferred size");

    // 5 characters at size 20 measure 101px on this font, not the 97.5px the
    // nominal advance predicts, so the honest answer here is 19 — and the naive
    // closed form would have said 20 and overflowed by a pixel.
    check(UiRenderer::fitCharSize("MARIO", 20, 100.0f) == 19,
          "a string that just misses shrinks by one, against the measured metric "
          "rather than the nominal one");

    check(UiRenderer::fitCharSize(std::string(10, 'M'), 20, 100.0f) <= 10,
          "a string that does not fit shrinks to at most floor(maxWidth / length)");

    check(UiRenderer::fitCharSize(std::string(40, 'M'), 20, 100.0f) == 8,
          "and stops at the floor of 8 rather than shrinking to 2 and vanishing");

    check(UiRenderer::fitCharSize("OK", 12, 4000.0f) == 12,
          "a huge box does not inflate the text past the preferred size");

    check(UiRenderer::fitCharSize("ANYTHING", 13, 0.0f) == 13,
          "maxWidth <= 0 means unbounded, which is what an un-migrated caller gets");

    check(UiRenderer::fitCharSize("SHORT", 9, 200.0f, 12) == 9,
          "a minimum above the preferred size does not invert the clamp");

    // The property that matters, over the whole range the UI actually uses:
    // above the floor, the returned size always fits.
    bool alwaysFits = true;
    bool neverGrows = true;
    for (unsigned int preferred : {9u, 11u, 12u, 13u, 15u, 16u, 24u}) {
        for (std::size_t n = 1; n <= 60; ++n) {
            for (float box : {60.0f, 130.0f, 290.0f, 424.0f, 560.0f}) {
                const std::string text(n, 'W');
                const unsigned int fitted = UiRenderer::fitCharSize(text, preferred, box);
                if (fitted > preferred) neverGrows = false;
                if (fitted > 8 && UiRenderer::measureTextWidth(text, fitted) > box) {
                    alwaysFits = false;
                }
            }
        }
    }
    check(neverGrows, "over 2,100 combinations the result never exceeds the preferred size");
    check(alwaysFits, "and above the floor it always measures inside the box");
}

// The string LOAD GAME actually builds, in the shape MenuState::formatSlotSummary
// produces: "<CHARACTER>  L<level>  <score>PT  STARS <n>/3  <m:ss>".
std::string slotSummary(const std::string& character, int level, long score,
                        int stars, const std::string& time) {
    return character + "  L" + std::to_string(level) + "  " + std::to_string(score) +
           "PT  STARS " + std::to_string(stars) + "/3  " + time;
}

void testLoadGameSlotSummaryFitsItsPanel() {
    section("defect 6 / P1 — the LOAD GAME slot summary stays inside its frame");

    // The page's real geometry, from MenuState::render()'s Page::Load branch.
    const float centerX     = Constants::WINDOW_WIDTH * 0.5f;
    const float listLeft    = centerX - 230.0f;
    const float panelRightX = centerX + 300.0f;
    constexpr unsigned int charSize = 12;

    // The shortest summary a real save can produce, and the longest.
    const std::string shortest = slotSummary("MARIO", 1, 0, 0, "0:00");
    const std::string longest  = slotSummary("LUIGI", 8, 9999999, 3, "99:59");
    check(shortest.size() == 31, "the shortest real summary is 31 characters");
    check(longest.size() == 38, "the worst real summary is 38 characters");

    // What the defect was: the old hardcoded value column at centerX + 10 left
    // 290px, and even the shortest string needed 371.
    constexpr float oldValueBox = 290.0f;
    check(UiRenderer::measureTextWidth(shortest, charSize) > oldValueBox,
          "the OLD 290px value column could not hold even the shortest summary");
    check(UiRenderer::measureTextWidth(longest, charSize) - oldValueBox > 150.0f,
          "and the worst case overran it by more than 150px — through the border");

    // What the fix does: no explicit value column is passed any more, so
    // drawMenuItems derives it from the widest label on the page and gives the
    // summary every remaining pixel up to the panel edge. Mirrors the renderer's
    // own arithmetic — gutter is one character cell.
    const float gutter = static_cast<float>(charSize);
    float widestLabel = 0.0f;
    for (const char* label : {"SLOT 1", "SLOT 2", "SLOT 3", "BACK"}) {
        widestLabel = std::max(widestLabel, UiRenderer::measureTextWidth(label, charSize));
    }
    const float valueX   = listLeft + widestLabel + gutter * 2.0f;
    const float valueBox = panelRightX - gutter - valueX;

    check(valueBox > oldValueBox + 100.0f,
          "the derived column hands the summary 100px more than the old one did");

    const unsigned int fitted = UiRenderer::fitCharSize(longest, charSize, valueBox);
    check(UiRenderer::measureTextWidth(longest, fitted) <= valueBox,
          "the worst real summary now measures inside its value column");
    check(valueX + UiRenderer::measureTextWidth(longest, fitted) <= panelRightX,
          "and its right edge lands inside the panel border, not past it");
    check(fitted >= 10,
          "at a still-readable size — it is fitted, not shrunk to the floor");
    check(fitted <= charSize, "and never larger than the page asked for");
}

void testFittedTextNeverLeavesItsBox(sf::RenderTexture* target) {
    section("defect 6 — drawTextFitted paints no pixel outside the box");

    if (!target) {
        std::cout << "  [note] no graphics context — pixel pass skipped\n";
        return;
    }

    // A string that cannot fit by shrinking alone, so this also exercises the
    // ellipsis path: 60 characters in 100px is 1.6px per character.
    const std::string overlong(60, 'W');
    constexpr float boxLeft  = 40.0f;
    constexpr float boxWidth = 100.0f;

    const auto rightmostInkedColumn = [&target]() {
        const sf::Image image = target->getTexture().copyToImage();
        int rightmost = -1;
        for (unsigned int y = 0; y < image.getSize().y; ++y) {
            for (unsigned int x = 0; x < image.getSize().x; ++x) {
                const sf::Color c = image.getPixel({x, y});
                if (c.r > 40 || c.g > 40 || c.b > 40) rightmost = static_cast<int>(x);
            }
        }
        return rightmost;
    };

    // Control: the unfitted call is the defect, and it must still be visible —
    // otherwise the fitted pass below could pass for the wrong reason.
    target->clear(sf::Color::Black);
    UiRenderer::drawText(*target, overlong, {boxLeft, 20.0f}, 24, sf::Color::White);
    target->display();
    const int unfittedRight = rightmostInkedColumn();
    check(unfittedRight > static_cast<int>(boxLeft + boxWidth),
          "drawText with no budget still paints past the box — the defect is observable");

    target->clear(sf::Color::Black);
    UiRenderer::drawTextFitted(*target, overlong, {boxLeft, 20.0f}, 24, sf::Color::White,
                              boxWidth);
    target->display();
    const int fittedRight = rightmostInkedColumn();
    check(fittedRight >= 0, "drawTextFitted still draws something rather than giving up");
    check(fittedRight <= static_cast<int>(boxLeft + boxWidth),
          "and every inked pixel of it is inside the box");

    // Centred text is measured about pos.x, so both edges have to be checked.
    target->clear(sf::Color::Black);
    UiRenderer::drawTextFitted(*target, overlong, {640.0f, 20.0f}, 24, sf::Color::White,
                              200.0f, true);
    target->display();
    const sf::Image centred = target->getTexture().copyToImage();
    bool insideBand = true;
    for (unsigned int y = 0; y < centred.getSize().y; ++y) {
        for (unsigned int x = 0; x < centred.getSize().x; ++x) {
            const sf::Color c = centred.getPixel({x, y});
            if ((c.r > 40 || c.g > 40 || c.b > 40) && (x < 540u || x > 740u)) insideBand = false;
        }
    }
    check(insideBand, "a centred fitted line stays within maxWidth on both sides");
}

void testWrapText() {
    section("defect 6 — wrapText keeps every line inside the budget");

    constexpr unsigned int size = 11;
    constexpr float budget = 200.0f;

    const std::vector<std::string> lines =
        UiRenderer::wrapText("BOTH PLAYERS SHARE KEYS - SEE OPTIONS SLASH KEYS", size, budget);
    bool allFit = true;
    std::string rejoined;
    for (const std::string& line : lines) {
        if (UiRenderer::measureTextWidth(line, size) > budget) allFit = false;
        if (!rejoined.empty()) rejoined += " ";
        rejoined += line;
    }
    check(lines.size() > 1, "a line longer than the budget is split into several");
    check(allFit, "and every resulting line measures inside it");
    check(rejoined == "BOTH PLAYERS SHARE KEYS - SEE OPTIONS SLASH KEYS",
          "with no word lost or duplicated");

    // A single unbreakable token is the case that would otherwise overflow
    // silently, because there is no space to break at.
    const std::vector<std::string> forced = UiRenderer::wrapText(std::string(80, 'X'), size, budget);
    bool forcedFit = true;
    for (const std::string& line : forced) {
        if (UiRenderer::measureTextWidth(line, size) > budget) forcedFit = false;
    }
    check(forced.size() > 1 && forcedFit,
          "a word wider than the box is broken mid-word rather than allowed to overflow");
}

void testDebugModeDefaultsOffAndPersists() {
    section("defect 12 — debugMode defaults to OFF and round-trips through config.json");

    float sfx = 0.0f, music = 0.0f;
    std::string difficulty;
    std::unordered_map<std::string, std::string> bindings, bindings2;
    bool colorblind = true;
    bool debug = true;   // deliberately seeded true: the loader must overwrite it

    // 1. No config.json at all — the state a fresh install is in.
    check(Serializer::loadSettings(sfx, music, difficulty, bindings, bindings2,
                                   colorblind, debug),
          "loadSettings succeeds with no config.json present");
    check(!debug, "and debug mode is OFF by default");

    // 2. Round trip.
    check(Serializer::saveSettings(70.0f, 40.0f, "hard", bindings, bindings2, false, true),
          "saveSettings writes a config.json with debugMode on");
    debug = false;
    colorblind = true;
    check(Serializer::loadSettings(sfx, music, difficulty, bindings, bindings2,
                                   colorblind, debug),
          "and it reads back");
    check(debug, "debug mode survived the round trip");
    check(!colorblind,
          "and the colourblind flag came back independently — the two toggles do not "
          "drive each other");

    // 3. The forward-compatibility requirement: a config.json written by an
    // older build has no debugMode key at all, and that player must not be
    // handed the developer overlays. Same else-false read as colorblindMode.
    {
        std::ofstream legacy(Serializer::saveDirectory() + "/config.json");
        legacy << R"({"sfxVolume": 80.0, "musicVolume": 60.0, "difficulty": "normal",)"
               << R"( "keyBindings": {"jump": "W"}, "colorblindMode": true})";
    }
    debug = true;
    colorblind = false;
    check(Serializer::loadSettings(sfx, music, difficulty, bindings, bindings2,
                                   colorblind, debug),
          "a config.json predating the setting still parses rather than throwing away "
          "every other setting");
    check(!debug, "and debug mode reads as OFF when the key is absent");
    check(colorblind, "while the keys it does have are still honoured");
}

void testTheOptionsToggleRowDrivesTheRightFlag(sf::RenderTexture* target) {
    section("defect 12 — the OPTIONS row toggles debug mode, not colourblind");

    Game& game = Game::getInstance();
    game.setColorblindMode(false);
    game.setDebugMode(false);

    OptionsState options;
    options.enter();
    if (target) options.render(*target);

    // Settings rows: 0 music, 1 sfx, 2 difficulty, 3 colourblind, 4 debug, 5 back.
    // Selection starts on row 0, so three Downs land on colourblind.
    for (int i = 0; i < 3; ++i) step(options, sf::Keyboard::Key::Down);
    step(options, sf::Keyboard::Key::Enter);
    check(game.getColorblindMode(), "row 3 is still the colourblind toggle");
    check(!game.getDebugMode(), "and it did not touch debug mode");

    step(options, sf::Keyboard::Key::Down);
    step(options, sf::Keyboard::Key::Enter);
    check(game.getDebugMode(), "row 4 turns debug mode on");
    check(game.getColorblindMode(), "without disturbing colourblind mode");

    // Left/Right must reach it too — a Toggle row is adjusted as well as
    // activated, and adjustSelected() had the same hardcoded flag Enter did.
    step(options, sf::Keyboard::Key::Right);
    check(!game.getDebugMode(), "Right on the same row flips it back off");

    if (target) options.render(*target);
    options.exit();

    // Leave the process as it found it: this flag now decides whether the ImGui
    // dev windows exist, and a harness has no business turning them on.
    game.setDebugMode(false);
    game.setColorblindMode(false);
}

} // namespace

int main() {
    // Every save path in this process points at a throwaway directory, so
    // nothing here can read or delete real save data (g-rule-13). See
    // TestSaveSandbox.hpp for the ctest run that deleted the real one.
    TestSaveSandbox sandbox("r21_ui_layout");

    std::cout << "R21 UI layout / debug-mode harness\n";

    // Every measurement below is a claim about THIS font. Without it
    // ResourceManager hands back an empty placeholder, measureTextWidth returns
    // 0 for everything, and the whole harness would pass vacuously — which is
    // worse than no harness, so refuse to run instead.
    if (!ResourceManager::getInstance().loadFont("PressStart2P",
                                                 "assets/font/PressStart2P.ttf")) {
        std::cout << "  [FAIL] could not load assets/font/PressStart2P.ttf — every width "
                     "check below would be vacuous\n";
        return 1;
    }

    // Deliberately never destroyed: destroying it tears down the shared GL
    // context while SFML's own globals are still to be torn down after main
    // returns, which aborts the process even when every check passed. The same
    // trade verify_frontend_states.cpp documents.
    sf::RenderTexture* target = nullptr;
    try {
        target = new sf::RenderTexture(sf::Vector2u{1280u, 720u});
    } catch (...) {
        target = nullptr;
        std::cout << "  [note] no graphics context — render passes skipped\n";
    }

    testTheFontMetricFitCharSizeAssumes();
    testFitCharSizeArithmetic();
    testLoadGameSlotSummaryFitsItsPanel();
    testFittedTextNeverLeavesItsBox(target);
    testWrapText();
    testDebugModeDefaultsOffAndPersists();
    testTheOptionsToggleRowDrivesTheRightFlag(target);

    // Both singletons hold SFML resources that must be released while the GL
    // context is still alive.
    SoundManager::getInstance().shutdown();
    ResourceManager::getInstance().clear();

    std::cout << "\n----------------------------------------\n";
    std::cout << g_checks - g_failures << " / " << g_checks << " checks passed\n";
    if (g_failures > 0) {
        std::cout << g_failures << " FAILURE(S)\n";
        return 1;
    }
    std::cout << "ALL PASS\n";
    return 0;
}
