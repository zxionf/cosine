#pragma once

#include <glm/glm.hpp>

namespace xel::ui
{
struct Theme
{
    glm::vec4 window_bg{0.08f, 0.09f, 0.10f, 1.0f};
    glm::vec4 panel_bg{0.13f, 0.14f, 0.16f, 1.0f};
    glm::vec4 panel_border{0.25f, 0.27f, 0.30f, 1.0f};
    glm::vec4 text{0.90f, 0.92f, 0.95f, 1.0f};
    glm::vec4 text_disabled{0.50f, 0.52f, 0.55f, 1.0f};

    glm::vec4 button_bg{0.20f, 0.22f, 0.25f, 1.0f};
    glm::vec4 button_hover{0.26f, 0.29f, 0.33f, 1.0f};
    glm::vec4 button_pressed{0.16f, 0.18f, 0.21f, 1.0f};
    glm::vec4 button_disabled{0.15f, 0.16f, 0.18f, 1.0f};
    glm::vec4 focus_ring{0.20f, 0.60f, 1.00f, 1.0f};

    glm::vec4 checkbox_bg{0.18f, 0.20f, 0.23f, 1.0f};
    glm::vec4 checkbox_border{0.35f, 0.38f, 0.42f, 1.0f};
    glm::vec4 checkbox_fill{0.20f, 0.60f, 1.00f, 1.0f};

    glm::vec4 slider_track{0.22f, 0.24f, 0.27f, 1.0f};
    glm::vec4 slider_fill{0.20f, 0.60f, 1.00f, 1.0f};
    glm::vec4 slider_handle{0.85f, 0.88f, 0.92f, 1.0f};

    glm::vec4 text_input_bg{0.10f, 0.11f, 0.13f, 1.0f};
    glm::vec4 text_input_border{0.30f, 0.33f, 0.37f, 1.0f};
    glm::vec4 text_input_selection{0.20f, 0.60f, 1.00f, 0.35f};
    glm::vec4 text_input_cursor{0.90f, 0.92f, 0.95f, 1.0f};

    glm::vec4 tooltip_bg{0.10f, 0.11f, 0.13f, 0.95f};

    float text_size = 16.0f;
    float small_text_size = 12.0f;
    float padding = 8.0f;
    float spacing = 6.0f;
    float border = 1.0f;
    float control_height = 30.0f;
    float checkbox_size = 18.0f;
    float slider_height = 6.0f;
    float slider_handle_radius = 7.0f;

    // 主题标识，用于 UI 显示与切换判断。
    const char* name = "dark";

    static Theme dark() { return Theme{}; }

    static Theme light()
    {
        Theme t;
        t.name = "light";

        t.window_bg       = {0.93f, 0.94f, 0.95f, 1.0f};
        t.panel_bg        = {0.87f, 0.88f, 0.90f, 1.0f};
        t.panel_border    = {0.70f, 0.72f, 0.75f, 1.0f};
        t.text            = {0.03f, 0.035f, 0.04f, 1.0f};
        t.text_disabled   = {0.42f, 0.44f, 0.47f, 1.0f};

        t.button_bg       = {0.82f, 0.84f, 0.87f, 1.0f};
        t.button_hover    = {0.89f, 0.91f, 0.94f, 1.0f};
        t.button_pressed  = {0.74f, 0.76f, 0.79f, 1.0f};
        t.button_disabled = {0.90f, 0.91f, 0.92f, 1.0f};
        t.focus_ring      = {0.12f, 0.45f, 0.95f, 1.0f};

        t.checkbox_bg     = {0.97f, 0.98f, 0.99f, 1.0f};
        t.checkbox_border = {0.62f, 0.65f, 0.68f, 1.0f};
        t.checkbox_fill   = {0.12f, 0.45f, 0.95f, 1.0f};

        t.slider_track    = {0.78f, 0.80f, 0.83f, 1.0f};
        t.slider_fill     = {0.12f, 0.45f, 0.95f, 1.0f};
        t.slider_handle   = {0.16f, 0.17f, 0.19f, 1.0f};

        t.text_input_bg        = {0.98f, 0.98f, 0.99f, 1.0f};
        t.text_input_border    = {0.68f, 0.70f, 0.73f, 1.0f};
        t.text_input_selection = {0.12f, 0.45f, 0.95f, 0.30f};
        t.text_input_cursor    = {0.05f, 0.055f, 0.06f, 1.0f};

        t.tooltip_bg = {0.97f, 0.97f, 0.98f, 0.98f};
        return t;
    }
};

inline Theme& mutable_theme()
{
    static Theme value = Theme::dark();
    return value;
}

inline const Theme& theme()
{
    return mutable_theme();
}

inline void set_theme(const Theme& value)
{
    mutable_theme() = value;
}
} // namespace xel::ui
