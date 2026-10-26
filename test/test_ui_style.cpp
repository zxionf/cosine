#include "graphics/box.hpp"
#include "ui/button.hpp"
#include "ui/checkbox.hpp"
#include "ui/progress.hpp"
#include "ui/radio.hpp"
#include "ui/root.hpp"
#include "ui/slider.hpp"
#include "ui/style.hpp"
#include "ui/theme.hpp"
#include "ui/ui_system.hpp"

#include <cassert>
#include <cmath>
#include <iostream>
#include <memory>

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
void assert_inside(const graphics::DrawList& list, glm::vec4 rect)
{
    for (const auto& batch : list.batches()) {
        for (const auto& vertex : batch.vertices) {
            assert(vertex.pos.x >= rect.x - 0.01f);
            assert(vertex.pos.y >= rect.y - 0.01f);
            assert(vertex.pos.x <= rect.x + rect.z + 0.01f);
            assert(vertex.pos.y <= rect.y + rect.w + 0.01f);
        }
        for (uint32_t index : batch.indices) {
            assert(index < batch.vertices.size());
        }
    }
}

void test_control_state()
{
    using ui::ControlState;
    using ui::control_state;

    assert(control_state(true, false, false) == ControlState::Normal);
    assert(control_state(true, true, false) == ControlState::Hovered);
    assert(control_state(true, true, true) == ControlState::Pressed);
    // 优先级：Disabled > Pressed > Hovered > Normal。
    assert(control_state(false, true, true) == ControlState::Disabled);
    assert(control_state(true, false, true) == ControlState::Pressed);
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

    // Row / MenuItem：默认透明，hover/pressed 用强调色半透明叠加。
    assert(dark.style(StyleRole::Row, ControlState::Normal).fill.a == 0.0f);
    assert(near(dark.style(StyleRole::Row, ControlState::Hovered).fill.a, 0.12f));
    assert(near(dark.style(StyleRole::Row, ControlState::Pressed).fill.a, 0.20f));
    assert(near(dark.style(StyleRole::MenuItem, ControlState::Hovered).fill.a, 0.28f));
    assert(near(dark.style(StyleRole::MenuItem, ControlState::Pressed).fill.a, 0.40f));

    // Tooltip：浮层底色 + 中档圆角。
    const auto tooltip = dark.style(StyleRole::Tooltip, ControlState::Normal);
    assert(tooltip.fill == dark.tooltip_bg);
    assert(near(tooltip.radius, dark.radius_medium));

    // 焦点环颜色统一取 accent。
    assert(dark.style(StyleRole::Control, ControlState::Normal).focus_ring == dark.accent);
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
        assert_inside(list, bounds);
    }

    // 圆角路径：顶点增多、索引合法、全部落在矩形内。
    {
        graphics::DrawList list;
        list.set_full_clip({0.0f, 0.0f, 1000.0f, 1000.0f});
        graphics::append_rounded_rect(list, bounds, 6.0f, {1.0f, 1.0f, 1.0f, 1.0f});
        const auto g = stats(list);
        assert(g.vertices > 4);
        assert(g.indices % 3 == 0);
        assert_inside(list, bounds);
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
        assert_inside(b, bounds);
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
        assert_inside(list, bounds);
    }

    // 圆角环：顶点更多，厚度不为 0，全部落在矩形内。
    {
        graphics::DrawList list;
        list.set_full_clip({0.0f, 0.0f, 1000.0f, 1000.0f});
        graphics::append_rounded_ring(list, bounds, 8.0f, 2.0f, {1.0f, 1.0f, 1.0f, 1.0f});
        const auto g = stats(list);
        assert(g.vertices > 16);
        assert(g.indices % 3 == 0);
        assert_inside(list, bounds);
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
        assert_inside(list, bounds);
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
    assert(classic.radius_large == 0.0f);
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
                assert_inside(list, bounds);
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
    assert_inside(border_list, control_bounds);

    // Always：鼠标点击聚焦也显示边框，不要求先切到键盘模式。
    const auto button_bounds = button_ptr->global_bounds();
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
    assert(disabled_button.enabled() == false);

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

    // 经典：全部退化到矩形。slider = 轨道 + 填充 + 手柄 = 3 块矩形。
    ui::set_theme(ui::Theme::dark().with_rounding(false));
    assert(draw(slider).vertices == 12);
    // progress = 轨道 + 填充 + 贴边环（四条矩形）。
    assert(draw(progress).vertices == 24);
    // radio = 方框 + 贴边环 + 菱形两个三角形（每个 3 顶点）。
    assert(draw(radio).vertices == 4 + 16 + 6);

    // 圆角：轨道变胶囊、手柄变圆、progress 与 radio 也带圆弧（顶点数上升）。
    ui::set_theme(ui::Theme::dark());
    assert(draw(slider).vertices > 12);
    assert(draw(progress).vertices > 24);
    assert(draw(radio).vertices > 4 + 16 + 6);

    // 恢复默认，避免影响同进程内的其它用例。
    ui::set_theme(ui::Theme::dark());
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
    test_widget_migration();
    std::cout << "ui style tests passed\n";
    return 0;
}
