#include "Graphics/UiRenderer.hpp"
#include <cstdint>
#include <algorithm>
#include "Core/AchievementManager.hpp"
#include "Core/ResourceManager.hpp"
#include "Utils/Constants.hpp"

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Text.hpp>
#include <cmath>

namespace {

// Colours are named once here so the six menu states cannot drift apart.
constexpr std::uint8_t SELECTED_R = 255, SELECTED_G = 216, SELECTED_B = 0;

sf::Color selectedColor()  { return sf::Color(SELECTED_R, SELECTED_G, SELECTED_B); }
sf::Color normalColor()    { return sf::Color(235, 235, 235); }
sf::Color disabledColor()  { return sf::Color(110, 110, 110); }

// What a truncated string ends with. ASCII rather than U+2026, because the UI
// font has no glyph for the single-character ellipsis and would draw the
// missing-glyph box instead.
constexpr const char* ELLIPSIS = "...";

} // namespace

const sf::Font& UiRenderer::font() {
    return ResourceManager::getInstance().getFont("PressStart2P");
}

void UiRenderer::drawDimmer(sf::RenderTarget& target, std::uint8_t alpha, sf::Color tint) {
    sf::RectangleShape dim(sf::Vector2f(static_cast<float>(Constants::WINDOW_WIDTH),
                                        static_cast<float>(Constants::WINDOW_HEIGHT)));
    dim.setFillColor(sf::Color(tint.r, tint.g, tint.b, alpha));
    target.draw(dim);
}

void UiRenderer::drawPanel(sf::RenderTarget& target, sf::Vector2f topLeft, sf::Vector2f size,
                           sf::Color fill, sf::Color outline) {
    sf::RectangleShape panel(size);
    panel.setPosition(topLeft);
    panel.setFillColor(fill);
    panel.setOutlineColor(outline);
    panel.setOutlineThickness(3.0f);
    target.draw(panel);
}

void UiRenderer::drawText(sf::RenderTarget& target, const std::string& text, sf::Vector2f pos,
                          unsigned int size, sf::Color color, bool centerX) {
    sf::Text label(font());
    label.setString(text);
    label.setCharacterSize(size);
    label.setFillColor(color);
    if (centerX) {
        const float width = label.getLocalBounds().size.x;
        pos.x -= width * 0.5f;
    }
    // Round to whole pixels: the bitmap font smears badly on half-pixel offsets.
    label.setPosition({std::round(pos.x), std::round(pos.y)});
    target.draw(label);
}

void UiRenderer::drawShadowedText(sf::RenderTarget& target, const std::string& text, sf::Vector2f pos,
                                  unsigned int size, sf::Color color, bool centerX, sf::Color shadow) {
    drawText(target, text, {pos.x + 3.0f, pos.y + 3.0f}, size, shadow, centerX);
    drawText(target, text, pos, size, color, centerX);
}

unsigned int UiRenderer::fitCharSize(const std::string& text, unsigned int preferred,
                                     float maxWidth, unsigned int minimum) {
    if (text.empty() || maxWidth <= 0.0f) return preferred;
    if (minimum > preferred) minimum = preferred;

    // First estimate, from the font's design metric: PressStart2P is strictly
    // monospace at one em per advance (unitsPerEm 1000, every advance 1000), so
    // n characters at size S nominally occupy n*S px and the largest S that fits
    // width W is floor(W/n).
    const auto length = static_cast<float>(text.size());
    auto size = static_cast<unsigned int>(std::floor(maxWidth / length));
    size = std::clamp(size, minimum, preferred);

    // Then confirm it, because that estimate is NOT always right and errs in the
    // dangerous direction. FreeType hints the advance to whole pixels per size,
    // and the rounding is not always down: measured on this font, size 12 lays
    // out at 13px per character and size 20 at 21px, while 8, 11, 15 and 24 hit
    // their nominal width exactly. Size 12 is what the LOAD GAME page uses — so
    // trusting the closed form alone would have left that page still overflowing
    // by ~8%, which is the defect. Stepping down against the real metric also
    // means a future font swap degrades to a slightly small label rather than
    // silently reinstating the overflow.
    while (size > minimum && measureTextWidth(text, size) > maxWidth) {
        --size;
    }
    return size;
}

