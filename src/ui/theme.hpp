#pragma once

#include <glm/glm.hpp>

namespace xel::ui
{
// 样式角色：把控件抽象成「用途」，Theme 按角色而不是控件名提供外观。
// 新控件优先复用已有角色，避免每加一个控件就往 Theme 里塞一整套字段。
enum class StyleRole
{
    Surface,  // 面板 / 卡片底
    Control,  // 按钮类：Button、Checkbox 方框、Slider 手柄
    Input,    // 输入类框体：TextInput、Dropdown 框
    Track,    // 滑轨 / 进度 / 滚动条槽
    Accent,   // 选中与强调填充：勾选、滑动填充、进度填充
    Row,      // 列表行：Table / TreeView / List
    MenuItem, // 菜单项
    Tooltip,  // 浮层信息
};

// 交互状态：填充只看这一个维度。焦点不是填充状态，焦点环是叠加层，可与 hover 共存。
// 优先级：Disabled > Pressed > Hovered > Normal。
enum class ControlState
{
    Normal,
    Hovered,
    Pressed,
    Disabled,
};

// 密度档位：只缩放度量（间距、控件高度、字号、圆角），不改变布局规则。
enum class Density
{
    Compact,
    Normal,
    Spacious,
};

// 焦点绘制风格：
// Outline = 外扩圆角环（圆角样式），Border = 贝控件内边缘的四条实心框（经典样式）。
enum class FocusStyle
{
    Outline,
    Border,
};

// 焦点显示时机：与 FocusStyle 正交，可任意组合。
// Keyboard = 仅键盘操作期间显示，Always = 聚焦即显示，Never = 从不显示。
enum class FocusPolicy
{
    Keyboard,
    Always,
    Never,
};

// 一处解析结果：控件拿它直接绘制，不再自己写颜色分支。
struct BoxStyle
{
    glm::vec4 fill{0.0f, 0.0f, 0.0f, 0.0f};
    glm::vec4 border{0.0f, 0.0f, 0.0f, 0.0f};
    float border_width = 0.0f;
    float radius = 0.0f;
    glm::vec4 text{1.0f, 1.0f, 1.0f, 1.0f};
    float text_size = 16.0f;
    glm::vec4 focus_ring{1.0f, 1.0f, 1.0f, 1.0f};
};

struct Theme
{
    glm::vec4 window_bg{0.08f, 0.09f, 0.10f, 1.0f};
    glm::vec4 panel_bg{0.13f, 0.14f, 0.16f, 1.0f};
    glm::vec4 panel_border{0.25f, 0.27f, 0.30f, 1.0f};
    glm::vec4 text{0.90f, 0.92f, 0.95f, 1.0f};
    glm::vec4 text_disabled{0.50f, 0.52f, 0.55f, 1.0f};
    glm::vec4 text_muted{0.66f, 0.69f, 0.73f, 1.0f};

    glm::vec4 button_bg{0.20f, 0.22f, 0.25f, 1.0f};
    glm::vec4 button_hover{0.26f, 0.29f, 0.33f, 1.0f};
    glm::vec4 button_pressed{0.16f, 0.18f, 0.21f, 1.0f};
    glm::vec4 button_disabled{0.15f, 0.16f, 0.18f, 1.0f};
    glm::vec4 focus_ring{0.20f, 0.60f, 1.00f, 1.0f};
    // 强调色：焦点环、勾选、滑动填充、进度填充的唯一来源。
    glm::vec4 accent{0.20f, 0.60f, 1.00f, 1.0f};

    glm::vec4 checkbox_bg{0.18f, 0.20f, 0.23f, 1.0f};
    glm::vec4 checkbox_border{0.35f, 0.38f, 0.42f, 1.0f};
    glm::vec4 checkbox_fill{0.20f, 0.60f, 1.00f, 1.0f};

    glm::vec4 slider_track{0.22f, 0.24f, 0.27f, 1.0f};
    glm::vec4 slider_fill{0.20f, 0.60f, 1.00f, 1.0f};
    glm::vec4 slider_handle{0.85f, 0.88f, 0.92f, 1.0f};
    // 滑轨 hover 态底色（轨道是 Track 角色，hover 只提亮一档，不用按钮色）。
    glm::vec4 track_hover{0.30f, 0.33f, 0.37f, 1.0f};

