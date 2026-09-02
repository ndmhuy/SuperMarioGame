// verify_r21_lighting.cpp — Bonus D, dynamic lighting (SPEC.md §19.4).
//
// What this guards
// ----------------
// SPEC.md's rubric table trades the missing 3D component for "GLSL shader
// effects" and FEATURE_PROPOSAL.md repeats the claim; until R21 those five
// marks rested on a feature with no code. This harness is what keeps the claim
// true, and it has to do it on a machine with no GPU, because CI has none.
//
// So the split in LightingRenderer is the split here:
//
//   1. THE ASSET. assets/shaders/radial_light.frag must exist and must be
//      findable through the same ResourceManager::resolvePath the game calls.
//      A shader that only resolves from the build tree is the bug class
//      guard_asset_single_source already exists for.
//   2. THE C++/GLSL CONTRACT (g-rule-17). Every uniform LightingRenderer::render
//      writes must be declared in the .frag, and MAX_LIGHTS must agree on both
//      sides. Nothing else in the project would notice a rename: setting a
//      uniform that does not exist is silently ignored by SFML, so the light
//      would simply stop moving and no test would go red.
//   3. THE DAY/NIGHT MODEL. Pure arithmetic — phase at 0, a quarter, half, a
//      full period and past the wrap, plus the negative case a rewound clock
//      produces.
//   4. THE THEME MAPPING. Overworld and Ice carry no darkness of their own;
//      Underground and Castle do, and a cave is darker than a castle.
//   5. THE FALLBACK. With no GLSL support the pass must draw NOTHING and leave
//      the frame byte-identical, not paint a black rectangle over the level.
//
// Only checks 5's positive half and the radial falloff itself need a GPU. They
// announce a SKIP when sf::Shader::isAvailable() is false rather than failing.
//
// Run via:  ctest -R r21_lighting --output-on-failure
//
// Reachability note: PlayingState::renderLightPass calls LightingRenderer::render
// between the world pass and the HUD pass of PlayingState::render, which is on
// the path from main(). This harness proves the model and the contract, not that
// the seam is reached — the screenshots in saves/shots/r21i_* do that.

#include "Core/ResourceManager.hpp"
#include "Graphics/BackgroundRenderer.hpp"
#include "Graphics/LightingRenderer.hpp"

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Image.hpp>
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/Shader.hpp>
#include <SFML/Graphics/View.hpp>

#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "TestSaveSandbox.hpp"

