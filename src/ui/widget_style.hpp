#pragma once

#include "ui/style_types.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace xel::ui
{
// StyleRole 的槽位数（新增角色时同步这里；static_assert 会挡住漏改）。
inline constexpr std::size_t kStyleRoleCount = 12;

// 单个角色的过渡状态（UI-50 索引化）：
//   只记「从哪个状态 → 到哪个状态」+ 进度，颜色在渲染时按 (role, state) 从当前 Theme 现算。
//
// 为什么不再存 from/to 两份 BoxStyle：那是 1,688 B/控件（12 槽 × 140 B）且要堆分配。
// 目标样式本来就能由 (role, state) 从 Theme 解析出来，缓存两份快照是重复存储。
// 唯一丢失的信息是「被中断时的中间色」——用 reversing 把反向过渡变成进度回退来补偿：
//   hover(0) → 过渡中 → 又回到 normal：直接从当前进度往回走，不跳变。
// 三个以上状态快速交替（A→B→C，且 A→B 还在途中）时，from 取上一个目标状态 B，
// 会丢失 A→B 剩下的那点差值（≤ 一次过渡的残余），视觉上不可辨。
struct StyleSlot
{
    ControlState from = ControlState::Normal;
    ControlState to = ControlState::Normal;
    bool started = false;   // 第一次使用：直接吸附到目标，不做淡入
    bool reversing = false; // 反向过渡：progress 从当前值走向 0
    float progress = 1.0f;  // 0 = 完全在 from，1 = 完全在 to
};

// 控件状态过渡缓存（UI-38 分槽 / UI-50 索引化）。
//
// 不变量（UI-46 的教训）：同一控件同一帧内，一个 role 只能用一个 state——
// 槽按 role 索引、不区分 state，同帧内对同一 role 连续请求两种状态会互相覆盖。
// 需要两种外观时用两个角色（例如 Track 槽 + Thumb 滑块）。
//
// revision 是整组主题版本：变化时所有槽一起重建（换主题不做过渡）。
struct StyleTransition
{
    std::array<StyleSlot, kStyleRoleCount> slots{};
    std::uint64_t revision = 0;

    static_assert(static_cast<std::size_t>(StyleRole::Thumb) + 1 == kStyleRoleCount,
                  "kStyleRoleCount 必须等于 StyleRole 的枚举项数（新增角色时同步）");

    static constexpr std::size_t index_of(StyleRole role)
    {
        return static_cast<std::size_t>(role);
    }
    StyleSlot& slot(StyleRole role) { return slots[index_of(role)]; }
    const StyleSlot& slot(StyleRole role) const { return slots[index_of(role)]; }
    bool any_active() const
    {
        for (const auto& s : slots) {
            if (s.started && s.progress < 1.0f) return true;
        }
        return false;
    }
};
} // namespace xel::ui
