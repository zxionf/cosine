#pragma once

#include "ui/widget_style.hpp"

#include <memory>

namespace xel::ui
{
// 样式过渡部件（UI-48）：把控件里那份「惰性分配的过渡缓存」挪进部件，
// 让 Widget 不必直接持有 StyleTransition 的实现细节。
//
// 本批（R2，UI-48）只做搬家，行为完全不变：
//   - 仍是首次 visual_style() 才分配；
//   - 仍按 (role, state) 分槽；
//   - 仍是控件终身持有。
//
// 下一批（R4，UI-50）会把 StyleSlot 从「存 from/to 两份 BoxStyle」改成
// 「存样式表索引 + 进度」（1688B → ≤128B），并去掉堆分配——那时变化被限制在这个文件里。
struct VisualState
{
    // const 的 append_to / visual_style 里需要惰性写入，所以是 mutable。
    mutable std::unique_ptr<StyleTransition> transition;
};
} // namespace xel::ui
