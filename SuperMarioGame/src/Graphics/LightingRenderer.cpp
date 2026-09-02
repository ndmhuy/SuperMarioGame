#include "Graphics/LightingRenderer.hpp"

#include "Core/ResourceManager.hpp"

#include <SFML/Graphics/Glsl.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderStates.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iostream>

namespace {

// Two thirds of the frame lost to shadow, so the shape of the cave still reads
// at the edges of the lamp instead of going to pure black. Tuned by eye against
// bonus_1 and level_3 rather than derived.
constexpr float kUndergroundDarkness = 0.84f;
// Castles keep their torches, so they sit a step brighter than a cave.
constexpr float kCastleDarkness = 0.72f;

constexpr float kTwoPi = 6.28318530718f;

sf::Glsl::Vec3 toGlsl(sf::Color color) {
    return sf::Glsl::Vec3(static_cast<float>(color.r) / 255.0f,
                          static_cast<float>(color.g) / 255.0f,
                          static_cast<float>(color.b) / 255.0f);
}

} // namespace

const char* LightingRenderer::shaderAssetPath() {
    return "assets/shaders/radial_light.frag";
}

float LightingRenderer::baseDarknessForTheme(BackgroundTheme theme) {
    switch (theme) {
        case BackgroundTheme::Underground: return kUndergroundDarkness;
        case BackgroundTheme::Castle:      return kCastleDarkness;
        case BackgroundTheme::Ice:
        case BackgroundTheme::Overworld:
        default:
            // Outdoors. Not "no lighting" -- the day/night cycle supplies the
            // darkness for these, which is why this is a *base* and not the
            // final answer.
            return 0.0f;
    }
}

sf::Color LightingRenderer::shadowColorForTheme(BackgroundTheme theme) {
    switch (theme) {
        // Cold and nearly black: a cave has no light source of its own.
        case BackgroundTheme::Underground: return sf::Color(6, 6, 16);
        // Warmer, because what little the player can see in a castle is
        // firelight rather than daylight.
        case BackgroundTheme::Castle:      return sf::Color(16, 7, 9);
        case BackgroundTheme::Ice:         return sf::Color(10, 18, 34);
        case BackgroundTheme::Overworld:
        default:
            // Night sky blue rather than grey. A neutral shadow over a coloured
            // backdrop reads as the screen dimming, not as dusk falling.
            return sf::Color(8, 12, 40);
    }
}

float LightingRenderer::dayNightPhase(float elapsedSeconds) {
    float phase = std::fmod(elapsedSeconds, DAY_NIGHT_PERIOD) / DAY_NIGHT_PERIOD;
    // std::fmod keeps the sign of its left operand. A negative phase would run
    // the cosine below backwards through the cycle instead of wrapping it, so a
    // rewound clock (TimeRewindManager) would brighten into the previous night.
    if (phase < 0.0f) phase += 1.0f;
    return phase;
}

float LightingRenderer::nightFactor(float phase) {
    // A raised cosine rather than a triangle wave: it is continuous in its
    // derivative at both noon and midnight, so the cycle has no visible kink at
    // the moment it turns around, and it is exactly periodic across the wrap
    // with no special case at phase 1.
    return 0.5f - 0.5f * std::cos(kTwoPi * phase);
}

float LightingRenderer::darknessFor(BackgroundTheme theme, float elapsedSeconds) {
    const float base = baseDarknessForTheme(theme);
    // Indoors there is no sky for the cycle to act on: a cave is as dark at noon
    // as at midnight. Keeping the two independent is also what makes them
    // separately testable -- the theme mapping and the clock never have to be
    // untangled from a single blended number.
    if (base > 0.0f) return base;
    return NIGHT_DARKNESS * nightFactor(dayNightPhase(elapsedSeconds));
}

bool LightingRenderer::ensureShaderLoaded() {
    if (m_state == State::Ready)       return true;
    if (m_state == State::Unavailable) return false;

    // Pessimistic by default: every failure path below simply returns, and the
    // feature is off rather than half on.
    m_state = State::Unavailable;

    if (!sf::Shader::isAvailable()) {
        std::cerr << "[LightingRenderer] This machine reports no GLSL shader "
                     "support; dynamic lighting is off and the game renders "
                     "normally." << std::endl;
        return false;
    }

    // resolvePath is the one place that knows where assets are relative to the
    // working directory. A literal "../assets/..." here would work from the
    // build tree and fail from anywhere else -- the exact bug class
    // guard_asset_single_source and the ParticleSystem comment (A-13) record.
    const std::string path = ResourceManager::resolvePath(shaderAssetPath());
    if (!std::filesystem::exists(path)) {
        std::cerr << "[LightingRenderer] Shader not found: " << path
                  << " -- dynamic lighting is off and the game renders normally."
                  << std::endl;
        return false;
    }

    auto shader = std::make_unique<sf::Shader>();
    if (!shader->loadFromFile(path, sf::Shader::Type::Fragment)) {
        // SFML has already printed the driver's compile log by this point; this
        // line says what the consequence is, which the log does not.
        std::cerr << "[LightingRenderer] " << path
                  << " failed to compile; dynamic lighting is off and the game "
                     "renders normally." << std::endl;
        return false;
    }

    m_shader = std::move(shader);
    m_state  = State::Ready;
    return true;
}

