# 项目文件格式（`.cosine`）

> 状态：**已定**（2026-10-27），实现见 M6.1。
> 本文是写入/读取实现的唯一依据；实现偏差必须先改本文。
> 相关：`src/anim/scene.hpp`、`src/anim/property.hpp`、`src/anim/timeline.hpp`、`src/anim/easing.hpp`。

## 1. 设计目标

- **确定性**：同一项目重复保存，字节完全一致（便于 diff、测试、缓存）。
- **可演进**：新增字段不破坏旧读取器；不兼容变更靠主版本号显式拒绝。
- **自校验**：损坏、截断、文本模式污染都能在读取早期被识别。
- **不存指针**：只存 id，读回时重建父子关系。

## 2. 文件头（16 字节）

| 偏移 | 大小 | 字段 | 值 / 说明 |
|---|---|---|---|
| 0 | 4 | magic | `'C' 'S' 'N' 'P'` = `43 53 4E 50`（cosine scene project） |
| 4 | 4 | guard | `0D 0A 1A 0A`（CR LF SUB LF） |
| 8 | 1 | major | 主版本，不兼容变更时 +1。**从 1 开始** |
| 9 | 1 | minor | 次版本，向后兼容新增时 +1 |
| 10 | 2 | flags | 小端 uint16，见 §2.1 |
| 12 | 4 | header_size | 小端 uint32，当前为 **16** |

### 2.1 flags 位定义

| 位 | 含义 | v1.0 取值 |
|---|---|---|
| 0 | 端序标记（0 = 小端） | 0 |
| 1 | 文件末尾存在 CRC32 | 0（保留，暂不写） |
| 2 | payload 经过压缩 | 0（保留） |
| 3–15 | 保留，必须为 0 | 0 |

读取器必须**忽略未知的保留位**，但若发现不认识的位被置 1，可给出警告。

### 2.2 guard 的作用

`0D 0A 1A 0A` 沿用 PNG 的思路：如果文件被 FTP / 邮件 / Windows 文本模式
把 `0A` 改写成 `0D 0A`，这 4 字节就对不上，读取器可以在解析前直接判定
「文件被文本模式损坏」，而不是解析到一半才报错。

### 2.3 头部十六进制示例

```text
43 53 4E 50  0D 0A 1A 0A  01 00 00 00  10 00 00 00
└─ CSNP ──┘  └─ guard ──┘ └ maj min ┘  └ header 16 ┘
              flags=0x0000 ┘
```

## 3. 端序与读写规则

- **固定小端**。不随平台变化。
- **逐字段显式读写**，禁止把结构体直接 `memcpy` 落盘：

```cpp
// 推荐
void write_u32(std::vector<uint8_t>& out, std::uint32_t v);
std::uint32_t read_u32(std::span<const std::uint8_t> in, std::size_t& offset);

// 禁止
out.insert(out.end(), reinterpret_cast<const uint8_t*>(&some_struct), ...);
```

原因：结构体有填充字节、`std::string`/`std::vector` 内是指针、端序与对齐因编译器而异。

- 浮点按 **IEEE-754 原样**写出（4 字节），不做量化。
- 字符串统一为 **UTF-8** 原样字节，长度用 `u32` 前缀；不写终止符。

## 4. 类型标签（`value_tag`）

`PropertyValue` 是 `std::variant<float, vec2, vec3, vec4, bool, string, uint32_t>`。
**不直接使用 `variant::index()`**（它是实现细节），改用显式且稳定的 tag：

| tag | 类型 | 负载 |
|---|---|---|
| 0 | `float` | 4 字节 |
| 1 | `glm::vec2` | 8 字节（2 × f32） |
| 2 | `glm::vec3` | 12 字节 |
| 3 | `glm::vec4` | 16 字节 |
| 4 | `bool` | 1 字节（0/1） |
| 5 | `std::string` | `u32` 长度 + UTF-8 字节 |
| 6 | `std::uint32_t` | 4 字节 |

建议加一条 `static_assert` 或用 switch 全覆盖，保证 tag 与 variant 分支同步演进。

## 5. payload 布局

顺序固定：`节点表 → 时间轴元数据 → 轨道表 → 结束标记 →（可选）CRC32`。

### 5.1 节点表

```text
node_count u32
每个节点:
  id            u64
  type          u8       0=Group 1=Shape 2=Text 3=Image 4=Camera
  flags         u8       bit0 visible(默认1)  bit1 ellipse
  position      f32 × 2
  rotation      f32       弧度
  scale         f32 × 2
  size          f32 × 2   Shape / Image
  color         f32 × 4
  text_size     f32
  texture_id    u32
  zoom          f32
  name          u32 长度 + UTF-8
  text          u32 长度 + UTF-8
  child_count   u32
  child_ids     u64 × child_count     ← 保留顺序
```

