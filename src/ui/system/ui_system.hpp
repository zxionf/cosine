#pragma once

#include "ui/system/root.hpp"
#include "ui/system/ui_host.hpp"
#include "events/event.hpp"

#include <memory>
#include <vector>

namespace xel::ui
{
class UISystem
{
public:
    // 唯一构造：root 自带尺寸（无窗口的纯逻辑测试、离屏导出都用这个）。
    explicit UISystem(std::unique_ptr<UIRoot> root);
    ~UISystem();

    UIRoot* root() const { return root_.get(); }
    UIContext* context() const { return root_ ? root_->context() : nullptr; }
    void set_theme(const Theme& theme) { if (root_) root_->set_theme(theme); }
    void tick();
    // dt 单位为秒；UI 状态过渡用它推进。无参重载按 1/60s 处理，保持旧调用点不变。
    void tick(float dt);

    // 宿主注入：只提供光标回调（空则不改光标）。
    void bind_host(UiHost host) { host_ = std::move(host); }

    // 视图尺寸（framebuffer 物理像素）：由宿主在初始化与 resize 时推入。
    // 不在 dispatch 回调里立即改 root：置脏，下一 tick 统一应用（与布局时机一致）。
    void set_viewport(glm::vec2 framebuffer_size);

    // DPI：root 以逻辑像素为尺寸，物理 extent 除以该比例。默认 1.0，行为不变。
    void set_ui_scale(float scale)
    {
        const float value = scale > 0.0f ? scale : 1.0f;
        if (value != ui_scale_) {
            ui_scale_ = value;
            scale_dirty_ = true;
        }
    }
    float ui_scale() const { return ui_scale_; }

    // 当前指针位置（UI 逻辑坐标）：dispatch 时更新，供 tooltip 等每帧跟随用。
    glm::vec2 pointer_pos() const { return pointer_pos_; }
    // framebuffer 物理像素 → UI 逻辑坐标（÷ui_scale），与输入回调共用同一换算。
    glm::vec2 framebuffer_to_logical(glm::vec2 framebuffer_pos) const
    {
        return framebuffer_pos / ui_scale_;
    }

    void dispatch(const event::InputEvent& event);

    Widget* focused() const { return focused_.widget; }
    Widget* hovered() const { return hovered_.widget; }
    void set_focus(Widget* widget);
    void focus_next(bool backwards = false);

    // 指针手势阈值：0 表示关闭对应手势。
    void set_double_click_ms(float ms) { double_click_ms_ = ms; }
    void set_long_press_ms(float ms) { long_press_ms_ = ms; }
    void set_double_click_slop(float slop) { double_click_slop_ = slop; }
    float double_click_ms() const { return double_click_ms_; }
    float long_press_ms() const { return long_press_ms_; }
    float double_click_slop() const { return double_click_slop_; }

private:
    // 交互目标：同时记录指针与生命周期令牌。
    // 控件可能被容器直接销毁（clear_children / set_content），此时令牌过期，
    // 指针即使非空也不能解引用。
    struct Target
    {
        Widget* widget = nullptr;
        std::weak_ptr<const bool> token;

        void set(Widget* w)
        {
            widget = w;
            token = w ? w->alive_token() : std::weak_ptr<const bool>{};
        }
        void clear() { widget = nullptr; token.reset(); }
        bool valid() const { return widget != nullptr && !token.expired(); }

        Target& operator=(Widget* w) { set(w); return *this; }
        Target& operator=(std::nullptr_t) { clear(); return *this; }

        operator Widget*() const { return widget; }
        Widget* operator->() const { return widget; }
        explicit operator bool() const { return valid(); }
    };

    std::vector<Widget*> hit_path(Widget* root, glm::vec2 pos) const;
    void update_tree(Widget* widget, float dt);
    void update_long_press();
    void collect_focusable(Widget* widget, std::vector<Widget*>& out) const;
    // 移除 overlay / 子项后清理已失效的 hover、press、focus 目标。
    void prune_stale_targets();
    // 把焦点 getter/setter 注入 UIRoot，供模态栈快照/恢复焦点。
    void install_focus_hooks();

    void dispatch_mouse_move(const event::InputEvent& event);
    void dispatch_mouse_button(const event::InputEvent& event);
    void dispatch_mouse_scroll(const event::InputEvent& event);
    void dispatch_key(const event::InputEvent& event);
    void dispatch_char(const event::InputEvent& event);

    // resize 热区扫描：沿命中链（最深优先）找第一个自报非 Arrow 光标的控件。
    // 控件自己覆写 Node::cursor_at()，UISystem 不再认识任何具体控件类型。
    Widget* pointer_owner_at(glm::vec2 pos) const;
    void update_resize_cursor(glm::vec2 pos);

    std::unique_ptr<UIRoot> root_;

    Target hovered_;
    Target pressed_;
    Target focused_;

    // 指针手势状态：时间用 tick(dt) 累加，不依赖系统时钟，保证可确定性测试。
    static constexpr int kButtonLeft = 0;
    static constexpr int kButtonRight = 1;
    // Tab 顺序中「未显式设置」的排序键：显式 order 应远小于它。
    static constexpr int kAutoTabOrder = 1'000'000;

    struct ClickRecord
    {
        Target target;
        glm::vec2 pos{0.0f, 0.0f};
        int button = -1;
        float time = 0.0f;
        bool valid = false;
    };

    struct PressRecord
    {
        bool active = false;
        glm::vec2 pos{0.0f, 0.0f};
        int button = -1;
        float time = 0.0f;
        bool long_fired = false;
    };

    float elapsed_ = 0.0f;
    float double_click_ms_ = 400.0f;
    float long_press_ms_ = 600.0f;
    float double_click_slop_ = 6.0f;
    glm::vec2 pointer_pos_{0.0f, 0.0f};
    ClickRecord last_click_;
    PressRecord press_;

    // 视图尺寸与 DPI：viewport_ 是 framebuffer 物理像素，root 用逻辑像素。
    glm::vec2 viewport_{0.0f, 0.0f};
    bool viewport_dirty_ = false;
    float ui_scale_ = 1.0f;
    bool scale_dirty_ = false;
    UiHost host_;
};
} // namespace xel::ui
