# 20261026-property-system 交接

## 结果

- `src/anim/property.hpp/.cpp`（并入 `cosine_anim`）：
  - `PropertyId` / `PropertyValue = variant<float,vec2,vec3,vec4,bool,string,uint32_t>`；
  - 内置 id 为编译期常量 `prop::Position … prop::TextureId`（共 10 个）；
  - `PropertyRegistry`：名字 ↔ id ↔ 类型，`builtin_properties()` 单例；
  - `to_string(PropertyKind)`。
- `scene::Scene::get_property` / `set_property`：
  - `switch` 按 id 分发到节点字段；
  - 节点/属性不存在 → `nullopt` / `false`；
  - **类型不匹配 → `false`，字段不变**。
- 新增 `test/test_anim_property.cpp`（4 个用例），注册为 `anim_property`。

## 验证

- `ctest` 44/44（build 与 ASAN）。
- `sele close` 为 `appended`（`DECISIONS #116`）。

## 下一步（M4）

`timeline` + `track` + `keyframe`：

- `Timeline { duration, fps, loop, time_scale }`，`play/pause/seek`；
- `Track { NodeId, PropertyId, vector<Keyframe{ time, PropertyValue, Easing }> }`；
- 采样：时间 → 二分找相邻关键帧 → `ease()` 插值 → `Scene::set_property`。
- 之后是 `player`（确定性 seek）与 `events`。

## 注意

- 新增属性要同时改三处：`prop::*` 常量、`PropertyRegistry` 构造、`Scene` 的 `switch`。
- `get_property` 返回 `optional`，调用方要处理空值（未知属性）。
