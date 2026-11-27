#pragma once

#include "ui/widget.hpp"

namespace xel::ui
{
// 叶子控件基类（UI-47）：把全仓重复最多的两个样板收进基类。
//
// 判据（见 docs/todo-ui.md 附 B）：
//   - arrange(Rect) 函数体只有 `set_geometry(slot);`
//   - measure(vec2) 函数体只有 `return clamp_size(preferred_size());`
//
// 不满足的不要继承它：把语义差异显式留在控件自己身上（例如 CollapsibleHeader 的
// 高度由标题决定、TextInput 的 arrange 还要夹滚动、FileBrowser 要把几何转给内部 Table）。
//
// 注意：这里只是「默认实现」，不改变布局契约——measure 仍然是
// 「constraints 向下、结果向上」，arrange 仍然只写几何、不触发脏标记。
class LeafNode : public Widget
{
public:
    // 首选尺寸（构造期确定，见 Widget::set_preferred_size）经 clamp 后即自然尺寸。
    glm::vec2 measure(glm::vec2 /*constraints*/) override { return clamp_size(preferred_size()); }

    // 叶子不吃约束协商：父容器给什么矩形就用什么矩形。
    void arrange(Rect slot) override { set_geometry(slot); }
};
} // namespace xel::ui
