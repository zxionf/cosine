#pragma once

#include "ui/style/theme.hpp"
#include "ui/style/style.hpp"
#include "ui/views/list_view.hpp"
#include "graphics/font_atlas.hpp"
#include "graphics/box.hpp"
#include "graphics/line.hpp"
#include "graphics/rectangle.hpp"

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

// 树形列表：层级缩进、展开/折叠、键盘导航。滚动/可见行/行窗口由 ListView 提供。
// 选中用「根索引路径」表示（如 {0, 2, 1}），展开/折叠不改变其含义。
class TreeView : public ListView
{
public:
    WidgetRole semantics_role() const override { return WidgetRole::Tree; }
    TreeView(float width = 320.0f, float height = 240.0f)
    {
        set_min_size({width, height});
        set_max_size({width, height});
        set_preferred_size({width, height});
        set_row_height(26.0f);
    }

    void set_roots(std::vector<TreeNode> roots)
    {
        roots_ = std::move(roots);
        validate_selection();
        invalidate_visible();
        sync_selection();
        scroll_.jump_to(0.0f);
        mark_layout_dirty();
    }

    // 测量失效（主题/密度/字号切换、invalidate_measure_recursive）也要丢掉行缓存：
    // 行缓存必须挂在统一的失效路径上，不能自己维护一套。
    void invalidate_measure() override
    {
        ListView::invalidate_measure();
        invalidate_visible();
    }
    const std::vector<TreeNode>& roots() const { return roots_; }
    std::size_t root_count() const { return roots_.size(); }

    void add_root(TreeNode node)
    {
        roots_.push_back(std::move(node));
        invalidate_visible();
        sync_selection();
        mark_layout_dirty();
    }

    void clear_roots()
    {
        roots_.clear();
        selected_path_.clear();
        hovered_ = -1;
        invalidate_visible();
        selected_ = -1;
        scroll_.jump_to(0.0f);
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
        sync_selection();
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

        invalidate_visible();
        sync_selection();
        mark_layout_dirty();
    }

    void toggle(const std::vector<int>& path) { set_expanded(path, !is_expanded(path)); }

    void expand_all() { set_all_expanded(true); }
    void collapse_all()
    {
        set_all_expanded(false);
        validate_selection();
        sync_selection();
    }

    void set_indent(float indent)
    {
        indent_ = std::max(0.0f, indent);
        mark_layout_dirty();
    }
    float indent() const { return indent_; }

    void set_striped(bool striped) { striped_ = striped; }
    bool striped() const { return striped_; }

    // 滚动模式/平滑/offset 均继承自 ListView（ScrollState）。

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
    std::size_t visible_count() const { return cached_visible_rows().size(); }

    // ListView 契约：行数 = 展开后的可见节点数。
    std::size_t item_count() const override { return cached_visible_rows().size(); }

    glm::vec2 measure(glm::vec2 constraints) override
    {
        (void)constraints;
        return clamp_size(preferred_size());
    }

    void arrange(Rect slot) override
    {
        ListView::arrange(slot);
        sync_scroll_range();
    }

    void append_to(graphics::DrawList& list) const override
    {
        const auto& t = ui_theme();

        const glm::vec4 bounds{bounds_.pos.x, bounds_.pos.y, bounds_.size.x, bounds_.size.y};
        const BoxStyle surface = t.style(StyleRole::Surface, ControlState::Normal);
        const float panel_radius = corner_radius(surface.radius);
        graphics::append_rounded_rect(list, bounds, panel_radius, surface.fill);

        list.push_clip({std::round(bounds_.pos.x), std::round(bounds_.pos.y),
                        std::round(bounds_.size.x), std::round(bounds_.size.y)});

        const auto& rows = cached_visible_rows();
        const float content = static_cast<float>(rows.size()) * row_height_;
        const float bar_w = content > bounds_.size.y ? t.scrollbar_width + 2.0f : 0.0f;
        const float body_w = std::max(0.0f, bounds_.size.x - bar_w);

        list.push_clip({std::round(bounds_.pos.x), std::round(bounds_.pos.y),
                        std::round(body_w), std::round(bounds_.size.y)});

        // 行循环统一走 ListView::append_rows（像素对齐、可见窗口、裁剪都在基类）。
        append_rows(list, Rect{{bounds_.pos.x, bounds_.pos.y}, {body_w, bounds_.size.y}});
        list.pop_clip();

        draw_scrollbar(list, bounds_.pos.y, viewport_height(), bar_w);

        append_focus_ring(list, bounds, panel_radius);

        list.pop_clip();
    }

    bool hit_testable() const override { return true; }
    bool focusable() const override { return true; }
    Overflow overflow() const override { return Overflow::Clip; }

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
        const auto& rows = cached_visible_rows();
        if (index >= static_cast<int>(rows.size())) return false;

