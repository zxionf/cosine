# 20261026-property-system

<!-- 为什么改、边界与不属于本次的范围 -->

## M4：属性系统（注册 + 类型擦除读写）

### 为什么

时间轴的每条 track 都是「(节点, 属性) → 关键帧」。需要一个统一入口，
让轨道不必知道节点字段的具体类型，也让序列化 / 脚本按名字查找属性。

### 设计

`src/anim/property.hpp` / `property.cpp`（并入 `cosine_anim`），命名空间 `xel::anim`：

- `PropertyId = uint32_t`；内置属性 id 是**编译期常量**（`prop::Position` … `prop::TextureId`），
  这样 `Scene::get/set_property` 可以用 `switch` 分发，不必查表。
- `PropertyValue = variant<float, vec2, vec3, vec4, bool, string, uint32_t>`。
- `PropertyRegistry`：`name ↔ id ↔ kind` 的双向映射，构造时预注册 10 个内置属性；
  提供 `id_of(name)` / `name_of(id)` / `kind_of(id)`。`builtin_properties()` 返回进程内单例。
- `to_string(PropertyKind)` 供调试与序列化。

`scene::Scene`：

- `std::optional<PropertyValue> get_property(NodeId, PropertyId) const`；
- `bool set_property(NodeId, PropertyId, const PropertyValue&)`；
- 节点不存在 / 属性未知 → `nullopt` / `false`；
- **类型不匹配 → `false`**（不修改字段），避免轨道写错类型时静默污染。

内置属性与字段对应：`position` / `rotation` / `scale` / `visible` / `size` / `color` /
`text` / `text_size` / `zoom` / `texture_id`。

### 验证

新建 `test/test_anim_property.cpp` 并注册：

- 注册表：名字 → id、id → 名字、类型查询；未知名字返回 `kInvalidProperty`。
- 十个内置属性逐一 `set` / `get`，并确认写回节点字段本身。
- 类型不匹配（vec2 属性传 float、float 属性传 vec2、string 属性传 float）均返回 `false`，
  且字段保持不变。
- 未知属性、不存在的节点 → `false` / `nullopt`。
- 默认值：position `(0,0)`、rotation `0`、visible `true`、text 空、texture_id `0`。

### 边界（本次不做）

- 不做「自定义属性 / 用户注册」（内置集合先够用；M5/M6 需要时再加扩展点）。
- 不做属性变更通知 / 脏标记（player 每帧直接写值）。
- 不做序列化实现（id 稳定 + 名字映射已就位）。
