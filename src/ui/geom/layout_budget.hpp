#pragma once

// 主轴（流式方向）尺寸分配的统一接口（UI-32）。
// Vbox / Hbox 各自手写过一套「flex extra 分配」，现在收敛到这里；
// Grid / Wrap 的伸缩与 clamp 规则也复用同一套语义。
//
// 规则（详见 docs/ui_architecture.md「布局契约」）：
//   1. fixed 子项（fixed_size()）直接用 natural，不参与预算 / flex / stretch / shrink；
//   2. pct / fill 子项先认领预算：fill 视为 pct = 1；每个预算子项的认领额 =
//      pct × (available − 自身主轴 margin)；多个预算子项总和超出可用内容空间时
//      按认领额等比压缩，保证总和 ≤ 可用；
//   3. 其余子项取 natural；flex 子项的 natural 由调用方传 min_size（主轴），
//      这样 ScrollPanel 这类「内容远大于可视区」的 flex 子项不会把容器自然尺寸撑爆；
//   4. 剩余空间按 flex 分配：extra > 0 增长（上限 max_size）、extra < 0 收缩（下限 min_size）；
//   5. 结果统一 clamp 到 [min_size, max_size]。
//
// spacing 是相邻子项间距；margin 参与主轴自然尺寸与预算（全轴语义，UI-30 起）。

#include "ui/layout_metrics.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

namespace xel::ui
{

struct MainAxisItem
{
    float pct = 0.0f;         // 主轴 size_percent（> 0 启用）
    bool fill = false;        // 主轴 fill
    bool fixed = false;       // fixed_size()
    float flex = 0.0f;        // flex_grow（> 0 参与 grow/shrink）
    float natural = 0.0f;     // 内容自然主轴尺寸（flex 子项应传 min_size 主轴）
    float margin_before = 0.0f; // 主轴前 margin
    float margin_after = 0.0f;  // 主轴后 margin
    float min_size = 0.0f;      // 主轴 min_size
    float max_size = 1.0e9f;    // 主轴 max_size
};

// distribute_flex = false 用于容器自身的 measure（自然尺寸）：
// 只算 pct 预算与普通子项，不让 flex 子项把容器撑到可用尺寸。
inline std::vector<float> resolve_main_axis(float available, float spacing,
                                            const std::vector<MainAxisItem>& items,
                                            bool distribute_flex = true)
{
    const std::size_t n = items.size();
    std::vector<float> out(n, 0.0f);
    if (n == 0) return out;

    // 无界主轴：不做预算 / flex 分配，直接返回自然尺寸（clamp）。
    if (!is_bounded(available)) {
        for (std::size_t i = 0; i < n; ++i) {
            out[i] = glm::clamp(items[i].natural, items[i].min_size, items[i].max_size);
        }
        return out;
    }

    std::vector<bool> is_budget(n, false);
    std::vector<bool> is_flex(n, false);
    float flex_sum = 0.0f;
    for (std::size_t i = 0; i < n; ++i) {
        const auto& it = items[i];
        if (it.fixed) continue;
        if (it.fill || it.pct > 0.0f) {
            is_budget[i] = true;
        } else if (it.flex > 0.0f) {
            is_flex[i] = true;
            flex_sum += it.flex;
        }
    }

    // 非预算子项（固定 / 普通 / flex）先占位；flex 用调用方传入的 natural（min_size）。
    float base_total = spacing * static_cast<float>(n - 1);
    float budget_margin_total = 0.0f;
    float budget_content_want = 0.0f;
    for (std::size_t i = 0; i < n; ++i) {
        const auto& it = items[i];
        if (is_budget[i]) {
            budget_margin_total += it.margin_before + it.margin_after;
            const float claim = (it.fill ? 1.0f : it.pct)
                * std::max(0.0f, available - it.margin_before - it.margin_after);
            budget_content_want += claim;
        } else {
            out[i] = glm::clamp(it.natural, it.min_size, it.max_size);
            base_total += it.margin_before + out[i] + it.margin_after;
        }
    }

    // 预算可用内容空间 = 剩余空间 − 预算子项自身 margin。
    const float budget_content_available = available - base_total - budget_margin_total;
    float budget_scale = 1.0f;
    if (budget_content_want > budget_content_available + 1e-3f && budget_content_want > 1e-6f) {
        budget_scale = std::max(0.0f, budget_content_available) / budget_content_want;
    }

    for (std::size_t i = 0; i < n; ++i) {
        if (!is_budget[i]) continue;
        const auto& it = items[i];
        const float claim = (it.fill ? 1.0f : it.pct)
            * std::max(0.0f, available - it.margin_before - it.margin_after);
        out[i] = glm::clamp(claim * budget_scale, it.min_size, it.max_size);
        base_total += it.margin_before + out[i] + it.margin_after;
    }

    // flex 分配剩余（grow / shrink），迭代处理 clamp 到边界后剩余不归零的情况。
    if (distribute_flex && flex_sum > 0.0f) {
        float extra = available - base_total;
        for (int pass = 0; pass < 4 && std::fabs(extra) > 1e-3f; ++pass) {
            float movable = 0.0f;
            for (std::size_t i = 0; i < n; ++i) {
                if (!is_flex[i]) continue;
                if (extra > 0.0f && out[i] < items[i].max_size - 1e-3f) movable += items[i].flex;
                else if (extra < 0.0f && out[i] > items[i].min_size + 1e-3f) movable += items[i].flex;
            }
            if (movable <= 0.0f) break;
            for (std::size_t i = 0; i < n; ++i) {
                if (!is_flex[i]) continue;
                if (extra > 0.0f && out[i] >= items[i].max_size - 1e-3f) continue;
                if (extra < 0.0f && out[i] <= items[i].min_size + 1e-3f) continue;
                out[i] = glm::clamp(out[i] + extra * items[i].flex / movable,
                                    items[i].min_size, items[i].max_size);
            }
            float new_total = spacing * static_cast<float>(n - 1);
            for (std::size_t i = 0; i < n; ++i) {
                new_total += items[i].margin_before + out[i] + items[i].margin_after;
            }
            extra = available - new_total;
        }
    }

    return out;
}

} // namespace xel::ui