    glm::vec4 text_input_bg{0.10f, 0.11f, 0.13f, 1.0f};
    glm::vec4 text_input_border{0.30f, 0.33f, 0.37f, 1.0f};
    glm::vec4 text_input_selection{0.20f, 0.60f, 1.00f, 0.35f};
    glm::vec4 text_input_cursor{0.90f, 0.92f, 0.95f, 1.0f};
    glm::vec4 text_input_border_hover{0.45f, 0.48f, 0.52f, 1.0f};

    glm::vec4 tooltip_bg{0.10f, 0.11f, 0.13f, 0.95f};
    glm::vec4 scrollbar_track{0.18f, 0.19f, 0.22f, 1.0f};
    glm::vec4 scrollbar_thumb{0.42f, 0.45f, 0.50f, 1.0f};

    // 内阴影：只用于 Surface 上缘与 Tooltip 内缘，多层半透明条模拟。
    glm::vec4 inset_shadow{0.0f, 0.0f, 0.0f, 0.06f};
    float inset_shadow_size = 6.0f;

    float text_size = 16.0f;
    float small_text_size = 12.0f;
    float heading_text_size = 22.0f;
    float padding = 8.0f;
    float spacing = 6.0f;
    float border = 1.0f;
    float control_height = 30.0f;
    float row_height = 26.0f;
    float checkbox_size = 18.0f;
    float slider_height = 6.0f;
    float slider_handle_radius = 7.0f;
    float scrollbar_width = 8.0f;
    float scrollbar_min_thumb = 20.0f;

    // 圆角档位：radius 是控件默认圆角，radius_panel 是面板/卡片。
    float radius = 6.0f;
    float radius_small = 3.0f;
    float radius_medium = 6.0f;
    float radius_large = 10.0f;
    float radius_panel = 8.0f;
    // 胶囊/圆形圆角：取值故意很大，绘制时 clamp 到短边一半；classic 样式归零后变直角。
    // Slider 轨道与手柄、Progress、Radio 用它，保证「圆形控件」也随圆角开关变化。
    float radius_pill = 1000.0f;
    // 焦点环：画在控件外扩 focus_offset 的矩形上，厚度 focus_thickness。
    float focus_thickness = 2.0f;
    float focus_offset = 2.0f;
    // 经典焦点框贴边、不外扩，因此不使用 focus_offset。
    FocusStyle focus_style = FocusStyle::Outline;
    // 何时显示焦点框；与几何风格独立，由 with_focus_policy 设置。
    FocusPolicy focus_policy = FocusPolicy::Keyboard;

    Density density = Density::Normal;

    // 主题标识，用于 UI 显示与切换判断。
    const char* name = "dark";

    // 解析「角色 + 状态」到具体外观；实现见 ui/style.hpp。
    BoxStyle style(StyleRole role, ControlState state) const;
    // 返回换了密度档位的副本；已处于目标档位时原样返回（幂等）。
    Theme with_density(Density target) const;
    // 圆角与焦点风格的整组开关：rounded=false 时所有圆角归零并改用贴边焦点框（经典样式）。
    // 单向变换：要回到圆角样式，请从基础主题重新派生（如 base.with_rounding(true)）。
    Theme with_rounding(bool rounded) const;
    // 焦点显示时机：与几何风格正交，链式调用时后写者胜。
    Theme with_focus_policy(FocusPolicy policy) const;

