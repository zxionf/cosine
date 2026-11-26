#pragma once

#include "ui/style_types.hpp"
#include "ui/theme.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace xel::ui
{
// StyleRole 的槽位数（新增角色时同步这里；static_assert 会挡住漏改）。
inline constexpr std::size_t kStyleRoleCount = 11;

// 单个角色的过渡状态：只在 (role, state) 变化时重建，几何瞬时、颜色平滑。
struct StyleSlot
{
    ControlState state = ControlState::Normal;
    BoxStyle from{};
    BoxStyle to{};
    float progress = 1.0f; // 1 = 已完成
    bool started = false;
};

// 控件状态过渡缓存（UI-38）：
//
// 之前每个控件只有**一个**槽，于是 Slider 这种一帧内请求 Track + Accent + Tooltip
// 的控件会把上一个角色的当前色当成 from、并把 progress 归零 → 返回上一个角色的颜色，
// 表现为「滑轨被染成强调色」。现在按 StyleRole 分槽，各角色独立推进。
//
// revision 是整组主题版本：变化时所有槽一起重建（换主题不做过渡）。
struct StyleTransition
{
    std::array<StyleSlot, kStyleRoleCount> slots{};
    std::uint64_t revision = 0;

    static_assert(static_cast<std::size_t>(StyleRole::Knob) + 1 == kStyleRoleCount,
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
