#pragma once

#include "vbox.hpp"
#include "root.hpp"
#include "style.hpp"
#include "../graphics/text_debug.hpp"
#include "theme.hpp"
#include "widget.hpp"
#include "../graphics/box.hpp"
#include "../graphics/line.hpp"
#include "../graphics/rectangle.hpp"

#include <algorithm>
#include <cmath>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace xel::ui
{
// 应用内窗口（UI-6）：标题栏拖动、右下/右/下 resize、折叠、两种关闭。
//
// 挂载：放在 UIRoot 下并用 set_absolute 定位 —— 这样窗口可以拖到任何区域、
// 靠 z-index 叠放，不受面板裁剪。构造时已自动 set_absolute(true)。
//
// 拖动范围：允许拖出父区域，但至少保留 48px 可见；标题栏不能被拖出顶部。
class Window : public Widget
{
public:
    Window(std::string title, glm::vec2 pos, glm::vec2 size)
        : title_(std::move(title)), children_()
    {
        set_size(size);
        set_min_size({160.0f, kTitleHeight + 48.0f});
        set_max_size({1.0e6f, 1.0e6f});
        // 两个坐标系必须同时初始化：UIRoot 的自由画布读 local_.pos，Vbox 读 absolute_pos。
        set_pos(pos);
        set_absolute_pos(pos);

        auto body = std::make_unique<Vbox>(0.0f, 0.0f, size.x, size.y - kTitleHeight);
        body_ = body.get();
        children_.push_back(std::move(body));
        // 必须手动设 parent：冒泡链是 Section → body_ → Window，断了就只能在标题栏拖。
        body_->set_parent(this);
    }

    void set_title(std::string title)
    {
        title_ = std::move(title);
        mark_layout_dirty();
    }
    const std::string& title() const { return title_; }

    // 内容区容器：外部往窗口里塞一整块界面时用（资源浏览器等）。
    Vbox* content() const { return body_; }

    // 无标题栏窗口：整块区域都是拖动带（内容里的控件仍然优先接收点击）。
    void set_title_bar_visible(bool visible)
    {
        title_bar_visible_ = visible;
        mark_layout_dirty();
    }
    bool title_bar_visible() const { return title_bar_visible_; }

    // 始终置顶：raise() 之后强制最高 z（全局参数面板这类窗口用）。
    void set_always_on_top(bool on)
    {
        always_on_top_ = on;
        if (on) set_z_index(kOnTopZ);
    }
    bool always_on_top() const { return always_on_top_; }

    // 模态：窗口外被全屏遮罩挡住（点不动），Esc / 关闭才解除。
    void set_modal(bool on)
    {
        if (modal_ == on) return;
        modal_ = on;
        if (on) {
            set_always_on_top(true);
            if (auto* root = find_root_widget()) root->push_modal(this);
        } else {
            release_modal();
        }
    }
    bool modal() const { return modal_; }

    // 不可收起的窗口：不画折叠三角，点击也不响应。
    void set_collapsible(bool on) { collapsible_ = on; }
    bool collapsible() const { return collapsible_; }

    float title_height() const { return title_bar_visible_ ? kTitleHeight : 0.0f; }

    // 字体 debug：标题文字与内容区边框都跟着画（Label、Button 各自也支持）。
    void set_text_debug(graphics::TextDebugOptions options) { debug_ = options; }
    const graphics::TextDebugOptions& text_debug() const { return debug_; }

    void set_collapsed(bool collapsed)
    {
        if (collapsed_ == collapsed) return;
        if (collapsed) {
            expanded_height_ = height();
            set_size(width(), kTitleHeight);
        } else {
            set_size(width(), std::max(expanded_height_, kTitleHeight + 48.0f));
        }
        collapsed_ = collapsed;
        mark_layout_dirty();
    }
    bool collapsed() const { return collapsed_; }

    // 两种关闭：close() 只隐藏；destroy() 交给外部销毁（未注册回调则退化为隐藏）。
    void close()
    {
        release_modal();
        set_visible(false);
    }
    void destroy()
    {
        release_modal();
        if (on_destroy_) {
            on_destroy_();
        } else {
            close();
        }
    }
    void set_on_destroy(std::function<void()> handler) { on_destroy_ = std::move(handler); }

    void set_resizable(bool resizable) { resizable_ = resizable; }
    bool resizable() const { return resizable_; }

    bool dragging() const { return mode_ == Mode::Drag; }
    bool resizing() const { return mode_ == Mode::Resize; }

    // 置顶：提到同级最大 z + 1，然后按 paint 顺序归一化 1..N。
    void raise()
    {
        Widget* p = parent();
        if (p == nullptr) return;
        int max_z = 0;
        for (const auto& sibling : p->children()) {
            if (sibling.get() == this) continue;
            max_z = std::max(max_z, sibling->z_index());
        }
        set_z_index(max_z + 1);
        int next = 1;
        for (Widget* widget : p->paint_order()) widget->set_z_index(next++);
        // 归一化会把自己的 z 也改成 1..N，置顶窗口在这里重新拉高。
        if (always_on_top_) set_z_index(kOnTopZ);
    }

    void add(std::unique_ptr<Widget> child) override
    {
        if (body_ != nullptr) body_->add(std::move(child));
    }
    const std::vector<std::unique_ptr<Widget>>& children() const override { return children_; }

    glm::vec2 measure(glm::vec2) override
    {
        // 绝对定位控件不参与父容器的测量协商：尺寸只由自己（拖动/resize）维护。
        return {width(), height()};
    }

    void arrange(Rect slot) override
    {
        set_pos(slot.pos);
        // 注意：absolute_pos_ 保持「相对父内容区」语义，这里不能写 slot.pos，
        // 否则下次 arrange 会再叠一次父偏移。两个坐标系的一致性在拖动时维护。
        const bool self_sized = absolute();
        global_ = {slot.pos, self_sized ? glm::vec2{width(), height()} : slot.size};
        if (!self_sized) set_size(slot.size);
        layout();
    }

    void layout() override
    {
        if (body_ == nullptr) return;
        const float title = title_height();
        const float body_h = collapsed_ ? 0.0f : std::max(0.0f, height() - title);
        body_->set_visible(!collapsed_);
        // 右侧与下侧留出 resize 抓取带：否则边缘点击会被内容区吞掉，resize 永远轮不到窗口。
        body_->arrange({{global_.pos.x, global_.pos.y + title},
                        {std::max(0.0f, global_.size.x - kGripEdge),
                         std::max(0.0f, body_h - kGripEdge)}});
    }

    void append_to(graphics::DrawList& list) const override
    {
        if (!visible()) return;
        const auto& t = ui_theme();
        const glm::vec4 bounds{global_.pos.x, global_.pos.y, global_.size.x, global_.size.y};
        const float radius = corner_radius(t.radius_panel);
        const bool active = mode_ != Mode::None;

        // 窗口底 + 边框：用比面板更亮的一档，否则窗口会与父面板融成一片。
        // （真正的 elevated 令牌待补，这里先复用 control 色阶。）
        graphics::append_rounded_rect(list, bounds, radius, t.panel_bg);
        // 标题栏用 button_bg：窗口体已经用 surface，否则窗口和里面的按钮一个颜色。
        graphics::append_rounded_ring(list, bounds, radius, active ? 2.0f : 1.0f,
                                      active ? t.accent : t.panel_border);

        // 标题栏（无标题栏窗口跳过）。
        const float title = title_height();
        auto* atlas = font_atlas();
        if (title > 0.0f) {
            const glm::vec4 bar{bounds.x, bounds.y, bounds.z, title};
            graphics::append_rounded_rect(list, bar, radius, t.button_hover);

            if (atlas != nullptr) {
                // 用字体行高而不是字号：行框通常比字号高，按字号算文字会整体偏下。
                const float text_y = atlas->centered_text_y(t.text_size, bar.y, title);
                atlas->append_text(list, bar.x + kGlyphZone, text_y, t.text_size, title_, t.text);
                // debug 是叠加层，不能替代字形本身。
                if (debug_.any()) {
                    graphics::append_text_debug(list, *atlas, bar.x + kGlyphZone, text_y,
                                                t.text_size, title_, debug_);
                }
            }

            // 折叠三角（左）与关闭叉（右）。
            const glm::vec4 glyph_color = t.text_muted;
            const float cy = bar.y + title * 0.5f;
            // 不可收起的窗口不画折叠三角（标题仍可拖动）。
            if (collapsible_) {
                if (collapsed_) {
                    graphics::Line{bar.x + 8.0f, cy - 4.0f, bar.x + 16.0f, cy, 1.5f, glyph_color}
                        .append_to(list);
                    graphics::Line{bar.x + 16.0f, cy, bar.x + 8.0f, cy + 4.0f, 1.5f, glyph_color}
                        .append_to(list);
                } else {
                    graphics::Line{bar.x + 8.0f, cy + 3.0f, bar.x + 16.0f, cy - 4.0f, 1.5f, glyph_color}
                        .append_to(list);
                    graphics::Line{bar.x + 16.0f, cy - 4.0f, bar.x + 24.0f, cy + 3.0f, 1.5f, glyph_color}
                        .append_to(list);
                }
            }
            const float cx = bar.x + bar.z - 16.0f;
            graphics::Line{cx - 5.0f, cy - 5.0f, cx + 5.0f, cy + 5.0f, 1.5f, glyph_color}
                .append_to(list);
            graphics::Line{cx + 5.0f, cy - 5.0f, cx - 5.0f, cy + 5.0f, 1.5f, glyph_color}
                .append_to(list);
        }

        // 内容区。
        if (!collapsed_ && body_ != nullptr) {
            list.push_clip({bounds.x, bounds.y + title_height(), bounds.z,
                            std::max(0.0f, bounds.w - title_height())});
            body_->append_to(list);
            // debug 时把内容区（Vbox）的布局框也画出来。
            if (debug_.any()) {
                const Rect& box = body_->global_bounds();
                const glm::vec4 c = debug_.color;
                const float th = debug_.thickness;
                graphics::Rectangle{box.pos.x, box.pos.y, box.size.x, th, c}.append_to(list);
                graphics::Rectangle{box.pos.x, box.pos.y + box.size.y - th, box.size.x, th, c}
                    .append_to(list);
                graphics::Rectangle{box.pos.x, box.pos.y, th, box.size.y, c}.append_to(list);
                graphics::Rectangle{box.pos.x + box.size.x - th, box.pos.y, th, box.size.y, c}
                    .append_to(list);
            }
            list.pop_clip();
        }
    }

    bool on_key_down(event::KeyCode key, event::Modifier) override
    {
        if (key != event::KeyCode::Escape) return false;
        close();
        return true;
    }

    bool on_mouse_down(glm::vec2 pos, int button) override
    {
        if (button != 0 || !visible()) return false;
        raise();

        if (hit_close(pos)) {
            destroy();
            return true;
        }
        if (hit_collapse(pos)) {
            set_collapsed(!collapsed_);
            return true;
        }
        if (resizable_ && !collapsed_) {
            const int edges = resize_edges(pos);
            if (edges != 0) {
                mode_ = Mode::Resize;
                edges_ = edges;
                // 用 absolute_pos + size 记偏移：global_ 要等父容器重排才刷新。
                grab_ = pos - (absolute_pos() + glm::vec2{width(), height()});
                return true;
            }
        }
        if (title_bar_visible_ && hit_title(pos)) {
            mode_ = Mode::Drag;
            grab_ = pos - absolute_pos();
            return true;
        }
        if (!title_bar_visible_ && global_bounds().contains(pos)) {
            // 无标题栏：拖动本体（内容里的控件会先接收点击，到不了这里）。
            mode_ = Mode::Drag;
            grab_ = pos - absolute_pos();
            return true;
        }
        return false;
    }

    bool on_mouse_move(glm::vec2 pos) override
    {
        if (mode_ == Mode::None) return false;

        if (mode_ == Mode::Drag) {
            const glm::vec2 next = clamp_position(pos - grab_);
            set_absolute_pos(next);
            // UIRoot 是自由画布：每帧用 child.x()/y() 重定位，所以 local_.pos 也必须更新，
            // 否则拖动会被下一帧 layout 撤销。Vbox 下则用 absolute_pos，两者同时保持。
            set_pos(next);
            global_.pos = next;
        } else {
            const glm::vec2 want = pos - grab_ - absolute_pos();
            glm::vec2 next{width(), height()};
            if ((edges_ & kEdgeRight) != 0) next.x = want.x;
            if ((edges_ & kEdgeBottom) != 0) next.y = want.y;
            set_size(std::max(next.x, 160.0f),
                     std::max(next.y, collapsed_ ? title_height() : title_height() + 48.0f));
            global_.size = {width(), height()};
        }

        layout(); // body_ 跟着新的 global_ 重新排
        mark_layout_dirty();
        if (Widget* p = parent()) p->mark_layout_dirty();
        return true;
    }

    bool on_mouse_up(glm::vec2, int) override
    {
        if (mode_ == Mode::None) return false;
        mode_ = Mode::None;
        return true;
    }

private:
    enum class Mode
    {
        None,
        Drag,
        Resize,
    };

    static constexpr float kTitleHeight = 32.0f;
    static constexpr float kGlyphZone = 32.0f; // 折叠三角占位，标题文字从这里开始
    static constexpr float kKeepVisible = 48.0f;
    UIRoot* find_root_widget() const
    {
        Widget* node = const_cast<Window*>(this);
        while (node != nullptr) {
            if (auto* root = dynamic_cast<UIRoot*>(node)) return root;
            node = node->parent();
        }
        return nullptr;
    }

    void release_modal()
    {
        if (!modal_) return;
        modal_ = false;
        if (auto* root = find_root_widget()) root->pop_modal(this);
    }

    static constexpr float kGripSize = 12.0f;
    static constexpr float kGripEdge = 6.0f;
    static constexpr int kOnTopZ = 10000;
    static constexpr int kEdgeRight = 1;
    static constexpr int kEdgeBottom = 2;

    Rect close_rect() const
    {
        return {{global_.pos.x + global_.size.x - 28.0f, global_.pos.y + 4.0f}, {24.0f, 24.0f}};
    }
    Rect collapse_rect() const
    {
        return {{global_.pos.x + 4.0f, global_.pos.y + 4.0f}, {24.0f, 24.0f}};
    }
    Rect title_rect() const
    {
        return {{global_.pos.x, global_.pos.y}, {global_.size.x, title_height()}};
    }

    bool hit_close(glm::vec2 pos) const
    {
        if (collapsed_ || !title_bar_visible_) return false;
        return close_rect().contains(pos);
    }
    bool hit_collapse(glm::vec2 pos) const
    {
        if (!title_bar_visible_ || !collapsible_) return false;
        return collapse_rect().contains(pos);
    }
    bool hit_title(glm::vec2 pos) const { return title_rect().contains(pos); }

    // 右下角优先，然后右边、下边。
    int resize_edges(glm::vec2 pos) const
    {
        const float right = global_.pos.x + global_.size.x;
        const float bottom = global_.pos.y + global_.size.y;
        const bool in_corner = pos.x >= right - kGripSize && pos.y >= bottom - kGripSize;
        if (in_corner) return kEdgeRight | kEdgeBottom;

        const bool below_title = pos.y >= global_.pos.y + title_height();
        if (pos.x >= right - 6.0f && below_title) return kEdgeRight;
        if (pos.y >= bottom - 6.0f && pos.x >= global_.pos.x + kGripSize) return kEdgeBottom;
        return 0;
    }

    glm::vec2 clamp_position(glm::vec2 pos) const
    {
        const glm::vec2 area = parent() != nullptr ? glm::vec2{parent()->width(), parent()->height()}
                                                   : glm::vec2{1280.0f, 800.0f};
        const float max_x = std::max(0.0f, area.x - kKeepVisible);
        const float min_x = kKeepVisible - std::max(kKeepVisible, global_.size.x);
        const float max_y = std::max(0.0f, area.y - (title_height() > 0.0f ? title_height() : kKeepVisible));
        return {std::clamp(pos.x, min_x, max_x), std::clamp(pos.y, 0.0f, max_y)};
    }

    std::string title_;
    std::vector<std::unique_ptr<Widget>> children_;
    Vbox* body_ = nullptr;
    Mode mode_ = Mode::None;
    int edges_ = 0;
    glm::vec2 grab_{0.0f, 0.0f};
    bool collapsed_ = false;
    bool resizable_ = true;
    bool title_bar_visible_ = true;
    bool always_on_top_ = false;
    bool modal_ = false;
    bool collapsible_ = true;
    graphics::TextDebugOptions debug_{};
    float expanded_height_ = 0.0f;
    std::function<void()> on_destroy_;
};
} // namespace xel::ui