    static Theme dark() { return Theme{}; }
    static Theme light();
};

inline float density_metric_factor(Density density)
{
    switch (density) {
        case Density::Compact:  return 0.85f;
        case Density::Spacious: return 1.15f;
        case Density::Normal:   break;
    }
    return 1.0f;
}

inline float density_text_factor(Density density)
{
    switch (density) {
        case Density::Compact:  return 0.90f;
        case Density::Spacious: return 1.10f;
        case Density::Normal:   break;
    }
    return 1.0f;
}

inline Theme Theme::with_density(Density target) const
{
    Theme theme_value = *this;
    const float metric = density_metric_factor(target) / density_metric_factor(density);
    const float text_scale = density_text_factor(target) / density_text_factor(density);
    theme_value.density = target;

    theme_value.padding *= metric;
    theme_value.spacing *= metric;
    theme_value.control_height *= metric;
    theme_value.row_height *= metric;
    theme_value.checkbox_size *= metric;
    theme_value.slider_height *= metric;
    theme_value.slider_handle_radius *= metric;
    theme_value.scrollbar_width *= metric;
    theme_value.scrollbar_min_thumb *= metric;
    theme_value.inset_shadow_size *= metric;

    theme_value.text_size *= text_scale;
    theme_value.small_text_size *= text_scale;
    theme_value.heading_text_size *= text_scale;
    theme_value.radius *= text_scale;
    theme_value.radius_small *= text_scale;
    theme_value.radius_medium *= text_scale;
    theme_value.radius_large *= text_scale;
    theme_value.radius_panel *= text_scale;
    theme_value.radius_pill *= text_scale;
    return theme_value;
}

inline Theme Theme::with_focus_policy(FocusPolicy policy) const
{
    Theme theme_value = *this;
    theme_value.focus_policy = policy;
    return theme_value;
}

inline Theme Theme::with_rounding(bool rounded) const
{
    Theme theme_value = *this;
    if (rounded) {
        theme_value.focus_style = FocusStyle::Outline;
        return theme_value;
    }
    theme_value.radius = 0.0f;
    theme_value.radius_small = 0.0f;
    theme_value.radius_medium = 0.0f;
    theme_value.radius_large = 0.0f;
    theme_value.radius_panel = 0.0f;
    theme_value.radius_pill = 0.0f;
    // 内阴影属于圆角视觉的一部分：classic 完全回到旧观感。
    theme_value.inset_shadow_size = 0.0f;
    theme_value.focus_style = FocusStyle::Border;
    return theme_value;
}

inline Theme Theme::light()
{
    Theme t;
    t.name = "light";

    t.window_bg       = {0.93f, 0.94f, 0.95f, 1.0f};
    t.panel_bg        = {0.87f, 0.88f, 0.90f, 1.0f};
    t.panel_border    = {0.70f, 0.72f, 0.75f, 1.0f};
    t.text            = {0.03f, 0.035f, 0.04f, 1.0f};
    t.text_disabled   = {0.42f, 0.44f, 0.47f, 1.0f};
    t.text_muted      = {0.36f, 0.38f, 0.41f, 1.0f};

    t.button_bg       = {0.82f, 0.84f, 0.87f, 1.0f};
    t.button_hover    = {0.89f, 0.91f, 0.94f, 1.0f};
    t.button_pressed  = {0.74f, 0.76f, 0.79f, 1.0f};
    t.button_disabled = {0.90f, 0.91f, 0.92f, 1.0f};
    t.focus_ring      = {0.12f, 0.45f, 0.95f, 1.0f};
    t.accent          = {0.12f, 0.45f, 0.95f, 1.0f};

    t.checkbox_bg     = {0.97f, 0.98f, 0.99f, 1.0f};
    t.checkbox_border = {0.62f, 0.65f, 0.68f, 1.0f};
    t.checkbox_fill   = {0.12f, 0.45f, 0.95f, 1.0f};

    t.slider_track    = {0.78f, 0.80f, 0.83f, 1.0f};
    t.slider_fill     = {0.12f, 0.45f, 0.95f, 1.0f};
    t.slider_handle   = {0.16f, 0.17f, 0.19f, 1.0f};
    t.track_hover     = {0.68f, 0.71f, 0.75f, 1.0f};

    t.text_input_bg           = {0.98f, 0.98f, 0.99f, 1.0f};
    t.text_input_border       = {0.68f, 0.70f, 0.73f, 1.0f};
    t.text_input_selection    = {0.12f, 0.45f, 0.95f, 0.30f};
    t.text_input_cursor       = {0.05f, 0.055f, 0.06f, 1.0f};
    t.text_input_border_hover = {0.45f, 0.47f, 0.50f, 1.0f};

    t.tooltip_bg      = {0.97f, 0.97f, 0.98f, 0.98f};
    t.scrollbar_track = {0.80f, 0.82f, 0.85f, 1.0f};
    t.scrollbar_thumb = {0.55f, 0.58f, 0.62f, 1.0f};
    t.inset_shadow    = {0.0f, 0.0f, 0.0f, 0.05f};
    return t;
}

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
