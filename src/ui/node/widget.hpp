#pragma once

// 入口别名。
//
//   Node   —— 整棵树唯一的元素类型：身份 / 几何 / 树 / 可见性 / 层叠 / 语义 /
//             布局契约（measure/arrange 默认即叶子行为）/ 4 个部件 / 测量缓存 / 脏标记
//   Widget —— Node 的**别名**（历史名称，400+ 调用点保留）
//
// 为什么是别名而不是派生类：一旦 Widget 变成子类，所有 `Node* → Widget*`
// 都会变成向下转型，而 `children()` 存的就是 Node。
#include "ui/node/node.hpp"

namespace xel::ui
{
using Widget = Node;
} // namespace xel::ui
