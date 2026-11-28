#include "graphics/box.hpp"
#include "ui/core/button.hpp"
#include "ui/core/checkbox.hpp"
#include "ui/core/progress.hpp"
#include "ui/core/radio.hpp"
#include "ui/system/root.hpp"
#include "ui/core/slider.hpp"
#include "ui/style/style.hpp"
#include "ui/style/theme.hpp"
#include "ui/system/ui_system.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>
#include <memory>
#include <vector>

namespace
{
using namespace xel;

bool near(float a, float b, float eps = 1e-4f)
{
    return std::fabs(a - b) <= eps;
}

struct Geometry
{
    size_t vertices = 0;
    size_t indices = 0;
};

Geometry stats(const graphics::DrawList& list)
{
    Geometry g;
    for (const auto& batch : list.batches()) {
        g.vertices += batch.vertices.size();
        g.indices += batch.indices.size();
    }
    return g;
}

// 所有顶点必须落在给定矩形内（浮点容差），索引必须合法。
// margin：圆角的抗锯齿边带（fringe）按设计会向外扩 1 物理像素，属于预期。
void assert_inside(const graphics::DrawList& list, glm::vec4 rect, float margin = 0.0f)
{
    const float slack = margin + 0.01f;
    for (const auto& batch : list.batches()) {
        for (const auto& vertex : batch.vertices) {
            assert(vertex.pos.x >= rect.x - slack);
            assert(vertex.pos.y >= rect.y - slack);
            assert(vertex.pos.x <= rect.x + rect.z + slack);
            assert(vertex.pos.y <= rect.y + rect.w + slack);
        }
        for (uint32_t index : batch.indices) {
            assert(index < batch.vertices.size());
        }
    }
}

void test_control_state()
{
    using ui::ControlState;

    // UI-52：状态映射唯一入口是 Interaction::state()（自由函数 control_state 已删除）。
    const auto state_of = [](bool enabled, bool hovered, bool pressed, bool checked = false) {
        ui::Interaction state;
        state.enabled = enabled;
        state.hovered = hovered;
        state.pressed = pressed;
        return state.state(checked);
    };
    assert(state_of(true, false, false) == ControlState::Normal);
    assert(state_of(true, true, false) == ControlState::Hovered);
    assert(state_of(true, true, true) == ControlState::Pressed);
    // 优先级：Disabled > Pressed > Hovered > Checked > Normal。
    assert(state_of(false, true, true) == ControlState::Disabled);
    assert(state_of(true, false, true) == ControlState::Pressed);
    assert(state_of(true, false, false, true) == ControlState::Checked);
}

void test_style_roles()
{
    using ui::ControlState;
    using ui::StyleRole;

    const ui::Theme dark = ui::Theme::dark();
    const ui::Theme light = ui::Theme::light();

    // Control：四态分别落到按钮调色板，禁用时文字降级。
    assert(dark.style(StyleRole::Control, ControlState::Normal).fill == dark.button_bg);
    assert(dark.style(StyleRole::Control, ControlState::Hovered).fill == dark.button_hover);
    assert(dark.style(StyleRole::Control, ControlState::Pressed).fill == dark.button_pressed);
    assert(dark.style(StyleRole::Control, ControlState::Disabled).fill == dark.button_disabled);
    assert(dark.style(StyleRole::Control, ControlState::Normal).text == dark.text);
    assert(dark.style(StyleRole::Control, ControlState::Disabled).text == dark.text_disabled);

    // Surface：带面板边框与面板圆角。
    const auto surface = dark.style(StyleRole::Surface, ControlState::Normal);
    assert(surface.fill == dark.panel_bg);
    assert(surface.border == dark.panel_border);
    assert(near(surface.border_width, dark.border));
    assert(near(surface.radius, dark.radius_panel));

    // Input：hover 提亮边框而不是换底色。
    const auto input = dark.style(StyleRole::Input, ControlState::Hovered);
    assert(input.fill == dark.text_input_bg);
    assert(input.border == dark.text_input_border_hover);
    assert(light.style(StyleRole::Input, ControlState::Hovered).border == light.text_input_border_hover);

    // Track：hover 用 track_hover。
    assert(dark.style(StyleRole::Track, ControlState::Normal).fill == dark.slider_track);
    assert(dark.style(StyleRole::Track, ControlState::Hovered).fill == dark.track_hover);

    // Accent：强调色唯一来源，禁用时降级为 text_disabled。
    assert(dark.style(StyleRole::Accent, ControlState::Normal).fill == dark.accent);
    assert(dark.style(StyleRole::Accent, ControlState::Disabled).fill == dark.text_disabled);

    // Row / MenuItem：默认透明，hover/pressed 沿用按钮色系（旧观感），圆角来自 radius_small。
    assert(dark.style(StyleRole::Row, ControlState::Normal).fill.a == 0.0f);
    assert(dark.style(StyleRole::Row, ControlState::Hovered).fill == dark.button_hover);
    assert(dark.style(StyleRole::Row, ControlState::Pressed).fill == dark.button_pressed);
    assert(dark.style(StyleRole::MenuItem, ControlState::Hovered).fill == dark.button_hover);
    assert(dark.style(StyleRole::MenuItem, ControlState::Pressed).fill == dark.button_pressed);

    // Tooltip：浮层底色 + 中档圆角。
    const auto tooltip = dark.style(StyleRole::Tooltip, ControlState::Normal);
    assert(tooltip.fill == dark.tooltip_bg);
    assert(near(tooltip.radius, dark.radius_medium));

    // 焦点环颜色统一取 accent（BoxStyle 不再携带 focus_ring，直接读主题）。
    assert(dark.accent == ui::Theme::dark().accent);
}

void test_density()
{
    using ui::Density;

    const ui::Theme normal = ui::Theme::dark();
    const ui::Theme compact = normal.with_density(Density::Compact);
    assert(compact.density == Density::Compact);
    assert(compact.padding < normal.padding);
    assert(compact.control_height < normal.control_height);
    assert(compact.text_size < normal.text_size);
    assert(compact.radius < normal.radius);

    const ui::Theme spacious = normal.with_density(Density::Spacious);
    assert(spacious.padding > normal.padding);
    assert(spacious.control_height > normal.control_height);
    assert(spacious.text_size > normal.text_size);

    // 换档可逆：Compact -> Normal 回到原始度量。
    const ui::Theme back = compact.with_density(Density::Normal);
    assert(near(back.padding, normal.padding));
    assert(near(back.control_height, normal.control_height));
    assert(near(back.text_size, normal.text_size));
    assert(near(back.radius, normal.radius));

    // 幂等：已在目标档位时不缩放。
    const ui::Theme same = normal.with_density(Density::Normal);
    assert(near(same.padding, normal.padding));
    assert(near(same.control_height, normal.control_height));
}

void test_rounded_rect_geometry()
{
    const glm::vec4 bounds{0.0f, 0.0f, 100.0f, 40.0f};

    // 小半径退化为普通矩形：4 顶点 / 6 索引，与旧路径一致。
    {
        graphics::DrawList list;
        list.set_full_clip({0.0f, 0.0f, 1000.0f, 1000.0f});
        graphics::append_rounded_rect(list, bounds, 0.0f, {1.0f, 1.0f, 1.0f, 1.0f});
        const auto g = stats(list);
        assert(g.vertices == 4);
        assert(g.indices == 6);
        assert_inside(list, bounds, 1.45f);
    }

    // 圆角路径：顶点增多、索引合法、全部落在矩形内。
    {
        graphics::DrawList list;
        list.set_full_clip({0.0f, 0.0f, 1000.0f, 1000.0f});
        graphics::append_rounded_rect(list, bounds, 6.0f, {1.0f, 1.0f, 1.0f, 1.0f});
        const auto g = stats(list);
        assert(g.vertices > 4);
        assert(g.indices % 3 == 0);
        assert_inside(list, bounds, 1.45f);
    }

    // 半径 clamp 到短边一半：越界半径与大半径结果一致。
    {
        graphics::DrawList a;
        a.set_full_clip({0.0f, 0.0f, 1000.0f, 1000.0f});
        graphics::append_rounded_rect(a, bounds, 20.0f, {1.0f, 1.0f, 1.0f, 1.0f});

        graphics::DrawList b;
        b.set_full_clip({0.0f, 0.0f, 1000.0f, 1000.0f});
        graphics::append_rounded_rect(b, bounds, 1000.0f, {1.0f, 1.0f, 1.0f, 1.0f});

        assert(stats(a).vertices == stats(b).vertices);
        assert(stats(a).indices == stats(b).indices);
        assert_inside(b, bounds, 1.45f);
    }

    // 尺寸为 0 或完全透明时不产生任何几何。
    {
        graphics::DrawList list;
        list.set_full_clip({0.0f, 0.0f, 1000.0f, 1000.0f});
        graphics::append_rounded_rect(list, {0.0f, 0.0f, 0.0f, 40.0f}, 6.0f, {1.0f, 1.0f, 1.0f, 1.0f});
        graphics::append_rounded_rect(list, bounds, 6.0f, {1.0f, 1.0f, 1.0f, 0.0f});
        assert(stats(list).vertices == 0);
    }
}

void test_rounded_ring_geometry()
{
    const glm::vec4 bounds{10.0f, 20.0f, 120.0f, 60.0f};

    // 半径为 0：四条直边矩形，不重叠、不遮挡中心。
    {
        graphics::DrawList list;
        list.set_full_clip({0.0f, 0.0f, 1000.0f, 1000.0f});
        graphics::append_rounded_ring(list, bounds, 0.0f, 2.0f, {1.0f, 1.0f, 1.0f, 1.0f});
        const auto g = stats(list);
        assert(g.vertices == 16);
        assert(g.indices == 24);
        assert_inside(list, bounds, 1.45f);
    }

    // 圆角环：顶点更多，厚度不为 0，全部落在矩形内。
    {
        graphics::DrawList list;
        list.set_full_clip({0.0f, 0.0f, 1000.0f, 1000.0f});
        graphics::append_rounded_ring(list, bounds, 8.0f, 2.0f, {1.0f, 1.0f, 1.0f, 1.0f});
        const auto g = stats(list);
        assert(g.vertices > 16);
        assert(g.indices % 3 == 0);
        assert_inside(list, bounds, 1.45f);
    }

    // 厚度为 0 或透明时不绘制。
    {
        graphics::DrawList list;
        list.set_full_clip({0.0f, 0.0f, 1000.0f, 1000.0f});
        graphics::append_rounded_ring(list, bounds, 8.0f, 0.0f, {1.0f, 1.0f, 1.0f, 1.0f});
        graphics::append_rounded_ring(list, bounds, 8.0f, 2.0f, {1.0f, 1.0f, 1.0f, 0.0f});
        assert(stats(list).vertices == 0);
    }

    // 内阴影：多层叠加，全部落在矩形内。
    {
        graphics::DrawList list;
        list.set_full_clip({0.0f, 0.0f, 1000.0f, 1000.0f});
        graphics::append_inset_shadow(list, bounds, 8.0f, 6.0f, {0.0f, 0.0f, 0.0f, 0.12f});
        assert(stats(list).vertices > 0);
        assert_inside(list, bounds, 1.45f);
    }
}

void test_rounding_switch()
{
    using ui::Density;
    using ui::FocusStyle;

    const ui::Theme modern = ui::Theme::dark();
    assert(modern.focus_style == FocusStyle::Outline);
    assert(modern.radius > 0.0f);

    // 经典样式：所有圆角归零、焦点框改回贴边四条实心框。
    const ui::Theme classic = modern.with_rounding(false);
    assert(classic.focus_style == FocusStyle::Border);
    assert(classic.radius == 0.0f);
    assert(classic.radius_small == 0.0f);
    assert(classic.radius_medium == 0.0f);
    assert(classic.radius_panel == 0.0f);
    // 只切形状，不动调色板与度量。
    assert(classic.panel_bg == modern.panel_bg);
    assert(classic.padding == modern.padding);
    assert(classic.accent == modern.accent);

    // 与密度可组合：先缩放到目标密度，再清零圆角。
    const ui::Theme classic_compact = modern.with_density(Density::Compact).with_rounding(false);
    assert(classic_compact.focus_style == FocusStyle::Border);
    assert(classic_compact.radius == 0.0f);
    assert(classic_compact.padding < modern.padding);

    // with_rounding(true) 只恢复焦点风格；圆角从基础主题重新派生。
    const ui::Theme restored = modern.with_rounding(true);
    assert(restored.focus_style == FocusStyle::Outline);
    assert(restored.radius == modern.radius);

    // 经典样式下圆角矩形退化为 4 顶点矩形：回到样式系统之前的绘制。
    graphics::DrawList list;
    list.set_full_clip({0.0f, 0.0f, 1000.0f, 1000.0f});
    graphics::append_rounded_rect(list, {0.0f, 0.0f, 100.0f, 30.0f}, classic.radius, {1.0f, 1.0f, 1.0f, 1.0f});
    assert(stats(list).vertices == 4);

    // with_rounding 只改几何，不动焦点时机。
    assert(classic.focus_policy == ui::FocusPolicy::Keyboard);
}

// 几何（圆角/直角）× 时机（Keyboard/Always/Never）的自由组合矩阵。
void test_focus_policy_matrix()
{
    using ui::FocusPolicy;
    using ui::InputMode;

    for (bool rounded : {true, false}) {
        for (auto policy : {FocusPolicy::Keyboard, FocusPolicy::Always, FocusPolicy::Never}) {
            auto root = std::make_unique<ui::UIRoot>(0.0f, 0.0f, 400.0f, 300.0f);
            auto button = std::make_unique<ui::Button>("ok");
            auto* button_ptr = button.get();
            root->add(std::move(button));

            ui::UISystem system{std::move(root)};
            system.tick();
            system.set_theme(ui::Theme::dark()
                                 .with_rounding(rounded)
                                 .with_focus_policy(policy));
            system.set_focus(button_ptr);

            // 鼠标模式：只有 Always 显示。
            system.context()->input_mode = InputMode::Mouse;
            assert(button_ptr->focus_visible() == (policy == FocusPolicy::Always));

            // 键盘模式：Keyboard 与 Always 显示，Never 不显示。
            system.context()->input_mode = InputMode::Keyboard;
            const bool expected = policy != FocusPolicy::Never;
            assert(button_ptr->focus_visible() == expected);

            // 几何只由 focus_style 决定，与 policy 无关。
            graphics::DrawList list;
            list.set_full_clip({0.0f, 0.0f, 1000.0f, 1000.0f});
            const glm::vec4 bounds{10.0f, 10.0f, 100.0f, 30.0f};
            button_ptr->append_focus_ring(list, bounds, rounded ? 6.0f : 0.0f);
            if (!expected) {
                assert(stats(list).vertices == 0);
            } else if (rounded) {
                // 外扩圆角环：顶点多于贴边四框，且会超出控件矩形（offset）。
                assert(stats(list).vertices > 16);
            } else {
                assert(stats(list).vertices == 16);
                assert(stats(list).indices == 24);
                assert_inside(list, bounds, 1.45f);
            }
        }
    }

    // 链式顺序：with_rounding 不改 policy，后写的 with_focus_policy 生效。
    const ui::Theme classic_keyboard = ui::Theme::dark()
                                           .with_rounding(false)
                                           .with_focus_policy(FocusPolicy::Keyboard);
    assert(classic_keyboard.focus_style == ui::FocusStyle::Border);
    assert(classic_keyboard.focus_policy == FocusPolicy::Keyboard);

    const ui::Theme rounded_never = ui::Theme::dark()
                                        .with_focus_policy(FocusPolicy::Never)
                                        .with_rounding(true);
    assert(rounded_never.focus_style == ui::FocusStyle::Outline);
    assert(rounded_never.focus_policy == FocusPolicy::Never);
}

void test_focus_visibility()
{
    using ui::InputMode;

    auto root = std::make_unique<ui::UIRoot>(0.0f, 0.0f, 400.0f, 300.0f);
    auto button = std::make_unique<ui::Button>("ok");
    auto* button_ptr = button.get();
    root->add(std::move(button));

    ui::UISystem system{std::move(root)};
    system.tick();

    // 未聚焦：不显示焦点环。
    assert(!button_ptr->focus_visible());

    system.set_focus(button_ptr);
    assert(button_ptr->is_focused());
    // 默认鼠标模式：聚焦也不显示焦点环。
    assert(!button_ptr->focus_visible());

    // 键盘事件把输入模式切到 Keyboard，焦点环可见。
    event::InputEvent key;
    key.type = event::InputEvent::Type::KeyDown;
    key.key = event::KeyCode::A;
    system.dispatch(key);
    assert(system.context()->input_mode == InputMode::Keyboard);
    assert(button_ptr->focus_visible());

    // 焦点环几何只在可见时产生顶点。
    graphics::DrawList ring_list;
    ring_list.set_full_clip({0.0f, 0.0f, 1000.0f, 1000.0f});
    button_ptr->append_focus_ring(ring_list, {0.0f, 0.0f, 100.0f, 30.0f}, 6.0f);
    assert(stats(ring_list).vertices > 0);

    // 经典样式下焦点框退化为贴控件内边缘的四条实心框（16 顶点），且不外扩。
    const glm::vec4 control_bounds{10.0f, 10.0f, 100.0f, 30.0f};
    system.set_theme(ui::Theme::dark().with_rounding(false).with_focus_policy(ui::FocusPolicy::Always));
    graphics::DrawList border_list;
    border_list.set_full_clip({0.0f, 0.0f, 1000.0f, 1000.0f});
    button_ptr->append_focus_ring(border_list, control_bounds, 0.0f);
    assert(stats(border_list).vertices == 16);
    assert(stats(border_list).indices == 24);
    assert_inside(border_list, control_bounds, 1.45f);

    // Always：鼠标点击聚焦也显示边框，不要求先切到键盘模式。
    const auto button_bounds = button_ptr->bounds();
    event::InputEvent mouse_down;
    mouse_down.type = event::InputEvent::Type::MouseButtonDown;
    mouse_down.mouse_pos = {button_bounds.pos.x + button_bounds.size.x * 0.5f,
                            button_bounds.pos.y + button_bounds.size.y * 0.5f};
    system.dispatch(mouse_down);
    assert(system.context()->input_mode == InputMode::Mouse);
    assert(button_ptr->is_focused());
    assert(button_ptr->focus_visible());

    graphics::DrawList clicked_list;
    clicked_list.set_full_clip({0.0f, 0.0f, 1000.0f, 1000.0f});
    button_ptr->append_focus_ring(clicked_list, control_bounds, 0.0f);
    assert(stats(clicked_list).vertices == 16);

    system.set_theme(ui::Theme::dark());

    // 鼠标移动切回 Mouse，焦点环隐藏。
    event::InputEvent move;
    move.type = event::InputEvent::Type::MouseMove;
    move.mouse_pos = {5.0f, 5.0f};
    system.dispatch(move);
    assert(system.context()->input_mode == InputMode::Mouse);
    assert(!button_ptr->focus_visible());

    graphics::DrawList hidden_list;
    hidden_list.set_full_clip({0.0f, 0.0f, 1000.0f, 1000.0f});
    button_ptr->append_focus_ring(hidden_list, {0.0f, 0.0f, 100.0f, 30.0f}, 6.0f);
    assert(stats(hidden_list).vertices == 0);

    // 失焦后即使键盘模式也不显示。
    system.context()->input_mode = InputMode::Keyboard;
    system.set_focus(nullptr);
    assert(!button_ptr->is_focused());
    assert(!button_ptr->focus_visible());
}

void test_widget_migration()
{
    using ui::ControlState;

    const ui::Theme dark = ui::Theme::dark();

    // 迁移后的控件仍按主题取色：禁用态文字降级、强调色来自 accent。
    ui::Button disabled_button("off");
    disabled_button.set_enabled(false);
    assert(disabled_button.is_enabled() == false);

    ui::Checkbox checkbox("check", true);
    assert(checkbox.checked());

    ui::Radio radio("radio", true);
    assert(radio.selected());

    ui::Slider slider(160.0f);
    slider.set_value(0.5f);
    assert(near(slider.value(), 0.5f));

    ui::Progress progress(120.0f);
    progress.set_value(0.25f);
    assert(near(progress.value(), 0.25f));

    // 圆角样式在控件绘制路径上被使用：DrawList 有几何产出。
    graphics::DrawList list;
    list.set_full_clip({0.0f, 0.0f, 1000.0f, 1000.0f});
    disabled_button.arrange({{0.0f, 0.0f}, {120.0f, 30.0f}});
    disabled_button.append_to(list);
    assert(stats(list).vertices > 0);

    assert(dark.style(ui::StyleRole::Accent, ControlState::Normal).fill == dark.accent);
}

// 经典样式下所有「圆形/胶囊」控件必须回到直角：slider 轨道与手柄、progress、radio。
void test_classic_shapes()
{
    auto draw = [](ui::Widget& widget) {
        graphics::DrawList list;
        list.set_full_clip({0.0f, 0.0f, 1000.0f, 1000.0f});
        widget.append_to(list);
        return stats(list);
    };

    ui::Slider slider(200.0f);
    slider.set_value(0.5f);
    slider.arrange({{0.0f, 0.0f}, {200.0f, 30.0f}});
    ui::Progress progress(200.0f);
    progress.set_value(0.5f);
    progress.arrange({{0.0f, 0.0f}, {200.0f, 10.0f}});

    ui::Radio radio("radio", true);
    radio.arrange({{0.0f, 0.0f}, {120.0f, 24.0f}});

    // UI-42：主题不能全局改了，控件要挂到一个 context 上（全局 set_theme 已移除）。
    ui::UIContext ctx;
    ctx.set_theme(ui::Theme::dark().with_rounding(false));
    slider.set_context(&ctx);
    progress.set_context(&ctx);
    radio.set_context(&ctx);

    // 经典：全部退化到矩形。slider = 轨道 + 填充 + 手柄 = 3 块矩形。
    assert(draw(slider).vertices == 12);
    // progress = 轨道 + 填充 + 贴边环（四条矩形）。
    assert(draw(progress).vertices == 24);
    // radio = 方框 + 贴边环 + 菱形两个三角形（每个 3 顶点）。
    assert(draw(radio).vertices == 4 + 16 + 6);

    // 圆角：轨道变胶囊、手柄变圆、progress 与 radio 也带圆弧（顶点数上升）。
    ctx.set_theme(ui::Theme::dark());
    assert(draw(slider).vertices > 12);
    assert(draw(progress).vertices > 24);
    assert(draw(radio).vertices > 4 + 16 + 6);
}

// Checkbox 的勾选块必须像素对齐且与外框同心：
// 半像素 inset 会让它在屏幕上看去偏左/偏上。
void test_checkbox_pixel_alignment()
{
    ui::UIContext ctx;
    ctx.set_theme(ui::Theme::dark()
                      .with_rounding(false)
                      .with_focus_policy(ui::FocusPolicy::Never));

    ui::Checkbox checkbox("x", true);
    checkbox.set_context(&ctx);
    checkbox.arrange({{0.0f, 0.0f}, {200.0f, 24.0f}});

    graphics::DrawList list;
    list.set_full_clip({0.0f, 0.0f, 1000.0f, 1000.0f});
    checkbox.append_to(list);

    // 顶点顺序：背景 4 → 边框环 16 → 勾选块 4（经典样式无圆角弧度）。
    std::vector<glm::vec2> points;
    for (const auto& batch : list.batches()) {
        for (const auto& vertex : batch.vertices) points.push_back(vertex.pos);
    }
    assert(points.size() == 24);

    // 所有顶点落在整像素上。
    for (const auto& point : points) {
        assert(point.x == std::floor(point.x));
        assert(point.y == std::floor(point.y));
    }

    // 外框矩形：points[0]=(x,y)、points[2]=(x+w,y+h)。
    const float box_cx = (points[0].x + points[2].x) * 0.5f;
    const float box_cy = (points[0].y + points[2].y) * 0.5f;

    float mx0 = points[20].x, mx1 = points[20].x;
    float my0 = points[20].y, my1 = points[20].y;
    for (std::size_t i = 20; i < 24; ++i) {
        mx0 = std::min(mx0, points[i].x);
        mx1 = std::max(mx1, points[i].x);
        my0 = std::min(my0, points[i].y);
        my1 = std::max(my1, points[i].y);
    }

    // 同心。
    assert(std::fabs((mx0 + mx1) * 0.5f - box_cx) < 1e-4f);
    assert(std::fabs((my0 + my1) * 0.5f - box_cy) < 1e-4f);
    // 四边留白相等。
    assert(std::fabs((mx0 - points[0].x) - (points[2].x - mx1)) < 1e-4f);
    assert(std::fabs((my0 - points[0].y) - (points[2].y - my1)) < 1e-4f);

    // 位置为半像素时也对齐到整像素（round），且仍保持同心。
    checkbox.arrange({{0.5f, 0.5f}, {200.0f, 25.0f}});
    graphics::DrawList half_list;
    half_list.set_full_clip({0.0f, 0.0f, 1000.0f, 1000.0f});
    checkbox.append_to(half_list);
    for (const auto& batch : half_list.batches()) {
        for (const auto& vertex : batch.vertices) {
            assert(vertex.pos.x == std::floor(vertex.pos.x));
            assert(vertex.pos.y == std::floor(vertex.pos.y));
        }
    }
}

// 主题级圆角缩放：控件默认从 Theme 继承，per-widget override 只是例外。
void test_radius_scale()
{
    const ui::Theme base = ui::Theme::dark();

    // 0x：全部直角，连 pill（圆形/胶囊）也归零。
    const ui::Theme square = base.with_radius_scale(0.0f);
    assert(square.radius == 0.0f);
    assert(square.radius_small == 0.0f);
    assert(square.radius_medium == 0.0f);
    assert(square.radius_panel == 0.0f);
    assert(square.radius_pill == 0.0f);

    // 2x：几何档位加倍，pill 仍保持「足够大」的语义值。
    const ui::Theme doubled = base.with_radius_scale(2.0f);
    assert(near(doubled.radius, base.radius * 2.0f));
    assert(near(doubled.radius_panel, base.radius_panel * 2.0f));
    assert(doubled.radius_pill > 100.0f);

    // 与密度组合：两个变换都作用在圆角上，先密度后缩放。
    const ui::Theme compact = base.with_density(ui::Density::Compact).with_radius_scale(0.5f);
    assert(near(compact.radius, base.radius * 0.9f * 0.5f));

    // 负值归零。
    assert(base.with_radius_scale(-1.0f).radius == 0.0f);
}

// WCAG 2.x 相对对比度：linear 值 → 相对亮度 → (L1+0.05)/(L2+0.05)。
// 主题存的是线性值，正好直接当相对亮度的输入用（不用再解 sRGB 编码）。
float relative_luminance(glm::vec4 linear_rgb)
{
    return 0.2126f * linear_rgb.r + 0.7152f * linear_rgb.g + 0.0722f * linear_rgb.b;
}

float contrast_ratio(glm::vec4 a, glm::vec4 b)
{
    const float la = relative_luminance(a);
    const float lb = relative_luminance(b);
    const float hi = std::max(la, lb);
    const float lo = std::min(la, lb);
    return (hi + 0.05f) / (lo + 0.05f);
}

// 状态语义完整性（UI-43）：每个角色的每个状态都得有明确定义，不靠 default 掉到“无填充/正常色”。
void test_state_coverage()
{
    using ui::ControlState;
    using ui::StyleRole;

    for (const ui::Theme& theme : {ui::Theme::dark(), ui::Theme::light()}) {
        // Row：Disabled 必须与 Normal 不同（不能“禁用看起来还能点”）。
        const auto row_normal = theme.style(StyleRole::Row, ControlState::Normal);
        const auto row_disabled = theme.style(StyleRole::Row, ControlState::Disabled);
        assert(row_disabled.fill != row_normal.fill);
        assert(row_disabled.text == theme.text_disabled);

        // MenuItem：Disabled 文本变灰（填充可以为透明，但不能“完全同外观”）。
        const auto menu_normal = theme.style(StyleRole::MenuItem, ControlState::Normal);
        const auto menu_disabled = theme.style(StyleRole::MenuItem, ControlState::Disabled);
        assert(menu_disabled.text == theme.text_disabled);
        assert(menu_disabled.text != menu_normal.text);

        // RowSelected：选中行的填充与 Row 的每种状态都不相同。
        const auto selected = theme.style(StyleRole::RowSelected, ControlState::Normal);
        assert(selected.fill != row_normal.fill);
        assert(selected.fill != row_disabled.fill);
        assert(selected.fill != theme.style(StyleRole::Row, ControlState::Hovered).fill);

        // 选中行文字保持正文色（不是禁用色）。
        assert(selected.text == theme.text);
    }

    // RowSelected 是 accent 的半透明版（换主题时跟着变）。
    const auto dark_sel = ui::Theme::dark().style(StyleRole::RowSelected, ControlState::Normal);
    const auto light_sel = ui::Theme::light().style(StyleRole::RowSelected, ControlState::Normal);
    assert(dark_sel.fill != light_sel.fill);
    assert(std::fabs(dark_sel.fill.a - 0.28f) < 1e-3f);
}

// 滚动条滑块可见性（UI-46）：
// 回归点：滑块曾经借 Track 的 Hovered 状态来区分（同一个过渡槽），每帧被槽的
// Normal 请求覆盖，两者最终都是“中间色”→ 滑块在视觉上消失（尤其浅色主题）。
// 这里锁三件事：Thumb 是独立角色、与槽有可见对比、hover/press 有可区分反馈。
void test_scrollbar_thumb_visibility()
{
    using ui::ControlState;
    using ui::StyleRole;

    for (const ui::Theme& theme : {ui::Theme::dark(), ui::Theme::light()}) {
        const auto track = theme.style(StyleRole::Track, ControlState::Normal).fill;
        const auto thumb = theme.style(StyleRole::Thumb, ControlState::Normal).fill;
        const auto hover = theme.style(StyleRole::Thumb, ControlState::Hovered).fill;
        const auto pressed = theme.style(StyleRole::Thumb, ControlState::Pressed).fill;

        // 不透明：半透明在槽上会被合成成槽色的一半。
        assert(thumb.a == 1.0f);
        assert(track.a == 1.0f);

        // 与槽的对比度 >= 2.2:1（非文本元素不用 WCAG 4.5，但旧实现只有 1.3:1，
        // 正是“滑块消失”的量级；实测 dark 2.31 / light 2.42）。
        assert(contrast_ratio(thumb, track) >= 2.2f);

        // 三态互不相同：hover 提亮/加深一档，拖动回到 accent。
        assert(hover != thumb);
        assert(contrast_ratio(hover, track) >= 2.2f);
        assert(pressed == theme.accent);
    }

    // 深浅主题各用各的滑块色（浅色下比槽暗，深色下比槽亮）。
    assert(ui::Theme::dark().style(StyleRole::Thumb, ControlState::Normal).fill !=
           ui::Theme::light().style(StyleRole::Thumb, ControlState::Normal).fill);

    // 独立槽位（回归锁）：同一控件先请求 Track/Normal 再请求 Thumb/Normal，
    // 两次结果不能相等，否则说明又退回“共用一个槽”的老路。
    ui::Progress host;
    const auto& t = ui::theme(); // 无 context 的裸控件回退只读默认主题
    const auto styled_track = host.visual_style(StyleRole::Track, ControlState::Normal);
    const auto styled_thumb = host.visual_style(StyleRole::Thumb, ControlState::Normal);
    assert(styled_track.fill == t.style(StyleRole::Track, ControlState::Normal).fill);
    assert(styled_thumb.fill == t.style(StyleRole::Thumb, ControlState::Normal).fill);
    assert(styled_track.fill != styled_thumb.fill);
}

void test_text_size_roles()
{
    const ui::Theme dark = ui::Theme::dark();

    // Body 角色：正文档。
    assert(near(dark.style(ui::StyleRole::Control, ui::ControlState::Normal).text_size,
                dark.text_size));
    assert(dark.style(ui::StyleRole::Control, ui::ControlState::Normal).text_size_role
           == ui::TextSize::Body);

    // Tooltip 角色：Small 档（以前控件只能直读 small_text_size）。
    const auto tooltip = dark.style(ui::StyleRole::Tooltip, ui::ControlState::Normal);
    assert(tooltip.text_size_role == ui::TextSize::Small);
    assert(near(tooltip.text_size, dark.small_text_size));
    assert(tooltip.text_size < dark.text_size);

    // text_size_of() 与角色换算同源。
    assert(near(dark.text_size_of(ui::TextSize::Small), dark.small_text_size));
    assert(near(dark.text_size_of(ui::TextSize::Body), dark.text_size));
    assert(near(dark.text_size_of(ui::TextSize::Heading), dark.heading_text_size));

    // 主题字号变化时，角色输出跟着变（不是硬编码常量）。
    ui::Theme bigger = dark;
    bigger.text_size = 20.0f;
    bigger.small_text_size = 14.0f;
    assert(near(bigger.style(ui::StyleRole::Control, ui::ControlState::Normal).text_size, 20.0f));
    assert(near(bigger.style(ui::StyleRole::Tooltip, ui::ControlState::Normal).text_size, 14.0f));

    // 密度档位也会缩放字号（with_density 已覆盖字号）。
    const ui::Theme compact = dark.with_density(ui::Density::Compact);
    assert(compact.small_text_size < dark.small_text_size);
}

// accent 上的文字必须读得清（UI-39）。
// 以前 Accent 角色硬编码 0x0F1419：浅色主题（accent=#1F6FD0）只有 ~3.7:1，不达 AA。
void test_accent_on_contrast()
{
    constexpr float kAaBody = 4.5f;

    for (const ui::Theme& theme : {ui::Theme::dark(), ui::Theme::light()}) {
        const float ratio = contrast_ratio(theme.accent, theme.accent_on);
        assert(ratio >= kAaBody);
        // 角色输出必须与令牌一致（Accent 的 text 就是 accent_on）。
        const auto accent_style = theme.style(ui::StyleRole::Accent, ui::ControlState::Normal);
        assert(accent_style.text == theme.accent_on);
    }

    // 正文：text 在 panel_bg 上也要达到 AA。
    for (const ui::Theme& theme : {ui::Theme::dark(), ui::Theme::light()}) {
        assert(contrast_ratio(theme.text, theme.panel_bg) >= kAaBody);
        assert(contrast_ratio(theme.text, theme.window_bg) >= kAaBody);
    }
}

} // namespace

int main()
{
    test_control_state();
    test_style_roles();
    test_density();
    test_rounding_switch();
    test_rounded_rect_geometry();
    test_rounded_ring_geometry();
    test_focus_visibility();
    test_focus_policy_matrix();
    test_classic_shapes();
    test_checkbox_pixel_alignment();
    test_radius_scale();
    test_widget_migration();
    test_accent_on_contrast();
    test_text_size_roles();
    test_state_coverage();
    test_scrollbar_thumb_visibility();
    std::cout << "ui style tests passed\n";
    return 0;
}
