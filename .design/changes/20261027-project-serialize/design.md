# 20261027-project-serialize

<!-- 为什么改、边界与不属于本次的范围 -->

## M6.1：项目序列化与撤销栈

### 交付

| 文件 | 内容 |
|---|---|
| `src/anim/project.hpp` | `Project { Scene, Timeline }`（数据根对象） |
| `src/anim/serialize.hpp/.cpp` | `serialize` / `deserialize`，严格按 `docs/project_format.md` |
| `src/anim/command.hpp/.cpp` | `Command`、`CommandStack` + 5 个具体命令 |
| `src/anim/scene.*` | 新增 `insert` / `clear` / `set_root` |
| `src/anim/timeline.*` | 新增 `Track::remove_key`、`Timeline::ensure_track / remove_track / find_track` |
| `test/test_anim_serialize.cpp` | 8 组用例（round-trip、确定性、空项目、错误路径、层级重建） |
| `test/test_anim_command.cpp` | 8 组用例（属性、增删节点、关键帧、redo 清空、栈上限） |

命令：`SetPropertyCommand`、`AddNodeCommand`、`RemoveNodeCommand`、`AddKeyCommand`、`RemoveKeyCommand`。

### 关键决定与踩到的坑

1. **`Scene` 构造时自带 root 节点（id=1）**。
   直接 `insert` 文件节点会与它撞 id（`duplicate or invalid node id`）。
   修正：`deserialize` 先 `clear()`，全部 insert，再 `set_root("未被引用为 child 且 id 最小")`。
   `docs/project_format.md` §5.1 已补这一条。
2. **`Timeline::add_track` 无条件新建**。逐条 key 读回会产生多条同名轨道；
   读回改用 `ensure_track`。
3. **`PropertyValue` 用显式 tag**，不用 `variant::index()`（实现细节会变）。
4. **命令必须幂等**：`AddNodeCommand` 首次 apply 分配 id 并记住，redo 用同一 id；
   `RemoveNodeCommand` 首次 apply 拍子树快照，redo 直接 destroy，不再重拍。
5. **测试诊断要用 stderr**：`assert` 失败会 abort，stdout 重定向时是全缓冲，
   诊断信息会丢。

### 确定性

`serialize` 按「节点 id 升序、轨道 (node, property) 升序、关键帧 time 升序」输出；
测试用「读回再序列化，字节与首次完全相同」验证。

### 验证

- `test_anim_serialize`：
  - round-trip：root + 嵌套 Group + 三个轨道（position 带自定义 cubic-bezier、visible 阶跃、text 字符串）；
  - 确定性、空项目；
  - 错误路径：空输入、截断、坏魔术、guard 损坏、主版本不支持、次版本更新、缺 `END!`；
  - 手工拼字节构造：自引用成环、互相成环、多重父、引用不存在的子节点；
  - 合法层级重建（1→2→3）。
- `test_anim_command`：set property、add node（自动/指定 id）、remove 子树、关键帧增删、
  push 清 redo、栈上限丢弃最旧、空栈。
- `ctest` **53/53**（build 与 ASAN，新增 2 个用例集）。

### 边界（本次不做）

- 不做文件读写封装（`save(path)` / `load(path)`），只做内存字节 ↔ `Project`；落盘在 M6.4 接文件对话框时加。
- 不做 CRC32 与压缩（格式已预留 `flags` 位）。
- 撤销栈不含「选择状态」与「时间轴游标」等 UI 状态。
- 命令合并（连续拖动合成一条）留到 M6.3 时间轴编辑时按需做。
