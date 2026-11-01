#pragma once

#include "ui/style_types.hpp"
#include "ui/theme.hpp"

#include <cstdint>

namespace xel::ui
{
// 控件状态过渡缓存：只在 (role, state) 变化时重建，几何瞬时、颜色平滑。
// 定义在独立头里，避免 widget.hpp 为了一个私有成员把 theme/style 传给所有控件。
struct StyleTransition
{
    StyleRole role = StyleRole::Control;
    ControlState state = ControlState::Normal;
    BoxStyle from{};
    BoxStyle to{};
    float progress = 1.0f; // 1 = 已完成
    bool started = false;
    std::uint64_t revision = 0; // 建立时的主题版本
};
} // namespace xel::ui
