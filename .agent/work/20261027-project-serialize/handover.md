# 20261027-project-serialize 交接

## 结果

| 文件 | 内容 |
|---|---|
| `src/anim/project.hpp` | `Project { Scene, Timeline }` |
| `src/anim/serialize.hpp/.cpp` | `serialize(const Project&)` / `deserialize(span, Project&, string* error)` |
| `src/anim/command.hpp/.cpp` | `Command`、`CommandStack`、5 个命令 |
| `src/anim/scene.*` | `insert` / `clear` / `set_root` |
| `src/anim/timeline.*` | `Track::remove_key`、`ensure_track` / `remove_track` / `find_track` |

命令：`SetPropertyCommand`、`AddNodeCommand`、`RemoveNodeCommand`、`AddKeyCommand`、`RemoveKeyCommand`。

## 五个坑（都实际踩过）

1. **`Scene` 构造自带 root（id=1）**：`deserialize` 直接 insert 会撞 id。
   修法：先 `clear()` → 全部 insert → `set_root(无父且 id 最小)`。
   `docs/project_format.md` §5.1 已补这个事实。
2. **`Timeline::add_track` 无条件新建**：逐条 key 读回会产生多条同名轨道，改用 `ensure_track`。
3. **`PropertyValue` 必须用显式 tag**，不要 `variant::index()`。
4. **命令要幂等**：`AddNodeCommand` 记住首次分配的 id；`RemoveNodeCommand` 首次拍子树快照，
   redo 时不再重拍。
5. **测试诊断用 stderr**：`assert` abort 前 stdout 是全缓冲，`printf` 会丢。

## 验证

- `test_anim_serialize`（8 组）：round-trip（root + 嵌套 Group + 三轨道 + 自定义贝塞尔 + 阶跃 + 字符串）、
  确定性（读回再序列化字节相同）、空项目、7 类错误路径、手工构造成环/多重父/未知子、层级重建。
- `test_anim_command`（8 组）：属性、增删节点（自动/指定 id）、删除子树并恢复、关键帧增删、
  push 清 redo、栈上限、空栈。
- `ctest` **53/53**（build 与 ASAN）。
- `sele close` 为 `appended`（`DECISIONS #130`）；阶段计划 M6.1 勾选，里程碑表标 ✅。

## 格式落点

`docs/project_format.md`：16 字节头（`CSNP` + guard + major.minor + flags + header_size）、
固定小端、节点表 + 时间轴元数据 + 轨道表 + `END!`、确定性排序、错误信息表。

## 下一步：M6.2 属性编辑控件与 2D 旋转

- `DragFloat` / `SpinBox`（float 与 vec2/vec3/vec4 分量），色板 + 十六进制颜色输入。
- `SceneView` 支持 2D `rotation`：顶点绕节点中心旋转，不引入 mat4 管线。
- 验收：gallery 增「属性编辑」分区；旋转在 `ui_anim_showcase` 可见。
- 控件改动会影响 `docs/examples.md` 与 gallery 分区表，记得同步。
