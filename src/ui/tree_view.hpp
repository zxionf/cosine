#pragma once

#include "theme.hpp"
#include "widget.hpp"
#include "../graphics/font_atlas.hpp"
#include "../graphics/line.hpp"
#include "../graphics/rectangle.hpp"

#include <algorithm>
#include <cmath>
#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace xel::ui
{
struct TreeNode
{
    std::string title;
    std::vector<TreeNode> children;
    bool expanded = false;

    bool has_children() const { return !children.empty(); }
};

// 树形列表：层级缩进、展开/折叠、键盘导航、自绘滚动条。
// 选中用「根索引路径」表示（如 {0, 2, 1}），展开/折叠不改变其含义。
class TreeView : public Widget
{
public:
    TreeView(float width = 320.0f, float height = 240.0f)
    {
        set_min_size({width, height});
        set_max_size({width, height});
    }

    void set_roots(std::vector<TreeNode> roots)
    {
        roots_ = std::move(roots);
        validate_selection();
        scroll_ = 0.0f;
        mark_layout_dirty();
    }
    const std::vector<TreeNode>& roots() const { return roots_; }
    std::size_t root_count() const { return roots_.size(); }

    void add_root(TreeNode node)
    {
        roots_.push_back(std::move(node));
        mark_layout_dirty();
    }

    void clear_roots()
    {
        roots_.clear();
        selected_path_.clear();
        hovered_ = -1;
        scroll_ = 0.0f;
        mark_layout_dirty();
    }

    const TreeNode* node_at(const std::vector<int>& path) const
    {
        const std::vector<TreeNode>* level = &roots_;
        const TreeNode* node = nullptr;
        for (const int index : path) {
            if (index < 0 || index >= static_cast<int>(level->size())) return nullptr;
            node = &(*level)[static_cast<std::size_t>(index)];
            level = &node->children;
        }
        return node;
    }
    TreeNode* node_at(const std::vector<int>& path)
    {
        return const_cast<TreeNode*>(static_cast<const TreeView*>(this)->node_at(path));
    }

    bool is_expanded(const std::vector<int>& path) const
    {
        const TreeNode* node = node_at(path);
        return node && node->expanded;
    }

    const std::vector<int>& selected_path() const { return selected_path_; }
    const TreeNode* selected_node() const { return node_at(selected_path_); }

    void set_selected_path(std::vector<int> path, bool notify = true)
    {
        if (node_at(path) == nullptr) return;
        if (path == selected_path_) return;
        selected_path_ = std::move(path);
        ensure_selected_visible();
        if (notify && on_select_) on_select_(selected_path_);
    }

    void clear_selection()
    {
        selected_path_.clear();
    }

    void set_on_select(std::function<void(const std::vector<int>&)> handler)
    {
        on_select_ = std::move(handler);
    }

    void set_expanded(const std::vector<int>& path, bool expanded)
    {
        TreeNode* node = node_at(path);
        if (!node || !node->has_children() || node->expanded == expanded) return;
        node->expanded = expanded;

        // 折叠后子树不可见：选中若在其内部，上移到被折叠的节点。
        if (!expanded && selected_path_.size() > path.size() &&
            std::equal(path.begin(), path.end(), selected_path_.begin())) {
            selected_path_ = path;
            if (on_select_) on_select_(selected_path_);
        }

        scroll_ = clamp_scroll(scroll_);
        mark_layout_dirty();
    }

    void toggle(const std::vector<int>& path) { set_expanded(path, !is_expanded(path)); }

    void expand_all() { set_all_expanded(true); }
    void collapse_all()
    {
        set_all_expanded(false);
        validate_selection();
    }

    void set_row_height(float height)
    {
        row_height_ = std::max(12.0f, height);
        mark_layout_dirty();
    }
    float row_height() const { return row_height_; }

    void set_indent(float indent)
    {
        indent_ = std::max(0.0f, indent);
        mark_layout_dirty();
    }
    float indent() const { return indent_; }

    void set_striped(bool striped) { striped_ = striped; }
    bool striped() const { return striped_; }

    float scroll_offset() const { return scroll_; }

    // 可见行（展开节点的前序遍历），交互与绘制共用同一份定义。
    struct VisibleRow
    {
        const TreeNode* node = nullptr;
        std::vector<int> path;
        int depth = 0;
    };

    std::vector<VisibleRow> visible_rows() const
    {
        std::vector<VisibleRow> rows;
        collect_visible(roots_, {}, 0, rows);
        return rows;
    }
    std::size_t visible_count() const { return visible_rows().size(); }

    glm::vec2 measure(glm::vec2 constraints) override
    {
        (void)constraints;
        return clamp_size(global_.size.x > 0.0f ? global_.size : local_.size);
    }

    void arrange(Rect slot) override
    {
        global_ = slot;
        set_pos(slot.pos);
        set_size(slot.size);
        scroll_ = clamp_scroll(scroll_);
    }

    void append_to(graphics::DrawList& list) const override
    {
        const auto& t = ui_theme();

        graphics::Rectangle background{global_.pos.x, global_.pos.y, global_.size.x, global_.size.y,
                                       t.panel_bg};
        background.append_to(list);

        list.push_clip({global_.pos.x, global_.pos.y, global_.size.x, global_.size.y});

        const auto rows = visible_rows();
        const float content = static_cast<float>(rows.size()) * row_height_;
        const float bar_w = content > global_.size.y ? t.scrollbar_width + 2.0f : 0.0f;
        const float body_w = std::max(0.0f, global_.size.x - bar_w);

        list.push_clip({global_.pos.x, global_.pos.y, body_w, global_.size.y});

        const int first_visible = row_height_ > 0.0f
            ? static_cast<int>(std::max(0.0f, scroll_) / row_height_)
            : 0;
        for (int index = first_visible; index < static_cast<int>(rows.size()); ++index) {
            const float y = global_.pos.y + static_cast<float>(index) * row_height_ - scroll_;
            if (y + row_height_ < global_.pos.y) continue;
            if (y > global_.pos.y + global_.size.y) break;
            draw_row(list, rows[static_cast<std::size_t>(index)], index, y, body_w);
        }
        list.pop_clip();

        draw_scrollbar(list, content, bar_w);

        if (focused_) {
            const float b = t.border;
            graphics::Rectangle top{global_.pos.x, global_.pos.y, global_.size.x, b, t.focus_ring};
            graphics::Rectangle bottom{global_.pos.x, global_.pos.y + global_.size.y - b,
                                       global_.size.x, b, t.focus_ring};
            top.append_to(list);
            bottom.append_to(list);
        }

        list.pop_clip();
    }

    bool hit_testable() const override { return true; }
    bool focusable() const override { return true; }
    bool clips_children() const override { return true; }

    void on_mouse_enter() override { hovered_ = row_at(last_mouse_); }
    void on_mouse_leave() override { hovered_ = -1; }
    void on_hover(glm::vec2 pos) override
    {
        last_mouse_ = pos;
        hovered_ = row_at(pos);
    }

    bool on_click(glm::vec2 pos, int) override
    {
        const int index = row_at(pos);
        if (index < 0) return false;
        const auto rows = visible_rows();
        if (index >= static_cast<int>(rows.size())) return false;

        const auto& row = rows[static_cast<std::size_t>(index)];
        if (row.node && row.node->has_children()) {
            // 箭头区域只切换展开状态，不改变选中。
            const float arrow_left = global_.pos.x + ui_theme().padding +
                                     static_cast<float>(row.depth) * indent_;
            if (pos.x >= arrow_left && pos.x <= arrow_left + arrow_width()) {
                toggle(row.path);
                return true;
            }
        }
        set_selected_path(row.path);
        return true;
    }

    bool on_scroll(glm::vec2 pos, glm::vec2 delta) override
    {
        if (!global_.contains(pos)) return false;
        if (content_height() <= body_height()) return false;
        scroll_ = clamp_scroll(scroll_ - delta.y * row_height_ * 1.5f);
        return true;
    }

    bool on_key_down(event::KeyCode key, event::Modifier) override
    {
        const auto rows = visible_rows();
        if (rows.empty()) return false;

        const int last = static_cast<int>(rows.size()) - 1;
        const int index = selected_index(rows);

        switch (key) {
            case event::KeyCode::Down:
                set_selected_path(rows[static_cast<std::size_t>(std::clamp(index + 1, 0, last))].path);
                return true;
            case event::KeyCode::Up:
                if (index < 0) set_selected_path(rows[static_cast<std::size_t>(last)].path);
                else set_selected_path(rows[static_cast<std::size_t>(std::max(0, index - 1))].path);
                return true;
            case event::KeyCode::Home:
                set_selected_path(rows.front().path);
                return true;
            case event::KeyCode::End:
                set_selected_path(rows.back().path);
                return true;
            case event::KeyCode::Right: {
                if (index < 0) {
                    set_selected_path(rows.front().path);
                    return true;
                }
                const auto& row = rows[static_cast<std::size_t>(index)];
                if (row.node && row.node->has_children()) {
                    if (!row.node->expanded) {
                        set_expanded(row.path, true);
                    } else {
                        auto child = row.path;
                        child.push_back(0);
                        set_selected_path(std::move(child));
                    }
                }
                return true;
            }
            case event::KeyCode::Left: {
                if (index < 0) {
                    set_selected_path(rows.front().path);
                    return true;
                }
                const auto& row = rows[static_cast<std::size_t>(index)];
                if (row.node && row.node->has_children() && row.node->expanded) {
                    set_expanded(row.path, false);
                } else if (row.path.size() > 1) {
                    auto parent = row.path;
                    parent.pop_back();
                    set_selected_path(std::move(parent));
                }
                return true;
            }
            case event::KeyCode::Enter:
            case event::KeyCode::KeypadEnter:
            case event::KeyCode::Space: {
                if (index < 0) return false;
                const auto& row = rows[static_cast<std::size_t>(index)];
                if (!row.node || !row.node->has_children()) return false;
                toggle(row.path);
                return true;
            }
            default:
                return false;
        }
    }

    void on_focus_gained() override { focused_ = true; }
    void on_focus_lost() override { focused_ = false; }

private:
    float arrow_width() const { return 14.0f; }

    float body_height() const { return std::max(0.0f, global_.size.y); }
    float content_height() const { return static_cast<float>(visible_count()) * row_height_; }

    float clamp_scroll(float value) const
    {
        const float max_scroll = std::max(0.0f, content_height() - body_height());
        return std::clamp(value, 0.0f, max_scroll);
    }

    static void collect_visible(const std::vector<TreeNode>& nodes, const std::vector<int>& base,
                                int depth, std::vector<VisibleRow>& out)
    {
        for (std::size_t i = 0; i < nodes.size(); ++i) {
            const TreeNode& node = nodes[i];
            std::vector<int> path = base;
            path.push_back(static_cast<int>(i));
            out.push_back(VisibleRow{&node, path, depth});
            if (node.expanded && node.has_children()) {
                collect_visible(node.children, path, depth + 1, out);
            }
        }
    }

    void set_all_expanded(bool expanded)
    {
        std::vector<TreeNode*> stack;
        for (auto& root : roots_) stack.push_back(&root);
        while (!stack.empty()) {
            TreeNode* node = stack.back();
            stack.pop_back();
            if (!node->has_children()) continue;
            node->expanded = expanded;
            for (auto& child : node->children) stack.push_back(&child);
        }
        scroll_ = clamp_scroll(scroll_);
        mark_layout_dirty();
    }

    // 选中路径可能因结构变化（set_roots / collapse_all）失效：
    // 截断到仍然可见的最长前缀。
    void validate_selection()
    {
        if (selected_path_.empty()) return;

        std::vector<int> walk;
        const std::vector<TreeNode>* level = &roots_;
        for (const int index : selected_path_) {
            if (index < 0 || index >= static_cast<int>(level->size())) break;
            const TreeNode& node = (*level)[static_cast<std::size_t>(index)];
            walk.push_back(index);
            if (!node.expanded) break;
            level = &node.children;
        }
        selected_path_ = std::move(walk);
    }

    int selected_index(const std::vector<VisibleRow>& rows) const
    {
        if (selected_path_.empty()) return -1;
        for (std::size_t i = 0; i < rows.size(); ++i) {
            if (rows[i].path == selected_path_) return static_cast<int>(i);
        }
        return -1;
    }

    void ensure_selected_visible()
    {
        if (selected_path_.empty()) return;
        if (body_height() <= 0.0f) {
            scroll_ = 0.0f;
            return;
        }
        const auto rows = visible_rows();
        for (std::size_t i = 0; i < rows.size(); ++i) {
            if (rows[i].path != selected_path_) continue;
            const float top = static_cast<float>(i) * row_height_;
            const float bottom = top + row_height_;
            if (top < scroll_) scroll_ = top;
            else if (bottom > scroll_ + body_height()) scroll_ = bottom - body_height();
            scroll_ = clamp_scroll(scroll_);
            return;
        }
    }

    int row_at(glm::vec2 pos) const
    {
        if (!global_.contains(pos)) return -1;
        const float local_y = pos.y - global_.pos.y + scroll_;
        const int index = static_cast<int>(std::floor(local_y / row_height_));
        if (index < 0 || index >= static_cast<int>(visible_count())) return -1;
        return index;
    }

    void draw_row(graphics::DrawList& list, const VisibleRow& row, int index,
                  float y, float body_w) const
    {
        const auto& t = ui_theme();
        const bool is_selected = row.path == selected_path_;
        const bool is_hovered = index == hovered_;

        if (is_selected) {
            const glm::vec4 color{t.focus_ring.r, t.focus_ring.g, t.focus_ring.b, 0.28f};
            graphics::Rectangle highlight{global_.pos.x, y, body_w, row_height_, color};
            highlight.append_to(list);
        } else if (is_hovered) {
            graphics::Rectangle highlight{global_.pos.x, y, body_w, row_height_, t.button_hover};
            highlight.append_to(list);
        } else if (striped_ && (index % 2) == 1) {
            graphics::Rectangle stripe{global_.pos.x, y, body_w, row_height_,
                                       glm::vec4{1.0f, 1.0f, 1.0f, 0.03f}};
            stripe.append_to(list);
        }

        const float pad = t.padding;
        const float x = global_.pos.x + pad + static_cast<float>(row.depth) * indent_;
        const float arrow = arrow_width();
        if (row.node && row.node->has_children()) {
            draw_chevron(list, x + arrow * 0.5f, y + row_height_ * 0.5f,
                         row.node->expanded, t.text_muted);
        }

        auto* atlas = font_atlas();
        if (!atlas || !row.node) return;

        const float text_x = x + arrow;
        const float text_y = y + std::max(0.0f, (row_height_ - atlas->line_height(t.text_size)) * 0.5f);
        const float clip_right = std::max(text_x, global_.pos.x + body_w - pad * 0.5f);
        list.push_clip({text_x, y, std::max(0.0f, clip_right - text_x), row_height_});
        atlas->append_text(list, text_x, text_y, t.text_size, row.node->title, t.text);
        list.pop_clip();
    }

    void draw_chevron(graphics::DrawList& list, float cx, float cy,
                      bool expanded, glm::vec4 color) const
    {
        const float s = 4.0f;
        if (expanded) {
            graphics::Line left{cx - s, cy - s * 0.5f, cx, cy + s * 0.5f, 1.5f, color};
            graphics::Line right{cx, cy + s * 0.5f, cx + s, cy - s * 0.5f, 1.5f, color};
            left.append_to(list);
            right.append_to(list);
        } else {
            graphics::Line top{cx - s * 0.5f, cy - s, cx + s * 0.5f, cy, 1.5f, color};
            graphics::Line bottom{cx + s * 0.5f, cy, cx - s * 0.5f, cy + s, 1.5f, color};
            top.append_to(list);
            bottom.append_to(list);
        }
    }

    void draw_scrollbar(graphics::DrawList& list, float content, float bar_w) const
    {
        const float body_h = body_height();
        if (bar_w <= 0.0f || body_h <= 0.0f) return;

        const auto& t = ui_theme();
        const float max_scroll = std::max(0.0f, content - body_h);
        const float track_x = global_.pos.x + global_.size.x - bar_w;

        graphics::Rectangle track{track_x, global_.pos.y, bar_w, body_h, t.scrollbar_track};
        track.append_to(list);

        const float ratio = std::min(1.0f, body_h / std::max(1.0f, content));
        const float thumb_h = std::max(t.scrollbar_min_thumb, body_h * ratio);
        const float travel = std::max(0.0f, body_h - thumb_h);
        const float thumb_y = global_.pos.y +
                              (max_scroll > 0.0f ? travel * (scroll_ / max_scroll) : 0.0f);

        graphics::Rectangle thumb{track_x + 1.0f, thumb_y, std::max(1.0f, bar_w - 2.0f), thumb_h,
                                  t.scrollbar_thumb};
        thumb.append_to(list);
    }

    std::vector<TreeNode> roots_;
    std::vector<int> selected_path_;
    int hovered_ = -1;
    glm::vec2 last_mouse_{-1.0f, -1.0f};
    float row_height_ = 26.0f;
    float indent_ = 16.0f;
    float scroll_ = 0.0f;
    bool striped_ = true;
    bool focused_ = false;
    std::function<void(const std::vector<int>&)> on_select_;
};
} // namespace xel::ui
