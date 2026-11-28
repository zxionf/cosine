// 图标：MTSDF 图集（tools/mtsdf_gen 烘焙）在不同目标尺寸下的表现。
#include "sections/sections.hpp"

#include "ui/core/icon.hpp"
#include "ui/core/label.hpp"

#include <cstdio>

#include <memory>
#include <string>
#include <vector>

namespace demo::sections
{
namespace
{
// 图集里烘焙的全部图标名（与 tools/mtsdf_gen 的 kIcons 一致）。
const char* const kIconNames[] = {"rounded_rect", "circle", "play", "chevron",
                                 "arrow", "plus", "gear"};
constexpr float kSizes[] = {16.0f, 24.0f, 32.0f, 48.0f, 64.0f};

ui::Icon* add_icon(ui::Widget& parent, Demo& demo, const std::string& name, float size,
                   glm::vec4 color)
{
    auto icon = std::make_unique<ui::Icon>(&demo.sdf_atlas(), demo.sdf_slot(), name, size);
    icon->set_color(color);
    auto* ref = icon.get();
    parent.add(std::move(icon));
    return ref;
}
} // namespace

void build_icons(Demo& demo, ui::Vbox& column)
{
    auto& atlas = demo.sdf_atlas();

    auto& intro = group(column, "MTSDF 图集");
    hint(intro,
         "每个图标是图集里的一个 cell（RGB = MSDF 三通道，A = 真 SDF）。"
         "渲染时按 median(RGB) 重建距离，用 fwidth 把边缘收成 1 物理像素，"
         "所以同一份 96px cell 在 16~64px 都清晰 —— 这是位图缩放做不到的。");
    char summary[256];
    std::snprintf(summary, sizeof(summary),
                  "atlas %dx%d · cell %dpx · px_range %.1f · %d 个图标 · 图片槽 %d%s",
                  atlas.width(), atlas.height(), atlas.cell(),
                  static_cast<double>(atlas.px_range()), atlas.icon_count(), demo.sdf_slot(),
                  atlas.valid() ? "" : "（未加载：显示占位）");
    hint(intro, summary);

    // 1) 尺寸阶梯：把「分辨率无关」直接摆出来。
    auto& ladder = group(column, "同一图标 × 5 种目标尺寸");
    hint(ladder, "16 / 24 / 32 / 48 / 64px。字形位图在这个跨度内会糊，SDF 不会。");
    for (const char* name : kIconNames) {
        auto& line = row(ladder, 16.0f);
        auto caption = std::make_unique<ui::Label>(name, 12.0f);
        caption->set_min_size({110.0f, 0.0f});
        line.add(std::move(caption));
        for (const float size : kSizes) {
            add_icon(line, demo, name, size, line.ui_theme().text);
        }
    }

    // 2) 颜色与语义：图标颜色只取顶点色，同一个 cell 可以染成任意颜色。
    auto& tinted = group(column, "染色：同一 cell × 不同颜色");
    hint(tinted, "MTSDF 的覆盖率乘在顶点色上，所以图标没有「固有色」的限制。");
    auto& tint_row = row(tinted, 12.0f);
    const glm::vec4 palette[] = {
        glm::vec4{0.36f, 0.62f, 1.00f, 1.0f},
        glm::vec4{0.36f, 0.84f, 0.58f, 1.0f},
        glm::vec4{1.00f, 0.62f, 0.30f, 1.0f},
        glm::vec4{0.95f, 0.40f, 0.66f, 1.0f},
        glm::vec4{0.62f, 0.48f, 1.00f, 1.0f},
    };
    for (const auto& color : palette) {
        add_icon(tint_row, demo, "gear", 40.0f, color);
    }
    hint(tinted, "半透明也成立：覆盖率 × 顶点 alpha，预乘后不会出现亮边。");
    auto& alpha_row = row(tinted, 12.0f);
    for (int i = 0; i < 4; ++i) {
        const float alpha = 0.25f + static_cast<float>(i) * 0.25f;
        add_icon(alpha_row, demo, "play", 40.0f, {0.88f, 0.90f, 0.94f, alpha});
    }

    // 3) 组合：图标 + 文字，就是按钮内部最常见的形态。
    auto& composed = group(column, "组合：图标 + 文字");
    hint(composed, "图标不参与命中与焦点（hit_testable=false），点击交给宿主控件。");
    auto& combo = row(composed, 8.0f);
    add_icon(combo, demo, "play", 20.0f, combo.ui_theme().text);
    auto label = std::make_unique<ui::Label>("Play", 14.0f);
    combo.add(std::move(label));
    gap(composed, 6.0f);
    auto& combo2 = row(composed, 8.0f);
    add_icon(combo2, demo, "arrow", 20.0f, combo2.ui_theme().text);
    combo2.add(std::make_unique<ui::Label>("Next", 14.0f));

    // 4) 降级：图集未加载 / 名字不存在时画占位方块。
    auto& fallback = group(column, "降级路径");
    hint(fallback,
         "图集缺失、槽未上传或名字拼错时画描边方块 —— UI 不会崩，也不会出错位的黑块。");
    auto& bad = row(fallback, 12.0f);
    add_icon(bad, demo, "no_such_icon", 32.0f, bad.ui_theme().text);
    hint(fallback, "上面这个方块就是「名字不存在」时的样子。");
}
} // namespace demo::sections
