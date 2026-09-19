#pragma once

#include <string>
#include <vector>
#include <map>
#include <glm/glm.hpp>

namespace xel::ui
{
    struct Color
    {
        float r = 1.0f, g = 1.0f, b = 1.0f, a = 1.0f;
        Color() = default;
        Color(float r, float g, float b, float a = 1.0f) : r(r), g(g), b(b), a(a) {}
    };

    struct Style
    {
        Color bgColor = Color(0.15f, 0.15f, 0.18f);
        Color fgColor = Color(0.9f, 0.9f, 0.9f);
        Color accentColor = Color(0.2f, 0.6f, 1.0f);
        Color borderColor = Color(0.3f, 0.3f, 0.35f);
        Color hoverColor = Color(0.25f, 0.25f, 0.3f);
        Color activeColor = Color(0.35f, 0.35f, 0.4f);
        float cornerRadius = 6.0f;
        float padding = 8.0f;
        float spacing = 4.0f;
        float fontSize = 16.0f;
        std::string fontPath = "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf";
    };

    struct Theme
    {
        Style style;
        std::map<std::string, Color> customColors;

        const Style& getStyle() const { return style; }
        void setStyle(const Style& s) { style = s; }
    };
}
