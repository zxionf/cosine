#pragma once

#include <glm/glm.hpp>

#include <string>

namespace xel::editor
{
// 编辑器设置（内存态；持久化后续再做）。
struct EditorSettings
{
    // General
    bool rounded_style = false;
    bool light_theme = false;
    float ui_text_size = 24.0f;

    // Preview
    glm::vec2 preview_canvas{1280.0f, 720.0f};
    int preview_fit = 0; // 0=Contain, 1=Cover, 2=Fill
    glm::vec4 preview_background{0.06f, 0.07f, 0.08f, 1.0f};
    bool show_ruler = true;

    // Export (CO-3)
    glm::vec2 export_size{1920.0f, 1080.0f};
    int export_fps = 30;
    glm::vec4 export_background{0.0f, 0.0f, 0.0f, 1.0f};
    std::string export_directory;
};
} // namespace xel::editor
