#pragma once

#include <cmath>
#include <cstdint>

#include <glm/glm.hpp>

#include "ui/style_types.hpp"

#include "ui/scroll_mode.hpp"

namespace xel::ui
{
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
    // 字号档位（UI-41）：角色想表达「这类控件用小字」时读它，而不是让控件直读 small_text_size。
    TextSize text_size_role = TextSize::Body;
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
    // 强调色：焦点环、勾选、滑动填充、进度填充的唯一来源。
    glm::vec4 accent{srgb_hex(0x3D8EE8)};
    // 强调色当**文字**时用更亮的一档：蓝底上写蓝色文字只有 4.1:1，不够读。
    glm::vec4 accent_text{srgb_hex(0x6FB0F2)};
    // accent 填充上的文字色（UI-39）：深色主题下深字、浅色主题下白字。
    // 对比度要求 >= 4.5:1（WCAG AA 正文级），由 test_ui_style 断言。
    glm::vec4 accent_on{srgb_hex(0x0F1419)};

    glm::vec4 slider_track{srgb_hex(0x3A3F45)};
    glm::vec4 slider_handle{srgb_hex(0xD8DDE3)};
    // 滑轨 hover 态底色（轨道是 Track 角色，hover 只提亮一档，不用按钮色）。
    glm::vec4 track_hover{srgb_hex(0x4A5058)};

    // 滚动条滑块（UI-46）：与 Track 槽的对比度 >= 2.2:1（dark 2.31 / light 2.42）。
    // 不复用 track_hover（只提亮一档，浅色主题下只有 1.3:1，滑块等于看不见）。
    glm::vec4 thumb{srgb_hex(0x6E7680)};
    glm::vec4 thumb_hover{srgb_hex(0x868E98)};

    glm::vec4 text_input_bg{srgb_hex(0x1E2125)};
    glm::vec4 text_input_border{srgb_hex(0x41474F)};
    glm::vec4 text_input_selection{srgb_hex(0x3D8EE8, 0.35f)};
    glm::vec4 text_input_cursor{srgb_hex(0xE4E7EB)};
    glm::vec4 text_input_border_hover{srgb_hex(0x5A6169)};

    glm::vec4 tooltip_bg{srgb_hex(0x2C3034, 0.97f)};

    // 内阴影：只用于 Surface 上缘（Panel），多层半透明条模拟。
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
    // 窗口 resize 命中带（像素）：边缘该范围内的鼠标命中 resize，而非内容。
    float resize_margin = 4.0f;
    // 布局字面量（UI-35 收口）：以前散在各控件里的“魔法数字”。
    //   dialog_*：DialogHost 的外边距与对话框最小/最大尺寸
    //   window_*：窗口最小宽、标题之外的最小内容高、拖动时保留的最小可见量
    //   splitter_handle：Splitter 手柄厚度
    //   section_inset：Section 内容相对标题条的内缩
    float dialog_margin = 24.0f;
    float dialog_min_width = 120.0f;
    float dialog_min_height = 80.0f;
    float window_min_width = 160.0f;
    float window_min_extra_height = 48.0f;
    float window_keep_visible = 48.0f;
    float splitter_handle = 6.0f;
    float section_inset = 12.0f;
    // 全局默认滚动模式；ScrollView 未显式覆盖时采用。
    ScrollMode scroll_mode = ScrollMode::Smooth;

    // 圆角档位：radius 是控件默认圆角，radius_panel 是面板/卡片。
    float radius = 6.0f;
    float radius_small = 3.0f;
    float radius_medium = 6.0f;
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
    // 文字绘制是否对齐到整像素；控件可用 Label::set_pixel_snap 单独覆盖。
    bool pixel_snap = true;

    Density density = Density::Normal;

    // 主题标识，用于 UI 显示与切换判断。
    const char* name = "dark";

    // 解析「角色 + 状态」到具体外观；实现见 ui/style.hpp。
    BoxStyle style(StyleRole role, ControlState state) const;
    // 字号档位 → 像素（UI-41）：角色内部用它，控件需要单点小字号（如菜单快捷键）也可直接问。
    float text_size_of(TextSize tier) const
    {
        switch (tier) {
            case TextSize::Small:   return small_text_size;
            case TextSize::Heading: return heading_text_size;
            case TextSize::Body:    break;
        }
        return text_size;
    }
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

// 无 UIContext 时的只读回退主题（控件在独立测试/离屏工具里可能没有 context）。
// 注意：主题改动只能经 `UIContext::set_theme()`；全局 version 已移除（UI-42）。
inline const Theme& theme()
{
    static const Theme value = Theme::dark();
    return value;
}
} // namespace xel::ui