bool LightingRenderer::isOperational() {
    return ensureShaderLoaded();
}

bool LightingRenderer::render(sf::RenderTarget& target,
                              const sf::View& worldView,
                              const std::vector<Light>& lights,
                              BackgroundTheme theme,
                              float elapsedSeconds) {
    const float darkness = darknessFor(theme, elapsedSeconds);
    // High noon on an overworld level. Returning before the shader is even
    // consulted keeps daylight levels at exactly the cost they had before this
    // feature existed, which is the honest way to add a full-screen pass.
    if (darkness <= 0.002f) return false;

    if (!ensureShaderLoaded()) return false;

    const sf::Vector2u size = target.getSize();
    if (size.x == 0 || size.y == 0) return false;

    std::array<sf::Glsl::Vec2, MAX_LIGHTS> positions{};
    std::array<float, MAX_LIGHTS>          radii{};
    std::array<float, MAX_LIGHTS>          intensities{};
    std::array<sf::Glsl::Vec3, MAX_LIGHTS> tints{};

    // World px -> screen px. Radii are authored in world units so a lamp is a
    // fixed number of TILES across, but the shader works in framebuffer pixels,
    // and the two are only equal while the view is unzoomed. Debug > Cheats'
    // FREE CAMERA zooms out, and before this scaling a lamp kept its pixel size
    // there and so covered several times its own world footprint.
    //
    // Measured through mapCoordsToPixel rather than computed from view and
    // viewport sizes, so it cannot disagree with the position mapping below --
    // whatever that does about viewports, this follows.
    const sf::Vector2i originPx = target.mapCoordsToPixel({0.0f, 0.0f}, worldView);
    const sf::Vector2i spanPx   = target.mapCoordsToPixel({1000.0f, 0.0f}, worldView);
    const float pixelsPerWorldUnit =
        std::max(0.01f, static_cast<float>(std::abs(spanPx.x - originPx.x)) / 1000.0f);

    std::size_t used = 0;
    for (const Light& light : lights) {
        if (used >= MAX_LIGHTS) break;
        if (light.radius < 1.0f) continue;

        const float pixelRadius = light.radius * pixelsPerWorldUnit;
        if (pixelRadius < 1.0f) continue;

        const sf::Vector2i pixel = target.mapCoordsToPixel(light.worldPosition, worldView);
        // A lamp whose halo cannot reach the frame is a wasted slot, and there
        // are only eight of them. Culling here rather than in the shader keeps
        // the fragment program branch-free.
        const int margin = static_cast<int>(pixelRadius) + 1;
        if (pixel.x < -margin || pixel.y < -margin ||
            pixel.x > static_cast<int>(size.x) + margin ||
            pixel.y > static_cast<int>(size.y) + margin) {
            continue;
        }

        // gl_FragCoord's origin is the bottom-left of the framebuffer;
        // mapCoordsToPixel's is the top-left. The flip belongs here, where the
        // target's height is known, and not in the shader, which is handed no
        // resolution at all.
        positions[used] = sf::Glsl::Vec2(
            static_cast<float>(pixel.x),
            static_cast<float>(size.y) - static_cast<float>(pixel.y));
        radii[used]       = pixelRadius;
        intensities[used] = std::clamp(light.intensity, 0.0f, 1.0f);
        tints[used]       = toGlsl(light.shadowTint);
        ++used;
    }

    // Slots past `used` keep their zero radius, which is how the shader knows
    // they are unused -- see radial_light.frag's loop.
    m_shader->setUniformArray("u_lightPos",       positions.data(),   MAX_LIGHTS);
    m_shader->setUniformArray("u_lightRadius",    radii.data(),       MAX_LIGHTS);
    m_shader->setUniformArray("u_lightIntensity", intensities.data(), MAX_LIGHTS);
    m_shader->setUniformArray("u_lightTint",      tints.data(),       MAX_LIGHTS);
    m_shader->setUniform("u_shadowColor", toGlsl(shadowColorForTheme(theme)));
    m_shader->setUniform("u_darkness", darkness);

    // Screen space: the quad has to cover the frame regardless of where the
    // camera is looking. The caller's view is restored before returning, because
    // PlayingState::render continues drawing in world space afterwards.
    const sf::View previous = target.getView();
    target.setView(target.getDefaultView());

    sf::RectangleShape quad(sf::Vector2f(static_cast<float>(size.x),
                                         static_cast<float>(size.y)));
    sf::RenderStates states;
    states.shader = m_shader.get();
    target.draw(quad, states);

    target.setView(previous);
    return true;
}
