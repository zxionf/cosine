#pragma once

#include "ui/core/label.hpp"
#include "ui/theme.hpp"

#include <string>

namespace xel::ui
{
// 排版角色：把「用途」映射到字号/颜色，布局与主题无关。
enum class TextRole
{
    Display,
    Title,
    Heading,
    Subheading,
    Body,
    Caption,
};

struct TextStyle
{
    float size = 16.0f;
    glm::vec4 color{1.0f, 1.0f, 1.0f, 1.0f};
    float line_spacing = 0.0f;
};

inline TextStyle text_style(TextRole role, const Theme& theme)
{
    const float base = theme.text_size;
    TextStyle style;
    style.color = theme.text;
    switch (role) {
        case TextRole::Display:
            style.size = base * 2.25f;
            style.line_spacing = base * 0.20f;
            break;
        case TextRole::Title:
            style.size = base * 1.75f;
            style.line_spacing = base * 0.15f;
            break;
        case TextRole::Heading:
            style.size = base * 1.35f;
            style.line_spacing = base * 0.10f;
            break;
        case TextRole::Subheading:
            style.size = base * 1.10f;
            style.line_spacing = base * 0.05f;
            break;
        case TextRole::Body:
            style.size = base;
            break;
        case TextRole::Caption:
            style.size = base * 0.82f;
            style.color = theme.text_muted;
            break;
    }
    return style;
}

// 语法糖：给已有 Label 套用角色。
inline void apply_text_role(Label& label, TextRole role)
{
    const TextStyle style = text_style(role, label.ui_theme());
    label.set_text_size(style.size);
    label.set_text_color(style.color);
    label.set_line_spacing(style.line_spacing);
}

class Heading : public Label
{
public:
    explicit Heading(std::string text, int level = 1)
        : Label(std::move(text))
    {
        set_level(level);
    }

    void set_level(int level)
    {
        level_ = std::clamp(level, 1, 3);
        set_text_size(text_style(role(), ui_theme()).size);
        mark_layout_dirty();
    }
    int level() const { return level_; }

private:
    TextRole role() const
    {
        switch (level_) {
            case 1: return TextRole::Title;
            case 2: return TextRole::Heading;
            default: return TextRole::Subheading;
        }
    }

    int level_ = 1;
};

// 段落：默认开启换行，便于展示长文本排版。
class Paragraph : public Label
{
public:
    explicit Paragraph(std::string text, float size = 0.0f)
        : Label(std::move(text), size)
    {
        set_wrap(true);
    }
};
} // namespace xel::ui
