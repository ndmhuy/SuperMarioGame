#pragma once

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/System/Vector2.hpp>
#include <cstdint>
#include <string>
#include <vector>

// One selectable row of a screen-space menu. `value` is the right-hand column
// used by settings rows ("Music   80%"); leave it empty for plain actions.
struct UiMenuItem {
    std::string label;
    std::string value;
    bool enabled = true;

    UiMenuItem() = default;
    UiMenuItem(std::string l, std::string v = "", bool e = true)
        : label(std::move(l)), value(std::move(v)), enabled(e) {}
};

// Screen-space drawing primitives shared by every menu-like state.
//
// These exist so the six front-end states draw the same panel, the same
// highlight and the same font without copying layout maths between them, and so
// none of them has to reach for ImGui — ImGui menus have to mutate game state
// from inside render(), which is the defect X-5 records.
//
// Every call assumes the target is already in its default (screen-space) view.
class UiRenderer {
public:
    // Full-screen translucent wash. Used by overlay states so the frame beneath
    // stays readable but visibly inactive.
    static void drawDimmer(sf::RenderTarget& target, std::uint8_t alpha,
                           sf::Color tint = sf::Color(0, 0, 0));

    // Bordered panel in the classic black/white NES palette.
    static void drawPanel(sf::RenderTarget& target, sf::Vector2f topLeft, sf::Vector2f size,
                          sf::Color fill = sf::Color(0, 0, 0, 220),
                          sf::Color outline = sf::Color(255, 255, 255, 230));

    // Single line of text. `centerX` centres horizontally on pos.x instead of
    // treating it as the left edge.
    static void drawText(sf::RenderTarget& target, const std::string& text, sf::Vector2f pos,
                         unsigned int size, sf::Color color, bool centerX = false);

    // Same, with a hard drop shadow — for headings that sit over the world.
    static void drawShadowedText(sf::RenderTarget& target, const std::string& text, sf::Vector2f pos,
                                 unsigned int size, sf::Color color, bool centerX = false,
                                 sf::Color shadow = sf::Color(0, 0, 0, 200));

    // Vertical list with the selected row highlighted by a caret and colour.
    // Disabled rows render greyed out. `blinkPhase` drives the caret blink and
    // is normally the state's own elapsed time.
    static void drawMenuItems(sf::RenderTarget& target, const std::vector<UiMenuItem>& items,
                              int selectedIndex, sf::Vector2f topLeft, float rowHeight,
                              unsigned int charSize, float valueColumnX = 0.0f,
                              float blinkPhase = 0.0f);

    // Achievement toasts, drawn in screen space in the top-right. Lives here
    // rather than in the HUD so any state can show them: they were an ImGui
    // window in the dev panel before, which meant they only ever appeared while
    // the dev overlay was up and never during ordinary play.
    static void drawAchievementToasts(sf::RenderTarget& target);

    // Width in pixels the given string would occupy, for manual layout.
    static float measureTextWidth(const std::string& text, unsigned int size);

    // Font every UI surface uses. Falls back to an empty font (SFML draws
    // nothing) rather than crashing when the asset is missing.
    static const sf::Font& font();
};
