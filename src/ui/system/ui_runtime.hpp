#pragma once

#include "ui/system/ui_host.hpp"
#include "ui/system/ui_system.hpp"

#include "graphics/display_scale.hpp"

#include <functional>
#include <memory>

namespace xel::graphics
{
class FontAtlas;
}

namespace xel::ui
{
// ── UI 运行时：UI 侧的组合根 ──────────────────────────────────────
//
// 它只做三件事，且全部与平台无关：
//   1. 拥有 root + UISystem（UI 树 / 布局 / 事件 / 焦点）
//   2. DPI 单一入口：set_display_scale() 把宿主给的 DisplayScale 转发到
//      「root 逻辑尺寸 + 渲染缩放 + 字体栅格化」
//   3. 宿主能力注入（光标 / 渲染缩放 / 字体图集），本类不 include 任何 backend 或渲染头
//
// 无窗口、无 GPU 的场景（逻辑测试、离屏导出）可以直接用：
//     UiRuntime ui;
//     ui.set_viewport({800.0f, 600.0f});
//     ui.root().add(std::make_unique<Button>("OK"));
//     ui.tick(1.0f / 60.0f);
class UiRuntime
{
public:
    // 宿主能力：留空表示该能力不可用（纯逻辑测试），而不是引入平台依赖。
    struct Hooks
    {
        std::function<void(CursorShape)> set_cursor;  // 系统光标（见 ui_host.hpp）
        std::function<void(float)> set_render_scale;  // 渲染缩放 + 字体栅格化比例
    };

    // 不给 root 时构造 0×0 空画布，尺寸随后由 set_viewport 推入。
    UiRuntime();
    explicit UiRuntime(std::unique_ptr<UIRoot> root);
    UiRuntime(const UiRuntime&) = delete;
    UiRuntime& operator=(const UiRuntime&) = delete;
    ~UiRuntime();

    // ── 宿主注入 ──────────────────────────────────────────────────
    void bind_host(Hooks hooks);
    void set_font_atlas(graphics::FontAtlas* atlas);

    // ── 视图与 DPI ────────────────────────────────────────────────
    // framebuffer 物理像素 → root 逻辑尺寸：由宿主在初始化与 resize 时推入。
    void set_viewport(glm::vec2 framebuffer_size);
    // 显示比例（dpr + ui_scale）是宿主注入的唯一事实：本类把它转发到
    // UISystem（root 尺寸）与渲染后端（投影 / 字体栅格化）。
    void set_display_scale(const graphics::DisplayScale& scale);
    // 便利入口：只改用户比例（dpr 不变）。
    void set_ui_scale(float scale) { set_display_scale(scale_.with_ui_scale(scale)); }
    const graphics::DisplayScale& display_scale() const { return scale_; }
    float ui_scale() const { return scale_.ui_scale; }
    glm::vec2 framebuffer_to_logical(glm::vec2 framebuffer_pos) const
    {
        return scale_.framebuffer_to_logical(framebuffer_pos);
    }
    glm::vec2 pointer_pos() const { return uisys_.pointer_pos(); }

    // ── 每帧 ──────────────────────────────────────────────────────
    void tick();
    void tick(float dt);

    // ── 树与调度（转发，不新增状态）─────────────────────────────────
    void dispatch(const event::InputEvent& event) { uisys_.dispatch(event); }
    UIRoot& root() { return *uisys_.root(); }
    const UIRoot& root() const { return *uisys_.root(); }
    UIContext* context() { return uisys_.context(); }
    const UIContext* context() const { return uisys_.context(); }
    void set_theme(const Theme& theme) { uisys_.set_theme(theme); }

    Widget* hovered() const { return uisys_.hovered(); }
    Widget* focused() const { return uisys_.focused(); }
    void set_focus(Widget* widget) { uisys_.set_focus(widget); }
    void focus_next(bool backwards = false) { uisys_.focus_next(backwards); }

    // 需要调度器细节（手势阈值、hit_path 等）时用；普通代码留在本类的转发接口上。
    UISystem& uisys() { return uisys_; }
    const UISystem& uisys() const { return uisys_; }

private:
    void apply_render_scale();

    UISystem uisys_;
    Hooks hooks_;
    graphics::FontAtlas* font_atlas_ = nullptr;
    graphics::DisplayScale scale_{};
};
} // namespace xel::ui
