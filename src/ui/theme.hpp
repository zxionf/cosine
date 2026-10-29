#pragma once

#include <cmath>
#include <cstdint>

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

// 颜色书写约定（UI-3）：
//   主题里的颜色一律按 **sRGB 直觉值**书写（十六进制），存储为**线性值**。
//   渲染目标是 *_SRGB 格式，硬件在写出时做线性 → sRGB 编码，
//   所以着色器必须拿到线性色，反过来说：写 #232629 就应该显示成 #232629。
inline float srgb_to_linear(float channel)
{
    if (channel <= 0.04045f) return channel / 12.92f;
    return std::pow((channel + 0.055f) / 1.055f, 2.4f);
}

inline glm::vec4 srgb_hex(std::uint32_t rgb, float alpha = 1.0f)
{
    const float r = static_cast<float>((rgb >> 16) & 0xFFu) / 255.0f;
    const float g = static_cast<float>((rgb >> 8) & 0xFFu) / 255.0f;
    const float b = static_cast<float>(rgb & 0xFFu) / 255.0f;
    return {srgb_to_linear(r), srgb_to_linear(g), srgb_to_linear(b), alpha};
}

struct Theme
{
    // 令牌：shell（窗口底）< surface（面板）< control（控件）< 文字。
    // 强调色 accent 是所有选中/焦点填充的唯一来源。
    glm::vec4 window_bg{srgb_hex(0x1B1D21)};
    glm::vec4 panel_bg{srgb_hex(0x232629)};
    glm::vec4 panel_border{srgb_hex(0x3A4048)};
    // 浮层（窗口）专用：比 panel_bg 亮一档，压在其他面板之上时能看出层次。
    glm::vec4 elevated{srgb_hex(0x2A2E33)};
    glm::vec4 elevated_border{srgb_hex(0x454B54)};
    // 分割条常态色：比表面亮一档，让三栅之间能看到可抓的边界。
    glm::vec4 splitter{srgb_hex(0x3F454D)};
    glm::vec4 text{srgb_hex(0xE4E7EB)};
    glm::vec4 text_disabled{srgb_hex(0x5F666E)};
    glm::vec4 text_muted{srgb_hex(0x9BA3AD)};

    glm::vec4 button_bg{srgb_hex(0x41474F)};
    glm::vec4 button_hover{srgb_hex(0x4C525A)};
    glm::vec4 button_pressed{srgb_hex(0x363B42)};
    glm::vec4 button_disabled{srgb_hex(0x30343A)};
    glm::vec4 focus_ring{srgb_hex(0x3D8EE8)};
    // 强调色：焦点环、勾选、滑动填充、进度填充的唯一来源。
    glm::vec4 accent{srgb_hex(0x3D8EE8)};
    // 强调色当**文字**时用更亮的一档：蓝底上写蓝色文字只有 4.1:1，不够读。
    glm::vec4 accent_text{srgb_hex(0x6FB0F2)};

    glm::vec4 checkbox_bg{srgb_hex(0x2F343A)};
    glm::vec4 checkbox_border{srgb_hex(0x4A5058)};
    glm::vec4 checkbox_fill{srgb_hex(0x3D8EE8)};

    glm::vec4 slider_track{srgb_hex(0x3A3F45)};
    glm::vec4 slider_fill{srgb_hex(0x3D8EE8)};
    glm::vec4 slider_handle{srgb_hex(0xD8DDE3)};
    // 滑轨 hover 态底色（轨道是 Track 角色，hover 只提亮一档，不用按钮色）。
    glm::vec4 track_hover{srgb_hex(0x4A5058)};

    glm::vec4 text_input_bg{srgb_hex(0x1E2125)};
    glm::vec4 text_input_border{srgb_hex(0x41474F)};
    glm::vec4 text_input_selection{srgb_hex(0x3D8EE8, 0.35f)};
    glm::vec4 text_input_cursor{srgb_hex(0xE4E7EB)};
    glm::vec4 text_input_border_hover{srgb_hex(0x5A6169)};

