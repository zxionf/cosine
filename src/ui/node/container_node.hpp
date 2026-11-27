#pragma once

#include "ui/node/widget.hpp"

namespace xel::ui
{
// 容器节点（UI-56，设计 §4.6）：设计里的第 3 类节点。
//
// 它不规定布局算法（Vbox/Grid/Wrap 各写各的 measure/arrange），只提供容器共用的能力：
//   - 子项只增改由 Node 提供（adopt / clear_children / replace_child / for_each_child）
//   - 「我是容器」可查询（is_container，非虚，由存储派生）
//   - add() 有默认实现（转发给 adopt），叶子仍然在 Node 里抛异常
//
// 为什么不做成"布局模板"：布局算法是每个容器自己的事，硬塞模板会把
// LinearLayout/Grid/Wrap 的差异挤进基类（那正是 UI-36 之前的老问题）。
class ContainerNode : public LayoutNode
{
public:
    void add(std::unique_ptr<Node> child) override { adopt(std::move(child)); }

    // 容器语义：由存储派生，不需要虚函数。
    bool is_container() const { return true; }
};
} // namespace xel::ui
