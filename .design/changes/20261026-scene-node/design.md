# 20261026-scene-node

<!-- 为什么改、边界与不属于本次的范围 -->

## M4：场景图与节点模型

### 为什么

M4 的时间轴需要一棵「可被属性系统寻址」的节点树。UI 的 `Widget` 树是布局/事件树，
不适合承载动画语义（transform、可见性、按类型绘制）。M4 起分成两棵树：
`Widget` 负责交互与布局，`scene::Node` 负责渲染与时间，二者在主循环里各自独立。

### 设计

新增 `src/anim/scene.hpp` / `scene.cpp`（并入 `cosine_anim`），命名空间 `xel::scene`。

- `NodeId = std::uint64_t`，自增且**不复用**（序列化友好）；`kInvalidNode = 0`。
- `NodeType { Group, Shape, Text, Image, Camera }`。
- `Transform { position(vec2), rotation(弧度), scale(vec2) }`。
- `Node`：`id` / `parent` / `type` / `name` / `transform` / `visible` / `children`（`NodeId` 列表）
  + 类型专属字段（`size` / `color` / `ellipse`、`text` / `text_size`、`texture_id`、`zoom`）。
- `Scene`：
  - 构造即创建 `root`（Group，id=1）；
  - `create(type, name)` 自动挂到 root；
  - `destroy(id)` 递归删除子树，并从父的 `children` 移除；root 不可删；
  - `set_parent(child, parent)`：拒绝成环（parent 是 child 的祖先则拒绝），
    从旧父摘除再挂新父；
  - `for_each(fn)`：深度优先、先父后子（children 顺序）；
  - `nodes()` 暴露只读表（测试与序列化用）。

### 为什么不用继承

继承体系会让「按 id 找节点 → 属性读写」需要 `dynamic_cast` 或虚表；单层 `Node` + 类型枚举
让属性系统只面对一个结构体，序列化也更直接。

### 验证

新建 `test/test_anim_scene.cpp` 并注册：

- root 存在；`create` 后 `parent == root`，children 顺序与创建顺序一致。
- `find` / `size` / `name` / `type` / `Transform` 读写。
- `set_parent`：正常移动（旧父摘除、新父追加）；拒绝成环；拒绝不存在的节点；拒绝自身。
- `destroy`：递归删除整个子树、从父摘除、root 不可删、**id 不复用**。
- `for_each`：深度优先顺序正确；隐藏节点仍被遍历（可见性由调用方决定）。

### 边界（本次不做）

- 不做渲染、不做 transform 矩阵合成、不做命中测试（M4 后续 / M6）。
- 不做属性系统（下一个变更）。
- 不做序列化实现，只保证 id 稳定、结构可遍历。
