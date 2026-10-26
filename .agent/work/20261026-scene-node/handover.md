# 20261026-scene-node 交接

## 结果

- 新增 `src/anim/scene.hpp` / `scene.cpp`（并入 `cosine_anim`），命名空间 `xel::scene`。
- `NodeId = uint64_t` 自增不复用；`NodeType { Group, Shape, Text, Image, Camera }`；
  `Transform { position, rotation(弧度), scale }`。
- `Node` 单层结构：id / parent / type / name / transform / visible / children +
  类型专属字段（size / color / ellipse、text / text_size、texture_id、zoom）。
- `Scene`：root 自动创建；`create` 挂 root；`destroy` 递归删除并摘除；
  `set_parent` 防环；`for_each` 深度优先、children 顺序。
- 新增 `test/test_anim_scene.cpp`（7 个用例），注册为 `anim_scene`。

## 验证

- `ctest` 43/43（build 与 ASAN）。
- `sele close` 为 `appended`，SPEC 已有「M4：场景图与节点模型」，缓动小节未被覆盖。
- `DECISIONS #112`。

## 下一步

`property-system`：属性注册 + 类型擦除读写，是时间轴与轨道的地基。

建议范围：
- `PropertyId`（字符串名 → 稳定 id 的注册表）；
- `PropertyValue = variant<float, glm::vec2, glm::vec4, std::string, bool>`；
- `Scene::get_property(NodeId, PropertyId)` / `set_property(...)`，属性表按节点类型注册；
- 先覆盖 transform（position/rotation/scale）与常用绘制字段（size/color/text/text_size）。

## 设计提醒

- 节点类型用枚举而非继承；新增字段时同步更新属性注册表。
- id 不复用是序列化的前提，不要在 `destroy` 里回收 id。