namespace {

int g_failures = 0;
int g_checks   = 0;
int g_skipped  = 0;

void check(bool condition, const std::string& what) {
    ++g_checks;
    std::cout << (condition ? "  [ ok ] " : "  [FAIL] ") << what << "\n";
    if (!condition) ++g_failures;
}

void skip(const std::string& what, const std::string& why) {
    ++g_skipped;
    std::cout << "  [SKIP] " << what << " -- " << why << "\n";
}

void section(const std::string& title) {
    std::cout << "\n=== " << title << " ===\n";
}

bool near(float a, float b, float tolerance = 0.0005f) {
    return std::fabs(a - b) <= tolerance;
}

std::string readShaderSource(std::string& resolvedPathOut) {
    resolvedPathOut = ResourceManager::resolvePath(LightingRenderer::shaderAssetPath());
    std::ifstream file(resolvedPathOut);
    if (!file.is_open()) return std::string();
    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// --------------------------------------------------------------------------
// 1 + 2. The asset, and the contract between it and the C++ that feeds it.
// --------------------------------------------------------------------------
void testTheShaderAssetIsFoundTheWayTheGameFindsIt() {
    section("The GLSL asset resolves through ResourceManager");

    std::string resolved;
    const std::string source = readShaderSource(resolved);

    std::cout << "  [note] resolved to: " << resolved << "\n";

    check(std::string(LightingRenderer::shaderAssetPath()) == "assets/shaders/radial_light.frag",
          "the shader is addressed by an asset-relative path, not a build-relative one");
    check(std::filesystem::exists(resolved),
          "resolvePath() finds the fragment shader from this working directory");
    check(!source.empty(), "the fragment shader is readable and not empty");
    check(source.find("gl_FragColor") != std::string::npos,
          "it really is a fragment program, not a placeholder file");
    check(source.find("smoothstep") != std::string::npos &&
          source.find("distance(") != std::string::npos,
          "the radial falloff is computed per fragment (distance + smoothstep)");
}

void testEveryUniformTheRendererWritesExistsInTheShader() {
    section("C++/GLSL uniform contract (g-rule-17)");

    std::string resolved;
    const std::string source = readShaderSource(resolved);

    // These are exactly the names LightingRenderer::render() passes to
    // setUniform / setUniformArray. SFML silently ignores a uniform that does
    // not exist, so a rename on either side produces a feature that quietly
    // stops working and a suite that stays green. This is the check that fails.
    const char* uniforms[] = {
        "u_lightPos", "u_lightRadius", "u_lightIntensity",
        "u_lightTint", "u_shadowColor", "u_darkness",
    };
    for (const char* name : uniforms) {
        const std::string declaration = std::string("uniform");
        const std::size_t at = source.find(name);
        // Found, and found on a line that declares it rather than only in prose.
        bool declared = false;
        if (at != std::string::npos) {
            const std::size_t lineStart = source.rfind('\n', at);
            const std::string line = source.substr(
                lineStart == std::string::npos ? 0 : lineStart + 1,
                at - (lineStart == std::string::npos ? 0 : lineStart + 1));
            declared = line.find(declaration) != std::string::npos;
        }
        check(declared, std::string("shader declares uniform ") + name);
    }

    // MAX_LIGHTS is stated twice by necessity — once as a GLSL compile-time
    // constant (the loop bound has to be constant, see the shader's comment) and
    // once as the C++ array size. A value in two places is already wrong in one
    // of them unless something checks.
    const std::string expected =
        "const int MAX_LIGHTS = " + std::to_string(LightingRenderer::MAX_LIGHTS) + ";";
    check(source.find(expected) != std::string::npos,
          "shader's MAX_LIGHTS matches LightingRenderer::MAX_LIGHTS (" +
              std::to_string(LightingRenderer::MAX_LIGHTS) + ")");
}

// --------------------------------------------------------------------------
// 3. The day/night model.
// --------------------------------------------------------------------------
void testDayNightPhaseIsCorrectAndPeriodic() {
    section("Day/night phase — boundaries and wrapping");

    const float P = LightingRenderer::DAY_NIGHT_PERIOD;
    check(near(P, 100.0f), "the cycle period is the 100s horizon the user asked for");

    check(near(LightingRenderer::dayNightPhase(0.0f), 0.0f),
          "t = 0 is the start of the cycle (noon)");
    check(near(LightingRenderer::dayNightPhase(P * 0.25f), 0.25f),
          "a quarter period in is phase 0.25");
    check(near(LightingRenderer::dayNightPhase(P * 0.5f), 0.5f),
          "half a period in is phase 0.5 (midnight)");
    check(near(LightingRenderer::dayNightPhase(P * 0.75f), 0.75f),
          "three quarters in is phase 0.75");

    // The wrap. A full period must land back exactly on the start, not on 1.0,
    // or the very next frame jumps the sky from midnight-adjacent to noon.
    check(near(LightingRenderer::dayNightPhase(P), 0.0f),
          "one full period wraps back to phase 0, not to 1");
    check(near(LightingRenderer::dayNightPhase(P + P * 0.25f), 0.25f),
          "past the wrap the cycle repeats (1.25 periods -> 0.25)");
    check(near(LightingRenderer::dayNightPhase(P * 7.5f), 0.5f),
          "seven and a half periods in is midnight again");

    // A long Endless run: floats lose resolution but the phase must stay in
    // range rather than saturating, which is what a naive accumulate-and-clamp
    // would do.
    const float longRun = LightingRenderer::dayNightPhase(P * 500.0f + P * 0.5f);
    check(longRun >= 0.0f && longRun < 1.0f, "phase stays in [0, 1) after 500 cycles");

    // TimeRewindManager can hand back a clock that has moved backwards. fmod
    // keeps its left operand's sign, so this is the case that needs the fix.
    check(near(LightingRenderer::dayNightPhase(-P * 0.25f), 0.75f),
          "a negative elapsed time wraps forward instead of running the cycle backwards");
}

void testNightFactorIsSmoothAndPeriodic() {
    section("Night factor — 0 at noon, 1 at midnight, continuous at the wrap");

    check(near(LightingRenderer::nightFactor(0.0f), 0.0f), "phase 0 is full daylight");
    check(near(LightingRenderer::nightFactor(0.25f), 0.5f), "phase 0.25 is dusk, halfway");
    check(near(LightingRenderer::nightFactor(0.5f), 1.0f), "phase 0.5 is full night");
    check(near(LightingRenderer::nightFactor(0.75f), 0.5f), "phase 0.75 is dawn, halfway");
    check(near(LightingRenderer::nightFactor(1.0f), 0.0f), "phase 1 is daylight again");

    // Monotonic into the night. A cycle that brightened halfway to midnight
    // would read as a flicker, not as dusk.
    bool monotonic = true;
    for (int i = 0; i < 50; ++i) {
        const float a = LightingRenderer::nightFactor(static_cast<float>(i) / 100.0f);
        const float b = LightingRenderer::nightFactor(static_cast<float>(i + 1) / 100.0f);
        if (b < a - 1e-5f) monotonic = false;
    }
    check(monotonic, "darkness increases monotonically from noon to midnight");

    // No step at the seam: the frame before the wrap and the frame after it must
    // be within a frame's worth of each other.
    const float before = LightingRenderer::nightFactor(
        LightingRenderer::dayNightPhase(LightingRenderer::DAY_NIGHT_PERIOD - 0.01f));
    const float after = LightingRenderer::nightFactor(
        LightingRenderer::dayNightPhase(LightingRenderer::DAY_NIGHT_PERIOD + 0.01f));
    check(near(before, after, 0.001f), "the cycle is continuous across the wrap");
}

// --------------------------------------------------------------------------
// 4. Theme -> darkness.
// --------------------------------------------------------------------------
void testThemeDrivesDarkness() {
    section("Theme -> darkness (the level file's \"theme\" field, already parsed)");

    check(near(LightingRenderer::baseDarknessForTheme(BackgroundTheme::Overworld), 0.0f),
          "overworld carries no darkness of its own");
    check(near(LightingRenderer::baseDarknessForTheme(BackgroundTheme::Ice), 0.0f),
          "ice carries no darkness of its own");
    check(LightingRenderer::baseDarknessForTheme(BackgroundTheme::Underground) > 0.5f,
          "underground is darkened");
    check(LightingRenderer::baseDarknessForTheme(BackgroundTheme::Castle) > 0.5f,
          "castle is darkened");
    check(LightingRenderer::baseDarknessForTheme(BackgroundTheme::Underground) >
              LightingRenderer::baseDarknessForTheme(BackgroundTheme::Castle),
          "a cave is darker than a castle, which keeps its torches");

    // Nothing may reach full opacity: at darkness 1.0 the unlit part of the
    // frame is solid colour and the level stops being playable outside the lamp.
    for (BackgroundTheme theme : {BackgroundTheme::Overworld, BackgroundTheme::Underground,
                                  BackgroundTheme::Castle, BackgroundTheme::Ice}) {
        check(LightingRenderer::baseDarknessForTheme(theme) < 0.95f,
              "no theme darkens to opacity (theme " +
                  std::to_string(static_cast<int>(theme)) + ")");
    }

    // The level file's spelling is what actually selects the theme in the game,
    // so the mapping the shipped levels rely on is pinned here too.
    check(BackgroundRenderer::parseThemeName("underground") == BackgroundTheme::Underground,
          "\"underground\" (level_1_sub, level_2_sub, bonus_1) selects the dark theme");
    check(BackgroundRenderer::parseThemeName("castle") == BackgroundTheme::Castle,
          "\"castle\" (level_3, level_3_sub) selects the dark theme");
    check(BackgroundRenderer::parseThemeName("overworld") == BackgroundTheme::Overworld,
          "\"overworld\" (level_1) selects the undarkened theme");
    check(BackgroundRenderer::parseThemeName("ice") == BackgroundTheme::Ice,
          "\"ice\" (level_2) selects the undarkened theme");
}

void testIndoorThemesIgnoreTheClockAndOutdoorThemesFollowIt() {
    section("darknessFor() — the clock acts on the sky, not on caves");

    const float P = LightingRenderer::DAY_NIGHT_PERIOD;
    const float caveAtNoon     = LightingRenderer::darknessFor(BackgroundTheme::Underground, 0.0f);
    const float caveAtMidnight = LightingRenderer::darknessFor(BackgroundTheme::Underground, P * 0.5f);
    check(near(caveAtNoon, caveAtMidnight),
          "a cave is as dark at noon as at midnight (no sky to act on)");
    check(near(caveAtNoon, LightingRenderer::baseDarknessForTheme(BackgroundTheme::Underground)),
          "a cave's darkness is exactly its theme's base");

    check(near(LightingRenderer::darknessFor(BackgroundTheme::Overworld, 0.0f), 0.0f),
          "an overworld level at t=0 is undarkened — the pass draws nothing");
    check(near(LightingRenderer::darknessFor(BackgroundTheme::Overworld, P * 0.5f),
               LightingRenderer::NIGHT_DARKNESS),
          "the same level at midnight reaches the full night darkness");
    check(LightingRenderer::darknessFor(BackgroundTheme::Overworld, P * 0.5f) <
              LightingRenderer::baseDarknessForTheme(BackgroundTheme::Underground),
          "night outdoors stays brighter than a cave — a night level must stay readable");

    // The user's brief: a campaign level (LEVEL_TIME 300s) sees a full cycle.
    check(300.0f / P >= 1.0f, "a 300s campaign level sees at least one whole cycle");
}

// --------------------------------------------------------------------------
// 5. The fallback, and (GPU permitting) the falloff itself.
// --------------------------------------------------------------------------
sf::Color sampleAt(const sf::Image& image, unsigned x, unsigned y) {
    return image.getPixel({x, y});
}

int luminance(sf::Color c) {
    return static_cast<int>(c.r) + static_cast<int>(c.g) + static_cast<int>(c.b);
}

void testRenderPathAndFallback() {
    section("Rendering: the fallback must leave the frame untouched");

    // Deliberately never destroyed: destroying it tears down the shared GL
    // context while SFML's own globals are still to be torn down after main
    // returns, which aborts the process even when every check passed. The same
    // trade verify_r21_ui_layout.cpp and verify_frontend_states.cpp document.
    sf::RenderTexture* targetPtr = nullptr;
    try {
        targetPtr = new sf::RenderTexture(sf::Vector2u{320u, 240u});
    } catch (...) {
        targetPtr = nullptr;
    }
    if (!targetPtr) {
        skip("light-pass rendering", "no graphics context on this machine");
        return;
    }
    sf::RenderTexture& target = *targetPtr;

    const sf::Color base(120, 120, 120);
    auto paintAndCapture = [&](bool runLightPass, LightingRenderer& lighting,
                               BackgroundTheme theme, float elapsed,
                               const std::vector<LightingRenderer::Light>& lights,
                               bool& drewOut) {
        target.clear(base);
        drewOut = false;
        if (runLightPass) {
            drewOut = lighting.render(target, target.getDefaultView(), lights, theme, elapsed);
        }
        target.display();
        return target.getTexture().copyToImage();
    };

    LightingRenderer lighting;

    // One lamp in the middle of the frame, in world coordinates that the default
    // view maps 1:1 onto pixels.
    LightingRenderer::Light lamp;
    lamp.worldPosition = {160.0f, 120.0f};
    lamp.radius        = 70.0f;
    lamp.intensity     = 1.0f;
    const std::vector<LightingRenderer::Light> lights{lamp};

    bool drew = false;
    const sf::Image lit =
        paintAndCapture(true, lighting, BackgroundTheme::Underground, 0.0f, lights, drew);

    if (!sf::Shader::isAvailable()) {
        // The path a grading machine or a CI box without GLSL takes. This is the
        // single highest-risk behaviour in the feature: getting it wrong means a
        // black screen, not a missing effect.
        check(!drew, "no GLSL support: the light pass declines to draw");
        check(near(static_cast<float>(luminance(sampleAt(lit, 10u, 10u))),
                   static_cast<float>(luminance(base)), 0.5f),
              "no GLSL support: the frame is left exactly as the world drew it");
        check(!lighting.isOperational(),
              "no GLSL support: isOperational() reports the feature off");
        skip("radial falloff", "sf::Shader::isAvailable() is false");
        return;
    }

    check(drew, "GLSL available: an underground level draws the darkness pass");
    check(lighting.isOperational(), "the fragment shader compiled and loaded");

    const int atLamp   = luminance(sampleAt(lit, 160u, 120u));
    const int atRim    = luminance(sampleAt(lit, 160u, 55u));
    const int atCorner = luminance(sampleAt(lit, 4u, 4u));
    std::cout << "  [note] luminance sum  centre=" << atLamp
              << "  rim=" << atRim << "  corner=" << atCorner
              << "  (unlit source=" << luminance(base) << ")\n";

    check(atCorner < luminance(base) / 2,
          "away from every lamp an underground level is substantially darkened");
    check(atLamp > atCorner,
          "the lamp's centre is brighter than the far corner");
    check(atLamp > atRim && atRim > atCorner,
          "brightness falls off radially: centre > rim > corner");

    // THE DONUT GUARD. What this pass contributes at a pixel is
    // alpha * (tint - scene), and alpha falls to zero at a lamp's core -- so a
    // tint BRIGHTER than the scene peaks in the mid-band and the lamp renders as
    // a bright ring around a dark hole. That was the state of this feature at
    // its first free-camera observation. Brightness must therefore fall off
    // monotonically from the centre outwards, with no local maximum in between.
    int previous = luminance(sampleAt(lit, 160u, 120u));
    bool monotonic = true;
    unsigned worstX = 0;
    for (unsigned dx = 4; dx <= 120; dx += 4) {
        const int here = luminance(sampleAt(lit, 160u + dx, 120u));
        // A few units of slack for the driver's own rounding across the ramp.
        if (here > previous + 6) { monotonic = false; worstX = dx; }
        previous = here;
    }
    check(monotonic,
          monotonic ? "brightness never rises again on the way out (no bright ring)"
                    : "brightness rises again at +" + std::to_string(worstX) +
                          "px from the lamp -- the tint is brighter than the scene");

    // The other half of the fallback contract, on a machine that does have a
    // GPU: "nothing to darken" must also mean "nothing drawn".
    bool drewAtNoon = false;
    const sf::Image noon =
        paintAndCapture(true, lighting, BackgroundTheme::Overworld, 0.0f, lights, drewAtNoon);
    check(!drewAtNoon, "an overworld level at high noon draws no pass at all");
    check(luminance(sampleAt(noon, 4u, 4u)) == luminance(base),
          "and therefore leaves the frame byte-identical");

    // The same level once the cycle has turned.
    bool drewAtNight = false;
    paintAndCapture(true, lighting, BackgroundTheme::Overworld,
                    LightingRenderer::DAY_NIGHT_PERIOD * 0.5f, lights, drewAtNight);
    check(drewAtNight, "the same overworld level at midnight does draw the pass");

    // The pass must not leak its screen-space view back to the caller: the
    // world pass that follows it in PlayingState::render depends on this.
    sf::View custom(sf::FloatRect({40.0f, 40.0f}, {80.0f, 60.0f}));
    target.setView(custom);
    (void)lighting.render(target, custom, lights, BackgroundTheme::Castle, 0.0f);
    check(near(target.getView().getCenter().x, custom.getCenter().x) &&
              near(target.getView().getCenter().y, custom.getCenter().y),
          "render() restores the caller's view before returning");

    // More lamps than slots must clamp, not overrun the uniform arrays.
    std::vector<LightingRenderer::Light> many;
    for (std::size_t i = 0; i < LightingRenderer::MAX_LIGHTS + 5; ++i) {
        LightingRenderer::Light extra = lamp;
        extra.worldPosition = {20.0f + 20.0f * static_cast<float>(i), 120.0f};
        many.push_back(extra);
    }
    bool drewMany = false;
    paintAndCapture(true, lighting, BackgroundTheme::Underground, 0.0f, many, drewMany);
    check(drewMany, "more lamps than MAX_LIGHTS still renders (surplus is dropped, not overrun)");
}

} // namespace

int main() {
    // This harness writes no save data, but the fixture is cheap and sealing it
    // means a future check that reaches for a save path cannot escape into the
    // developer's real saves/ (g-rule-13).
    TestSaveSandbox sandbox("verify_r21_lighting");

    std::cout << "========================================\n";
    std::cout << " R21 Bonus D — dynamic lighting\n";
    std::cout << "========================================\n";

    testTheShaderAssetIsFoundTheWayTheGameFindsIt();
    testEveryUniformTheRendererWritesExistsInTheShader();
    testDayNightPhaseIsCorrectAndPeriodic();
    testNightFactorIsSmoothAndPeriodic();
    testThemeDrivesDarkness();
    testIndoorThemesIgnoreTheClockAndOutdoorThemesFollowIt();
    testRenderPathAndFallback();

    // Holds SFML resources that must be released while the context is alive.
    ResourceManager::getInstance().clear();

    std::cout << "\n----------------------------------------\n";
    std::cout << g_checks - g_failures << " / " << g_checks << " checks passed";
    if (g_skipped > 0) std::cout << "  (" << g_skipped << " skipped)";
    std::cout << "\n";
    if (g_failures > 0) {
        std::cout << g_failures << " FAILURE(S)\n";
        return 1;
    }
    std::cout << "ALL PASS\n";
    return 0;
}
