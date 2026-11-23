#pragma once

#include "ui/containers/vbox.hpp"
#include "ui/root.hpp"
#include "ui/style.hpp"
#include "graphics/text_debug.hpp"
#include "ui/theme.hpp"
#include "ui/widget.hpp"
#include "graphics/box.hpp"
#include "graphics/line.hpp"
#include "graphics/rectangle.hpp"

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
    // 标题栏最小高度：实际值由 chrome() 按字体行高与主题 padding 派生，不低于此值。
    static constexpr float kDefaultTitleHeight = 16;

    Window(std::string title, glm::vec2 pos, glm::vec2 size)
        : title_(std::move(title))
    {
        set_size(size);
        set_min_size({160.0f, kDefaultTitleHeight + 48.0f});
        set_max_size({1.0e6f, 1.0e6f});
        // 两个坐标系必须同时初始化：UIRoot 的自由画布读 local_.pos，Vbox 读 absolute_pos。
        set_pos(pos);
        set_absolute_pos(pos);

        auto body = std::make_unique<Vbox>(0.0f, 0.0f, size.x, size.y - kDefaultTitleHeight);
        body_ = body.get();
        // 走 adopt()：这里漏设 parent 会让冒泡链断在 body_，只能拖标题栏。
        adopt(std::move(body));
    }

    void set_title(std::string title)
    {
        title_ = std::move(title);
        mark_layout_dirty();
    }
    const std::string& title() const { return title_; }

    // 内容区容器：外部往窗口里塞一整块界面时用（资源浏览器等）。
    Vbox* content() const { return body_; }

    // 底部栏（状态栏/工具栏）：全宽贴窗口底，覆盖底部 resize 抓取带。
    // 高度由 footer 自身的 measure 决定；footer 为 null 时窗口行为与原来一致。
    void set_footer(std::unique_ptr<Widget> footer)
    {
        footer_ = footer.get();
        adopt(std::move(footer));
        mark_layout_dirty();
    }
    Widget* footer() const { return footer_; }

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
        if (on) {
            set_z_index(kOnTopZ);
        } else if (z_index() == kOnTopZ) {
            // 关闭置顶时必须把 z 降回来，否则窗口仍会永远画在最上面。
            set_z_index(0);
        }
    }
    bool always_on_top() const { return always_on_top_; }

    // 模态：窗口外被全屏遮罩挡住（点不动），Esc / 关闭才解除。
    void set_modal(bool on)
    {
        if (modal_ == on) return;
        if (on) {
            modal_ = true;
            // 记录打开前的置顶 / Esc 状态，release_modal 时还原（否则会永久生效）。
            always_on_top_before_modal_ = always_on_top_;
            esc_closable_before_modal_ = esc_closable_;
            esc_closable_ = true;
            set_always_on_top(true);
            if (auto* root = find_root_widget()) root->push_modal(this);
        } else {
            // 不能先置 modal_ = false：release_modal() 靠这个标记判断要不要 pop。
            release_modal();
        }
    }
    bool modal() const { return modal_; }

    // 模态窗口与 Scrim 同层（Modal），非模态窗口按普通内容处理。
    Layer layer() const override { return modal_ ? Layer::Modal : Layer::Content; }

    // 不可收起的窗口：不画折叠三角，点击也不响应。
    void set_collapsible(bool on) { collapsible_ = on; }
    bool collapsible() const { return collapsible_; }

    // Esc 是否关闭窗口。默认 false，模态窗口在 set_modal(true) 时自动开启。
    void set_esc_closable(bool on) { esc_closable_ = on; }
    bool esc_closable() const { return esc_closable_; }

    // 窗口装饰几何：从主题与字体行高派生，绘制与命中共用同一份。
    // 标题高度跟随字体行高（行高 + 上下 padding），不再硬编码常量。
    struct Chrome
    {
        float title = 0.0f;                // 标题栏高 = 字体 line box 高度（无标题栏时为 0）
        float glyph = 0.0f;                // 标题文字左侧留白（折叠图标占位）
        float icon = 24.0f;                // 折叠/关闭图标的命中方块边长
        float inset = 4.0f;                // 图标距标题栏边缘
        float grip = 6.0f;                 // resize 抓取带宽
        float keep_visible = 48.0f;        // 拖动时保证留在视口内的最小可见量
    };

    Chrome chrome() const
    {
        const auto& t = ui_theme();
        auto* atlas = font_atlas();
        // 标题高 = 字体 line box 高度（不额外加 padding）。
        const float line = atlas != nullptr ? atlas->line_height(t.text_size) : t.text_size * 1.2f;
        Chrome c;
        c.inset = std::max(2.0f, std::round(line * 0.15f));
        c.title = line;
        // 图标方块在标题栏内垂直居中：高度 = 标题高 − 上下 inset。
        c.icon = std::max(8.0f, c.title - c.inset * 2.0f);
        c.glyph = c.icon + c.inset * 2.0f;
        c.grip = kGripEdge;
        c.keep_visible = kKeepVisible;
        return c;
    }

    float title_height() const
    {
        return title_bar_visible_ ? chrome().title : 0.0f;
    }

    // 字体 debug：标题文字与内容区边框都跟着画（Label、Button 各自也支持）。
    void set_text_debug(graphics::TextDebugOptions options) { debug_ = options; }
    const graphics::TextDebugOptions& text_debug() const { return debug_; }

    void set_collapsed(bool collapsed)
    {
        if (collapsed_ == collapsed) return;
        if (collapsed) {
            expanded_height_ = height();
            set_size(width(), title_height());
        } else {
            set_size(width(), std::max(expanded_height_, title_height() + 48.0f));
        }
        collapsed_ = collapsed;
        mark_layout_dirty();
    }
    bool collapsed() const { return collapsed_; }

    // 两种关闭：close() 只隐藏；destroy() 交给外部销毁（未注册回调则从 root 延迟移除）。
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
        } else if (auto* root = find_root_widget()) {
            // 未注册回调：从 root 的子项中延迟移除（不再退化为“隐藏但留在树上”）。
            root->remove(this);
        } else {
            set_visible(false);
        }
    }
    void set_on_destroy(std::function<void()> handler) { on_destroy_ = std::move(handler); }

    void set_resizable(bool resizable) { resizable_ = resizable; }
    bool resizable() const { return resizable_; }

    // 关闭前确认：返回 false 则拒绝关闭（Esc 与点叉都走这里）。
    void set_on_close(std::function<bool()> handler) { on_close_ = std::move(handler); }
    // 请求关闭：on_close_ 拒绝时返回 false 且不吞事件。
    // 实际 destroy() 延迟到下一次 layout()：事件派发/冒泡期间销毁自身会留下悬垂指针。
    bool request_close()
    {
        if (on_close_ && !on_close_()) return false;
        pending_destroy_ = true;
        mark_layout_dirty();
        return true;
    }

    // 最大化：铺满父容器（无父容器时用自身尺寸），再次调用则还原。
    void set_maximized(bool on)
    {
        if (maximized_ == on) return;
        maximized_ = on;
        if (on) {
            restore_pos_ = absolute_pos();
            restore_size_ = {width(), height()};
            const glm::vec2 area = parent() != nullptr
                                       ? glm::vec2{parent()->width(), parent()->height()}
                                       : restore_size_;
            set_absolute_pos({0.0f, 0.0f});
            set_pos({0.0f, 0.0f});
            set_size(area);
            global_ = {{0.0f, 0.0f}, area};
        } else {
            set_absolute_pos(restore_pos_);
            set_pos(restore_pos_);
            set_size(restore_size_);
            global_ = {restore_pos_, restore_size_};
        }
        layout();
        mark_layout_dirty();
        if (Widget* p = parent()) p->mark_layout_dirty();
    }
    bool maximized() const { return maximized_; }

    bool dragging() const { return mode_ == Mode::Drag; }
    bool resizing() const { return mode_ == Mode::Resize; }

    // 置顶：把自身移到父容器子项末尾（插入序）。不改写兄弟的 z_index。
    void raise()
    {
        if (Widget* p = parent()) p->raise_child(this);
        // 显式置顶的窗口仍需最高 z（模态帧的 z 由 UIRoot::layout 统一分配）。
        if (always_on_top_) set_z_index(kOnTopZ);
    }

    void add(std::unique_ptr<Widget> child) override
    {
        if (body_ != nullptr) body_->add(std::move(child));
    }

    glm::vec2 measure(glm::vec2) override
    {
        // 尺寸由自己（拖动/resize）维护，不参与父容器的测量协商。
        return {width(), height()};
    }
    // 与 measure 一致：忽略 constraints，尺寸取自身 min/max（Table/FileBrowser 同契约）。
    bool fixed_size() const override { return true; }

    void arrange(Rect slot) override
    {
        set_pos(slot.pos);
        // 窗口是固定尺寸控件：只用 slot 定位，不拿 slot.size 覆盖自身尺寸，
        // 否则被放进 Vbox 时会被拉伸。absolute_pos_ 保持「相对父内容区」语义。
        global_ = {slot.pos, {width(), height()}};
        layout();
    }

    void layout() override
    {
        // 延迟销毁的落地点：layout 在事件派发之外，且 UIRoot 已用 trash_ 延迟实际析构。
        if (pending_destroy_) {
            pending_destroy_ = false;
            destroy();
            return;
        }
        if (body_ == nullptr) return;
        const Chrome c = chrome();
        const float title = title_height();
        float footer_h = 0.0f;
        if (footer_ != nullptr && footer_->visible()) {
            footer_h = footer_->measure({global_.size.x, 0.0f}).y;
        }
        const float body_h = collapsed_ ? 0.0f : std::max(0.0f, height() - title - footer_h);
        body_->set_visible(!collapsed_);
        // 有 footer 时内容区全宽（footer 已覆盖底部抓取带），否则右侧留 resize 抓取带。
        const float body_w = footer_ != nullptr ? global_.size.x
                                                : std::max(0.0f, global_.size.x - c.grip);
        // footer 已覆盖底部，body 不再为底部抓取带留空，避免中间 box 与 footer 之间出现缝。
        const float body_grip = footer_ != nullptr ? 0.0f : c.grip;
        body_->arrange({{global_.pos.x + body_grip, global_.pos.y + title},
                        {body_w - body_grip, std::max(0.0f, body_h - body_grip)}});
        if (footer_ != nullptr) {
            footer_->set_visible(!collapsed_ && visible());
            footer_->arrange({{global_.pos.x, global_.pos.y + height() - footer_h},
                              {global_.size.x, footer_h}});
        }
    }

    void append_to(graphics::DrawList& list) const override
    {
        if (!visible()) return;
        const auto& t = ui_theme();
        const glm::vec4 bounds{global_.pos.x, global_.pos.y, global_.size.x, global_.size.y};
        const float radius = corner_radius(t.radius_panel);
        const bool active = mode_ != Mode::None;

        // 窗口底：比面板更亮的一档，否则窗口会与父面板融成一片。
        graphics::append_rounded_rect(list, bounds, radius, t.elevated);
        // 边框统一走圆角 ring：classic 下 radius = 0 时自动退化为直边，
        // 不再额外画直角矩形，避免圆角模式下露出尖角。
        // 焦点强调用基类的 is_focused()，不再自存一份状态。
        const bool emphasized = is_focused() || active;
        const glm::vec4 border_color = emphasized ? t.accent : t.elevated_border;
        graphics::append_rounded_ring(list, bounds, radius, emphasized ? 2.0f : 1.0f,
                                      border_color);

        // 标题栏（无标题栏窗口跳过）。
        const float title = title_height();
        const Chrome c = chrome();
        auto* atlas = font_atlas();
        if (title > 0.0f) {
            const glm::vec4 bar{bounds.x, bounds.y, bounds.z, title};
            graphics::append_rounded_rect(list, bar, radius, t.button_hover);

            if (atlas != nullptr) {
                // 用字体行高而不是字号：行框通常比字号高，按字号算文字会整体偏下。
                const float text_y = atlas->centered_text_y(t.text_size, bar.y, title);
                atlas->append_text(list, bar.x + c.glyph, text_y, t.text_size, title_, t.text);
                // debug 是叠加层，不能替代字形本身。
                if (debug_.any()) {
                    graphics::append_text_debug(list, *atlas, bar.x + c.glyph, text_y,
                                                t.text_size, title_, debug_);
                }
            }

            // 折叠/展开箭头（左）：两臂均为 45°（|dx| == |dy|），展开朝下、折叠朝右。
            const glm::vec4 glyph_color = t.text_muted;
            // 不可收起的窗口不画折叠三角（标题仍可拖动）。
            if (collapsible_) {
                const Rect box = collapse_rect();
                const float cx = box.pos.x + box.size.x * 0.5f;
                const float cy = box.pos.y + box.size.y * 0.5f;
                // 臂长（每轴）：图标方块的四分之一，45° 即 dx == dy == arm。
                const float arm = std::min(box.size.x, box.size.y) * 0.25f;
                const float half = arm * 0.5f;
                if (collapsed_) {
                    // 朝右 “▶”：顶点在右中，两臂往左上下 45°。
                    const glm::vec2 tip{cx + half, cy};
                    const glm::vec2 up{cx - half, cy - arm};
                    const glm::vec2 down{cx - half, cy + arm};
                    graphics::Line{up.x, up.y, tip.x, tip.y, 1.5f, glyph_color}.append_to(list);
                    graphics::Line{tip.x, tip.y, down.x, down.y, 1.5f, glyph_color}.append_to(list);
                } else {
                    // 朝下 “▼”：顶点在下中，两臂往左上/右上 45°。
                    const glm::vec2 tip{cx, cy + half};
                    const glm::vec2 left{cx - arm, cy - half};
                    const glm::vec2 right{cx + arm, cy - half};
                    graphics::Line{left.x, left.y, tip.x, tip.y, 1.5f, glyph_color}.append_to(list);
                    graphics::Line{tip.x, tip.y, right.x, right.y, 1.5f, glyph_color}.append_to(list);
                }
            }
            // 关闭叉：与 hit_close 保持同一可见性条件（折叠/无标题栏时不画也不命中）。
            if (closable()) {
                const Rect box = close_rect();
                const float cx = box.pos.x + box.size.x * 0.5f;
                const float cy = box.pos.y + box.size.y * 0.5f;
                const float s = std::min(box.size.x, box.size.y) * 0.2f;
                graphics::Line{cx - s, cy - s, cx + s, cy + s, 1.5f, glyph_color}.append_to(list);
                graphics::Line{cx + s, cy - s, cx - s, cy + s, 1.5f, glyph_color}.append_to(list);
            }
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

        // 底部栏：全宽贴窗口底（在内容区 clip 之外，覆盖底部 resize 抓取带）。
        if (!collapsed_ && footer_ != nullptr && footer_->visible()) {
            // 窗口窄于内容时裁掉溢出部分，避免按钮跑出窗口。
            list.push_clip(bounds);
            footer_->append_to(list);
            list.pop_clip();
        }
    }

    // append_to 把标题/内容/底栏都裁剪到窗口 bounds；命中必须同步裁剪，
    // 否则布局到窗口外的子项画不出来却能点得到。
    bool clips_children() const override { return true; }

    bool on_key_down(event::KeyCode key, event::Modifier) override
    {
        // Esc 关闭需要显式开启（模态窗口默认开启），否则普通窗口里的 Esc 会误关窗口。
        if (key != event::KeyCode::Escape || !esc_closable_) return false;
        return request_close();
    }

    void on_focus_gained() override
    {
        Widget::on_focus_gained();
        mark_layout_dirty();
    }
    void on_focus_lost() override
    {
        Widget::on_focus_lost();
        mark_layout_dirty();
    }

    // 双击标题栏最大化/还原（点在关闭/折叠图标上不算）。
    bool on_double_click(glm::vec2 pos, int) override
    {
        if (!title_bar_visible_ || !hit_title(pos) || hit_close(pos) || hit_collapse(pos)) {
            return false;
        }
        set_maximized(!maximized_);
        return true;
    }

    bool on_mouse_down(glm::vec2 pos, int button) override
    {
        if (button != 0 || !visible()) return false;
        raise();

        if (hit_close(pos)) {
            request_close();
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
                // 记起始矩形 + 起始鼠标点：8 个方向共用同一套增量算法。
                resize_start_pos_ = absolute_pos();
                resize_start_size_ = {width(), height()};
                grab_ = pos;
                return true;
            }
        }
        // 有标题栏时只认标题条（高度为 0 时 hit_title 永不命中）；
        // 无标题栏时整块都是拖动带（内容里的控件会先接点击，到不了这里）。
        const bool drag_area =
            hit_title(pos) || (!title_bar_visible_ && global_bounds().contains(pos));
        if (drag_area) {
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
            const glm::vec2 delta = pos - grab_;
            glm::vec2 next_pos = resize_start_pos_;
            glm::vec2 next_size = resize_start_size_;
            if ((edges_ & kEdgeLeft) != 0) {
                next_pos.x += delta.x;
                next_size.x -= delta.x;
            }
            if ((edges_ & kEdgeTop) != 0) {
                next_pos.y += delta.y;
                next_size.y -= delta.y;
            }
            if ((edges_ & kEdgeRight) != 0) next_size.x += delta.x;
            if ((edges_ & kEdgeBottom) != 0) next_size.y += delta.y;

            // 夹到最小/最大尺寸：把左/上边被推回的那部分偿还给位置，避免窗口向右下飘。
            const float min_w = std::max(160.0f, min_size().x);
            const float min_h = collapsed_ ? title_height()
                                           : std::max(title_height() + 48.0f, min_size().y);
            if (next_size.x < min_w) {
                if ((edges_ & kEdgeLeft) != 0) next_pos.x -= min_w - next_size.x;
                next_size.x = min_w;
            }
            if (next_size.y < min_h) {
                if ((edges_ & kEdgeTop) != 0) next_pos.y -= min_h - next_size.y;
                next_size.y = min_h;
            }
            next_size.x = std::min(next_size.x, max_size().x);
            next_size.y = std::min(next_size.y, max_size().y);

            set_absolute_pos(next_pos);
            set_pos(next_pos);
            global_.pos = next_pos;
            set_size(next_size);
            global_.size = next_size;
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
        // 还原模态前的置顶 / Esc 状态：模态期间的置顶与 Esc 是临时的，不能粘住。
        set_always_on_top(always_on_top_before_modal_);
        esc_closable_ = esc_closable_before_modal_;
    }

    static constexpr float kGripEdge = 6.0f;
    static constexpr int kOnTopZ = 10000;
    static constexpr int kEdgeRight = 1;
    static constexpr int kEdgeBottom = 2;
    static constexpr int kEdgeLeft = 4;
    static constexpr int kEdgeTop = 8;

    Rect close_rect() const
    {
        const Chrome c = chrome();
        return {{global_.pos.x + global_.size.x - c.inset - c.icon, global_.pos.y + c.inset},
                {c.icon, c.icon}};
    }
    Rect collapse_rect() const
    {
        const Chrome c = chrome();
        return {{global_.pos.x + c.inset, global_.pos.y + c.inset}, {c.icon, c.icon}};
    }
    Rect title_rect() const
    {
        return {{global_.pos.x, global_.pos.y}, {global_.size.x, title_height()}};
    }

    // 关闭叉可见/可命中：标题栏可见且未折叠。绘制与命中共用这一条判据。
    bool closable() const { return title_bar_visible_ && !collapsed_; }

    bool hit_close(glm::vec2 pos) const
    {
        if (!closable()) return false;
        return close_rect().contains(pos);
    }
    bool hit_collapse(glm::vec2 pos) const
    {
        if (!title_bar_visible_ || !collapsible_) return false;
        return collapse_rect().contains(pos);
    }
    bool hit_title(glm::vec2 pos) const { return title_rect().contains(pos); }

    // 四边 + 四角，角比边优先（8px 抓取带）。
    int resize_edges(glm::vec2 pos) const
    {
        const float left = global_.pos.x;
        const float top = global_.pos.y;
        const float right = left + global_.size.x;
        const float bottom = top + global_.size.y;
        const float grip = chrome().grip;

        const bool in_left = pos.x <= left + grip;
        const bool in_right = pos.x >= right - grip;
        // 顶边只留 6px：再宽就会把标题栏的拖动区一起吃进去。
        const bool in_top = pos.y <= top + grip;
        const bool in_bottom = pos.y >= bottom - grip;

        int edges = 0;
        if (in_left) edges |= kEdgeLeft;
        if (in_right) edges |= kEdgeRight;
        if (in_top) edges |= kEdgeTop;
        if (in_bottom) edges |= kEdgeBottom;
        return edges;
    }

    glm::vec2 clamp_position(glm::vec2 pos) const
    {
        const auto& t = ui_theme();
        (void)t;
        const float keep = chrome().keep_visible;
        const glm::vec2 area = parent() != nullptr ? glm::vec2{parent()->width(), parent()->height()}
                                                   : glm::vec2{1280.0f, 800.0f};
        const float max_x = std::max(0.0f, area.x - keep);
        const float min_x = keep - std::max(keep, global_.size.x);
        const float max_y = std::max(0.0f, area.y - (title_height() > 0.0f ? title_height() : keep));
        return {std::clamp(pos.x, min_x, max_x), std::clamp(pos.y, 0.0f, max_y)};
    }

    std::string title_;
    Vbox* body_ = nullptr;
    Widget* footer_ = nullptr;
    Mode mode_ = Mode::None;
    int edges_ = 0;
    glm::vec2 grab_{0.0f, 0.0f};
    bool collapsed_ = false;
    bool resizable_ = true;
    bool title_bar_visible_ = true;
    bool always_on_top_ = false;
    bool always_on_top_before_modal_ = false;
    bool esc_closable_before_modal_ = false;
    bool modal_ = false;
    bool collapsible_ = true;
    bool esc_closable_ = false;
    bool pending_destroy_ = false;
    bool maximized_ = false;
    graphics::TextDebugOptions debug_{};
    float expanded_height_ = 0.0f;
    glm::vec2 resize_start_pos_{0.0f, 0.0f};
    glm::vec2 resize_start_size_{0.0f, 0.0f};
    glm::vec2 restore_pos_{0.0f, 0.0f};
    glm::vec2 restore_size_{0.0f, 0.0f};
    std::function<void()> on_destroy_;
    std::function<bool()> on_close_;
};
} // namespace xel::ui
