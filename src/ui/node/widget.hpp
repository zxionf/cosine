#pragma once

// UI-56：三层分层的入口别名。
//
//   Node       —— 身份 / 几何 / 树 / 可见性 / 层叠 / 语义钩子（node.hpp）
//   LayoutNode —— 布局契约 + 4 个部件 + 测量缓存 + 脏标记（layout_node.hpp）
//   Widget     —— LayoutNode 的**别名**（历史名称，400+ 调用点保留）
//
// 为什么用别名而不是再包一层空类：`children()` 返回 `Node*` 视图，
// 若 Widget 是 LayoutNode 的派生类，所有 `Node* → Widget*` 都会变成向下转型，
// 需要在几十处加 static_cast；别名让 `Widget*` 与 `LayoutNode*` 同型，零改动。
#include "ui/node/layout_node.hpp"

namespace xel::ui
{
using Widget = LayoutNode;
} // namespace xel::ui