- **只存 `child_ids`，不存 `parent_id`**：读回时遍历一次把 `parent` 反推出来，
  避免两份关系不一致。根节点 = 没有被任何 `child_ids` 引用的节点。
- **`Scene` 的 root 也是一个普通节点**（id 最小、无父），会一并写出。
  读回时取「未被引用为 child 且 id 最小」的那个节点作为 root，
  这样「新节点自动挂到 root」的语义得以恢复。
- `children` 的**顺序即绘制顺序**，必须原样保留（`paint_order` 依赖它）。
- 所有字段**无条件写出**（即使该类型用不到），保证布局与版本一致、读取代码简单。

### 5.2 时间轴元数据

```text
duration    f32
fps         f32
loop        u8        0/1
time_scale  f32
```

### 5.3 轨道表

```text
track_count u32
每条轨道:
  node_id       u64
  property_id   u32       见 src/anim/property.hpp（1=Position … 10=TextureId）
  key_count     u32
  每个关键帧:
    time        f32
    easing      u8        0=Linear 1=EaseIn 2=EaseOut 3=EaseInOut 4=CubicBezier
    custom      f32 × 4   x1,y1,x2,y2（easing != CubicBezier 时写默认值）
    value_tag   u8        见 §4
    value       按 tag 的负载
```

### 5.4 结束标记与可选校验

```text
end_marker  u32 = 'E' 'N' 'D' '!'  → 字节 45 4E 44 21
crc32       u32（仅当 flags.bit1 = 1）  覆盖 header 起到 end_marker（含）的全部字节
```

`end_marker` 用于区分「文件正常结束」与「被截断但恰好前面对得上」。

## 6. 确定性要求

保存时按以下规则排序，保证同一项目产生相同字节：

| 内容 | 排序键 |
|---|---|
| 节点 | `id` 升序 |
| 轨道 | `(node_id, property_id)` 升序 |
| 关键帧 | `time` 升序；`time` 相同则保持插入顺序（稳定排序） |
| 字符串 | UTF-8 原样，不做规范化 |

`NodeId` 自增不复用、`PropertyId` 是编译期常量，两者都已经满足「稳定 id」前提。

## 7. 版本与兼容

- **major 不同 → 直接拒绝**：

  ```text
  error: unsupported project version 2.0 (this build supports 1.x)
  ```

- **minor 更高 → 默认拒绝**（避免静默丢字段），错误信息带文件版本与支持版本。
- **minor 更低 → 正常读取**：读取器必须能读所有 `minor <= 自身` 的文件。
- 读取器**不做「尽力解析」**：宁可报错，也不要写出损坏文件。

## 8. 错误处理

| 情况 | 提示 |
|---|---|
| 魔术头不匹配 | `not a cosine project (bad magic)` |
| guard 不匹配 | `project file was corrupted by text-mode transfer` |
| 版本不支持 | `unsupported project version X.Y (this build supports 1.x)` |
| 读到文件尾但仍缺字段 | `unexpected end of file at offset N` |
| 长度字段超限 | `length N exceeds limit`（单字符串上限 64 MB，节点数上限 1,000,000） |
| `child_ids` 自引用 / 成环 | `invalid hierarchy: cycle at node N` |
| `child_ids` 重复引用 | `invalid hierarchy: node N has multiple parents` |

## 9. 演进策略

- **不引入 chunk**：当前字段集稳定，版本号足以表达演进。
- **何时改成 chunk**：出现「可选大块资源」时（内嵌图片、图集缓存、脚本源码），
  再改为 `type(4) + size(4) + payload`，未知 chunk 直接跳过。
- **压缩**：需要时置 `flags.bit2`，压缩算法写入 payload 头部（`u8 codec`），
  读取器不支持该 codec 时报错。
- **CRC32**：v1.0 先不启用（`flags.bit1 = 0`），字段位置已预留，后续启用不需改版本。

## 10. 未定项

1. 是否把资源（图片、字体）内嵌进 `.cosine`，还是只存相对路径引用。
2. 压缩 codec 选型（zstd / lz4 / 无）。
3. 是否需要「增量保存 / 撤销记录落盘」。

以上三项在 M6.5（资源）与 M6.6（导出）之前给出决策，并追加到 `.design/DECISIONS.log`。