    glm::vec4 tooltip_bg{srgb_hex(0x2C3034, 0.97f)};
    glm::vec4 scrollbar_track{srgb_hex(0x2E3238)};
    glm::vec4 scrollbar_thumb{srgb_hex(0x5A6169)};

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

    // 状态过渡时长（毫秒）：0 表示关闭动画，颜色立即到位。
    float transition_ms = 120.0f;

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
    // 状态过渡时长（毫秒）：负值归零（关闭动画）。
    Theme with_transition_ms(float ms) const;
    // 圆角整体缩放：0 = 直角，1 = 默认档位，2 = 双倍；仅 0 时连 pill（圆形/胶囊）也归零。
    Theme with_radius_scale(float scale) const;

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

inline Theme Theme::with_radius_scale(float scale) const
{
    Theme theme_value = *this;
    const float k = scale < 0.0f ? 0.0f : scale;
    theme_value.radius *= k;
    theme_value.radius_small *= k;
    theme_value.radius_medium *= k;
    theme_value.radius_large *= k;
    theme_value.radius_panel *= k;
    // pill 是「足够大 + clamp 到短边一半」的语义值：保持大值，只在 0 时变成直角。
    if (k <= 0.0f) theme_value.radius_pill = 0.0f;
    return theme_value;
}

inline Theme Theme::with_transition_ms(float ms) const
{
    Theme theme_value = *this;
    theme_value.transition_ms = ms < 0.0f ? 0.0f : ms;
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

    t.window_bg       = srgb_hex(0xEDEFF2);
    t.panel_bg        = srgb_hex(0xDEE1E5);
    t.panel_border    = srgb_hex(0xB3B8BF);
    t.elevated        = srgb_hex(0xFAFBFC);
    t.elevated_border = srgb_hex(0xA8AEB6);
    t.splitter        = srgb_hex(0xC3C7CD);
    t.text            = srgb_hex(0x1A1D21);
    t.text_disabled   = srgb_hex(0x6B7178);
    t.text_muted      = srgb_hex(0x5C6269);

    t.button_bg       = srgb_hex(0xC8CCD2);
    t.button_hover    = srgb_hex(0xDCE0E5);
    t.button_pressed  = srgb_hex(0xB4B9C0);
    t.button_disabled = srgb_hex(0xE0E3E7);
    t.focus_ring      = srgb_hex(0x1F6FD0);
    t.accent          = srgb_hex(0x1F6FD0);
    t.accent_text     = srgb_hex(0x1A5FB4);

    t.checkbox_bg     = srgb_hex(0xF7F8FA);
    t.checkbox_border = srgb_hex(0x9EA3AA);
    t.checkbox_fill   = srgb_hex(0x1F6FD0);

    t.slider_track    = srgb_hex(0xC7CBD1);
    t.slider_fill     = srgb_hex(0x1F6FD0);
    t.slider_handle   = srgb_hex(0x292D33);
    t.track_hover     = srgb_hex(0xADB2B9);

    t.text_input_bg           = srgb_hex(0xFAFBFC);
    t.text_input_border       = srgb_hex(0xADB2B9);
    t.text_input_selection    = srgb_hex(0x1F6FD0, 0.30f);
    t.text_input_cursor       = srgb_hex(0x0D0F11);
    t.text_input_border_hover = srgb_hex(0x727880);

    t.tooltip_bg      = srgb_hex(0xF7F8FA, 0.98f);
    t.scrollbar_track = srgb_hex(0xCCD0D5);
    t.scrollbar_thumb = srgb_hex(0x8C9299);
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

// 主题版本号：每次应用新主题都自增。控件用它让样式过渡缓存立即失效，
// 否则切换主题/圆角/密度后控件会继续用旧主题解析出的颜色，直到下次状态变化。
inline std::uint64_t& mutable_theme_revision()
{
    static std::uint64_t value = 0;
    return value;
}

inline std::uint64_t theme_revision()
{
    return mutable_theme_revision();
}

inline void bump_theme_revision()
{
    ++mutable_theme_revision();
}

inline void set_theme(const Theme& value)
{
    mutable_theme() = value;
    bump_theme_revision();
}
} // namespace xel::ui
