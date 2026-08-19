#include "Graphics/UiRenderer.hpp"
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

void UiRenderer::drawMenuItems(sf::RenderTarget& target, const std::vector<UiMenuItem>& items,
                               int selectedIndex, sf::Vector2f topLeft, float rowHeight,
                               unsigned int charSize, float valueColumnX, float blinkPhase) {
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

        drawText(target, item.label, {topLeft.x, y}, charSize, color);

        if (!item.value.empty()) {
            const float vx = (valueColumnX > 0.0f) ? valueColumnX : topLeft.x + 320.0f;
            drawText(target, item.value, {vx, y}, charSize, color);
        }
    }
}

float UiRenderer::measureTextWidth(const std::string& text, unsigned int size) {
    sf::Text probe(font());
    probe.setString(text);
    probe.setCharacterSize(size);
    return probe.getLocalBounds().size.x;
}
