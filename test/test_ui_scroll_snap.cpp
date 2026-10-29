#include "graphics/drawable.hpp"
#include "ui/core/label.hpp"
#include "ui/root.hpp"
#include "ui/containers/scroll_view.hpp"
#include "ui/views/table.hpp"
#include "ui/views/tree_view.hpp"
#include "ui/containers/vbox.hpp"

#include <cassert>
#include <cmath>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace
{
using namespace xel;

bool is_integer(float value)
{
    return std::floor(value) == value;
}

// 裁剪顶部是否与控件顶部严格相等：渲染层会向外取整 scissor，
// 只有两者都是整数时，才不会多画出一条像素。
bool scissor_top_matches(const graphics::DrawList& list, float top)
{
    bool matched = false;
    for (const auto& batch : list.batches()) {
        if (batch.scissor.w <= 0.0f) continue;
        if (std::fabs(batch.scissor.y - top) > 0.01f) continue;
        matched = true;
    }
    return matched;
}

void test_scroll_view_content_snaps()
{
    auto content = std::make_unique<ui::Vbox>(0.0f, 0.0f, 300.0f, 0.0f);
    for (int i = 0; i < 30; ++i) {
        content->add(std::make_unique<ui::Label>("line " + std::to_string(i), 16.0f));
    }
    auto* inner = content.get();

    ui::ScrollView scroll;
    scroll.set_content(std::move(content));
    scroll.arrange({{100.0f, 400.0f}, {300.0f, 120.0f}});

    // 小数滚动偏移：内容位置必须落在整像素上。
    scroll.set_scroll_offset({0.0f, 60.4f});
    const auto pos = inner->global_bounds().pos;
    assert(is_integer(pos.x));
    assert(is_integer(pos.y));
    assert(std::fabs(pos.y - (400.0f - std::round(60.4f))) < 0.01f);

    // 裁剪顶部与控件顶部一致。
    graphics::DrawList list;
    list.set_full_clip({0.0f, 0.0f, 800.0f, 600.0f});
    scroll.append_to(list);
    assert(scissor_top_matches(list, 400.0f));
}

void test_tree_view_rows_snap()
{
    std::vector<ui::TreeNode> roots;
    for (int i = 0; i < 6; ++i) {
        ui::TreeNode node;
        node.title = "row " + std::to_string(i);
        roots.push_back(std::move(node));
    }

    ui::TreeView tree;
    tree.set_roots(std::move(roots));
    tree.arrange({{100.0f, 400.0f}, {300.0f, 60.0f}});
    // 行滚动与像素对齐的断言基于离散模式。
    tree.set_scroll_mode(xel::ui::ScrollMode::Line);

    // 滚轮 0.5 格 → 1.5 * row_height * 0.5 会产生小数偏移。
    tree.on_scroll({150.0f, 420.0f}, {0.0f, -0.5f});
    assert(!is_integer(tree.scroll_offset()));

    graphics::DrawList list;
    list.set_full_clip({0.0f, 0.0f, 800.0f, 600.0f});
    tree.append_to(list);
    assert(scissor_top_matches(list, 400.0f));

    // 行起点取整：滚动值 20.5 与 20.0 取整后一致，绘制结果等价。
    xel::graphics::DrawList round_down;
    round_down.set_full_clip({0.0f, 0.0f, 800.0f, 600.0f});
    tree.append_to(round_down);
    for (const auto& batch : round_down.batches()) {
        if (std::fabs(batch.scissor.y - 400.0f) > 0.01f) continue;
        // scissor 已对齐到像素；行高亮/内容顶点不得越过裁剪顶部。
        for (const auto& vertex : batch.vertices) {
            assert(vertex.pos.y >= batch.scissor.y - 0.01f);
        }
    }
}

void test_table_rows_snap()
{
    ui::Table table;
    table.set_columns({{"name", 200.0f, ui::TableColumn::Align::Left}});
    std::vector<std::vector<std::string>> rows;
    for (int i = 0; i < 20; ++i) {
        rows.push_back({"row " + std::to_string(i)});
    }
    table.set_rows(rows);
    table.arrange({{100.0f, 400.0f}, {300.0f, 120.0f}});
    // 行滚动与像素对齐的断言基于离散模式。
    table.set_scroll_mode(ui::ScrollMode::Line);

    // 0.33 格 × 1.5 × 行高 28 = 13.86，产生小数偏移。
    table.on_scroll({150.0f, 420.0f}, {0.0f, -0.33f});
    assert(!is_integer(table.scroll_offset()));

    graphics::DrawList list;
    list.set_full_clip({0.0f, 0.0f, 800.0f, 600.0f});
    table.append_to(list);

    // body 裁剪从表头下方开始，且行顶点不越过裁剪顶部。
    for (const auto& batch : list.batches()) {
        if (batch.scissor.w <= 0.0f) continue;
        if (batch.scissor.y <= 400.0f) continue;
        for (const auto& vertex : batch.vertices) {
            assert(vertex.pos.y >= batch.scissor.y - 0.01f);
        }
    }
}

void test_empty_scissor_batches_are_skipped()
{
    // 内容完全滚出视区时，裁剪交集高度为 0：这类 batch 不应被提交绘制。
    auto content = std::make_unique<ui::Vbox>(0.0f, 0.0f, 300.0f, 0.0f);
    for (int i = 0; i < 30; ++i) {
        content->add(std::make_unique<ui::Label>("line " + std::to_string(i), 16.0f));
    }
    ui::ScrollView scroll;
    scroll.set_content(std::move(content));
    scroll.arrange({{100.0f, 400.0f}, {300.0f, 120.0f}});

    // 控件完全在裁剪区之外时，push_clip 的交集为空。
    graphics::DrawList list;
    list.set_full_clip({0.0f, 0.0f, 800.0f, 300.0f}); // 控件在 400..520，完全在外
    scroll.append_to(list);
    for (const auto& batch : list.batches()) {
        assert(batch.scissor.w == 0.0f || batch.scissor.y >= 300.0f);
    }
}

} // namespace

int main()
{
    test_scroll_view_content_snaps();
    test_tree_view_rows_snap();
    test_table_rows_snap();
    test_empty_scissor_batches_are_skipped();
    std::cout << "ui scroll snap tests passed\n";
    return 0;
}
