#pragma once

#include "ui/geom/align.hpp"

#include <glm/glm.hpp>

#include <limits>

namespace xel::ui
{
// 可见性三态：
//   Flow       参与流式布局（默认）
//   Positioned 绝对定位：不占位，但仍 arrange（相当于旧的 absolute）
//   Hidden     完全不参与布局，也不绘制
//
// Widget 内用 `using Display = ::xel::ui::Display;` 保持 `Widget::Display::Flow` 的
// 既有写法，因此 43 个控件无需改动。
enum class Display
{
    Flow,
    Positioned,
    Hidden,
};

// 布局意图：把控件上散落的布局字段聚成一份数据。
//
// 这一层只是**搬家**，不改语义：
//   - 默认值与原来的成员初始化完全一致（max_size 为 FLT_MAX、size_percent 为 -1）；
//   - 访问器仍在 Widget 上（min_size()/set_fill()/layout_spec()…），转发到这里；
//   - 43 个控件对这些字段的引用数实测为 0，所以控件侧零修改。
struct LayoutParams
{
    glm::vec2 preferred{0.0f, 0.0f}; // 构造期自然尺寸（UI-32），与 arrange 的几何分离
    glm::vec2 min_size{0.0f, 0.0f};
    glm::vec2 max_size{std::numeric_limits<float>::max(), std::numeric_limits<float>::max()};
    glm::vec2 size_percent{-1.0f, -1.0f};
    glm::vec2 absolute_pos{0.0f, 0.0f};
    // 注意：可见性三态（Display）是 Node 的身份属性，不在布局意图里。
    float flex_grow = 0.0f;
    float aspect_ratio = 0.0f;
    bool fill_x = false;
    bool fill_y = false;
    Align align_self = Align::top_left();
    bool has_align = false;
    // 占位模式（Positioned/Hidden）由 Node 持有；这里只剩布局意图。
};
} // namespace xel::ui