        const auto& row = rows[static_cast<std::size_t>(index)];
        if (row.node && row.node->has_children()) {
            // 箭头区域只切换展开状态，不改变选中。
            const float arrow_left = bounds_.pos.x + ui_theme().padding +
                                     static_cast<float>(row.depth) * indent_;
            if (pos.x >= arrow_left && pos.x <= arrow_left + arrow_width()) {
                toggle(row.path);
                return true;
            }
        }
        set_selected_path(row.path);
        return true;
    }

    // 双击行：切换展开/折叠（与 Enter/Space 一致，叶子行不响应）。
    // 箭头区域由单击负责切换，双击不再重复切换，避免双击时切换两次。
    bool on_double_click(glm::vec2 pos, int) override
    {
        const int index = row_at(pos);
        if (index < 0) return false;
        const auto& rows = cached_visible_rows();
        if (index >= static_cast<int>(rows.size())) return false;

        const auto& row = rows[static_cast<std::size_t>(index)];
        if (!row.node || !row.node->has_children()) return false;

        const float arrow_left = bounds_.pos.x + ui_theme().padding +
                                 static_cast<float>(row.depth) * indent_;
        if (pos.x >= arrow_left && pos.x <= arrow_left + arrow_width()) return false;

        toggle(row.path);
        return true;
    }

    // 滚轮/平滑滚动由 ListView 的 ScrollState 处理（on_scroll / on_update 不再覆写）。

    bool on_key_down(event::KeyCode key, event::Modifier) override
    {
        const auto& rows = cached_visible_rows();
        if (rows.empty()) return false;

        const int last = static_cast<int>(rows.size()) - 1;
        const int index = selected_;

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

    void on_focus_lost() override
    {
        Widget::on_focus_lost();
    }

private:
    float arrow_width() const { return 14.0f; }

    void invalidate_visible() { visible_cache_valid_ = false; }

    const std::vector<VisibleRow>& cached_visible_rows() const
    {
        if (!visible_cache_valid_) {
            visible_cache_ = visible_rows();
            visible_cache_valid_ = true;
        }
        return visible_cache_;
    }

    // 把 selected_path_ 映射为 ListView 的行号（结构变化后同步）。
    void sync_selection()
    {
        if (selected_path_.empty()) {
            selected_ = -1;
            return;
        }
        const auto& rows = cached_visible_rows();
        for (std::size_t i = 0; i < rows.size(); ++i) {
            if (rows[i].path == selected_path_) {
                selected_ = static_cast<int>(i);
                return;
            }
        }
        selected_ = -1;
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
        invalidate_visible();
        sync_selection();
        scroll_.jump_to(scroll_.offset());
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

    int row_at(glm::vec2 pos) const
    {
        if (!bounds_.contains(pos)) return -1;
        // 统一用 ListView 的 row_at（基于 snapped_offset，与绘制一致）。
        return ListView::row_at(pos.y, bounds_.pos.y);
    }

    void draw_row(graphics::DrawList& list, std::size_t index, Rect row_rect, bool selected) const override
    {
        const auto& t = ui_theme();
        const auto& rows = cached_visible_rows();
        if (index >= rows.size()) return;
        const VisibleRow& row = rows[index];
        const bool is_hovered = static_cast<int>(index) == hovered_;

        const float row_radius = corner_radius(t.radius_small);
        if (selected) {
            // 选中行走 RowSelected 角色（与 Table 一致）。
            const BoxStyle sel = t.style(StyleRole::RowSelected, ControlState::Normal);
            graphics::append_rounded_rect(list, {row_rect.pos.x + 2.0f, row_rect.pos.y, row_rect.size.x - 4.0f, row_rect.size.y},
                                          row_radius, sel.fill);
        } else if (is_hovered) {
            const BoxStyle row_style = t.style(StyleRole::Row, ControlState::Hovered);
            graphics::append_rounded_rect(list, {row_rect.pos.x + 2.0f, row_rect.pos.y, row_rect.size.x - 4.0f, row_rect.size.y},
                                          row_radius, row_style.fill);
        } else if (striped_ && (index % 2) == 1) {
            graphics::append_rounded_rect(list, {row_rect.pos.x + 2.0f, row_rect.pos.y, row_rect.size.x - 4.0f, row_rect.size.y},
                                          row_radius, {1.0f, 1.0f, 1.0f, 0.03f});
        }

        const float pad = t.padding;
        const float x = row_rect.pos.x + pad + static_cast<float>(row.depth) * indent_;
        const float arrow = arrow_width();
        if (row.node && row.node->has_children()) {
            draw_chevron(list, x + arrow * 0.5f, row_rect.pos.y + row_rect.size.y * 0.5f,
                         row.node->expanded, t.text_muted);
        }

        auto* atlas = font_atlas();
        if (!atlas || !row.node) return;

        const float text_x = x + arrow;
        const float text_y = atlas->centered_text_y(t.text_size, row_rect.pos.y, row_rect.size.y);
        const float clip_right = std::max(text_x, row_rect.pos.x + row_rect.size.x - pad * 0.5f);
        list.push_clip({text_x, row_rect.pos.y, std::max(0.0f, clip_right - text_x), row_rect.size.y});
        // 行文字色取 Row 角色。
        atlas->append_text(list, text_x, text_y, t.text_size, row.node->title,
                           t.style(StyleRole::Row, ControlState::Normal).text);
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

    // 滚动条绘制由 ListView 基类提供（draw_scrollbar）。

    std::vector<TreeNode> roots_;
    std::vector<int> selected_path_;
    mutable std::vector<VisibleRow> visible_cache_;
    mutable bool visible_cache_valid_ = false;
    int hovered_ = -1;
    glm::vec2 last_mouse_{-1.0f, -1.0f};
    float indent_ = 16.0f;
    bool striped_ = true;
    std::function<void(const std::vector<int>&)> on_select_;
};
} // namespace xel::ui