void UiRenderer::drawTextFitted(sf::RenderTarget& target, const std::string& text, sf::Vector2f pos,
                                unsigned int size, sf::Color color, float maxWidth,
                                bool centerX, unsigned int minSize) {
    if (text.empty()) return;
    if (maxWidth <= 0.0f) {
        drawText(target, text, pos, size, color, centerX);
        return;
    }

    const unsigned int fitted = fitCharSize(text, size, maxWidth, minSize);
    if (measureTextWidth(text, fitted) <= maxWidth) {
        drawText(target, text, pos, fitted, color, centerX);
        return;
    }

    // Only reachable once shrinking has bottomed out at minSize. Dropping
    // characters is the last resort because it destroys information; shrinking
    // does not.
    std::string clipped = text;
    while (!clipped.empty() && measureTextWidth(clipped + ELLIPSIS, fitted) > maxWidth) {
        clipped.pop_back();
    }
    drawText(target, clipped + ELLIPSIS, pos, fitted, color, centerX);
}

void UiRenderer::drawMenuItems(sf::RenderTarget& target, const std::vector<UiMenuItem>& items,
                               int selectedIndex, sf::Vector2f topLeft, float rowHeight,
                               unsigned int charSize, float valueColumnX, float blinkPhase,
                               float panelRightX) {
    // Clearance is one character cell, so the gutters scale with the type. A
    // pixel count would stop being right the moment a page picked a different
    // charSize — which the Options and Controls pages now derive at runtime.
    const float gutter = static_cast<float>(charSize);

    // Where the value column starts. Callers that know their layout pass it;
    // the rest get it derived from the widest label actually present. This
    // replaces a flat `topLeft.x + 320` fallback that belonged to no panel at
    // all and was one of the reasons no value column knew where its box ended.
    float valueX = valueColumnX;
    if (valueX <= 0.0f) {
        float widestLabel = 0.0f;
        bool anyValue = false;
        for (const UiMenuItem& item : items) {
            if (!item.value.empty()) anyValue = true;
            widestLabel = std::max(widestLabel, measureTextWidth(item.label, charSize));
        }
        if (anyValue) valueX = topLeft.x + widestLabel + gutter * 2.0f;
    }

    // Zero means unbounded in both cases, which is what a caller that passes no
    // panel edge and has no values gets — the pause and game-over lists.
    const float labelMax = (valueX > 0.0f)         ? valueX - gutter - topLeft.x
                         : (panelRightX > 0.0f)    ? panelRightX - gutter - topLeft.x
                                                   : 0.0f;
    const float valueMax = (panelRightX > 0.0f && valueX > 0.0f)
                         ? panelRightX - gutter - valueX
                         : 0.0f;

    for (std::size_t i = 0; i < items.size(); ++i) {
        const UiMenuItem& item = items[i];
        const bool isSelected = (static_cast<int>(i) == selectedIndex);

        sf::Color color = normalColor();
        if (!item.enabled)     color = disabledColor();
        else if (isSelected)   color = selectedColor();

        const float y = topLeft.y + rowHeight * static_cast<float>(i);

        if (isSelected) {
            // Blink at 4 Hz so the cursor reads as a cursor even in a screenshot.
            const bool caretVisible = std::fmod(blinkPhase, 0.5f) < 0.35f;
            if (caretVisible) {
                drawText(target, ">", {topLeft.x - 28.0f, y}, charSize, color);
            }
        }

        drawTextFitted(target, item.label, {topLeft.x, y}, charSize, color, labelMax);

        if (!item.value.empty()) {
            drawTextFitted(target, item.value, {valueX, y}, charSize, color, valueMax);
        }
    }
}

float UiRenderer::measureTextWidth(const std::string& text, unsigned int size) {
    sf::Text probe(font());
    probe.setString(text);
    probe.setCharacterSize(size);
    return probe.getLocalBounds().size.x;
}

void UiRenderer::drawAchievementToasts(sf::RenderTarget& target) {
    const auto& toasts = AchievementManager::getInstance().getActiveToasts();
    if (toasts.empty()) return;

    constexpr float CARD_W = 300.0f;
    constexpr float CARD_H = 56.0f;
    constexpr float MARGIN = 16.0f;

    float y = MARGIN;
    for (const AchievementToast& toast : toasts) {
        const auto fade = [&toast](sf::Color c) {
            c.a = static_cast<std::uint8_t>(
                std::clamp(toast.alpha, 0.0f, 1.0f) * static_cast<float>(c.a));
            return c;
        };

        const float x = static_cast<float>(Constants::WINDOW_WIDTH) - CARD_W - MARGIN;
        drawPanel(target, {x, y}, {CARD_W, CARD_H},
                  fade(sf::Color(0, 0, 0, 220)), fade(sf::Color(255, 216, 0, 240)));
        drawText(target, "ACHIEVEMENT UNLOCKED", {x + 14.0f, y + 12.0f}, 9,
                 fade(sf::Color(255, 216, 0)));
        drawText(target, toast.name, {x + 14.0f, y + 30.0f}, 12,
                 fade(sf::Color(255, 255, 255)));
        y += CARD_H + 8.0f;
    }
}
