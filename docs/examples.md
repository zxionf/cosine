# 示例（examples/）

四个示例覆盖不同用途：文字排版、控件验收、最小动画、动画能力全景。
全部为 1200×800 窗口，构建产物在 `build/examples/`。

```bash
cmake --build build -j
cd build/examples && ./ui_gallery
```

## 通用约定

### 所有示例共享的环境变量

| 变量 | 取值 | 影响 |
|---|---|---|
| `COSINE_ASSET_DIR` | 目录路径 | 覆盖 `assets/` 查找根；用于从任意 cwd 运行或指定外部资源 |
| `COSINE_CJK_FONT` | 字体文件路径 | 指定生成图集用字体；缺省时按系统字体列表探测，全部失败则回退 `assets/fonts/default8.png` |
| `COSINE_CJK_FONT_FACE` | 整数 | TTC/OTC 集合内的 face 索引；选错会导致字形错位或加载失败 |
| `COSINE_UI_SCALE` | `auto` 或 `0.25`–`4` | UI 逻辑像素相对 framebuffer 的缩放比。`auto` 跟随 `framebuffer/window` 比值；数值按倍数放大（2 = 界面整体放大两倍，字形按 `字号×倍数` 重新栅格化，不会糊）。默认 1.0，等价于旧行为 |
| `COSINE_EXIT_AFTER` | 秒（浮点） | 运行到该秒数后正常关窗退出（走析构，释放 Vulkan 资源）。用于冒烟与截图；`ui_demo` 未接此变量 |

冒烟建议（正常退出，不要用 `timeout` 强杀，否则会跳过析构）：

```bash
cd build/examples
COSINE_EXIT_AFTER=3 ./ui_gallery 2>&1 | grep -Ei 'VUID-|validation (error|warning)'
```

计数应为 0。

---

## ui_demo — XEL Text Lab

文字排版实验台。用来观察对齐、换行、行距、降部、CJK 与像素对齐，
配合文字调试层看行框与基线的真实位置。

```bash
./build/examples/ui_demo
COSINE_TEXT_DEBUG=1 ./build/examples/ui_demo     # 调试层全开
```

### 内容分区

| 区域 | 作用 |
|---|---|
| Body / Caption 样本 | 正文与辅助字号的中英文混排基线 |
| Alignment | 左 / 居中 / 右对齐在同一行宽下的偏移表现 |
| Wrapping & line spacing | 中英混排换行；同段落 0px 与 6px 行距对照 |
| Descenders at large size | 96px `Hqyp gjpqy`，检查 g/j/p/q/y 的降部是否被行框或控件边界裁切 |
| CJK & dynamic glyphs | 中文排版、动态字形（生僻字首次出现时实时加载）、中英混排标点 |
| 信息面板 | Display 尺寸、DPI、字体名、行高参数、当前调试开关状态 |

### 按键

| 键 | 作用 |
|---|---|
| `F1` | 调试面板（FPS / 帧时间 / draw call / 顶点数 / hover-focus） |
| `F2` | 字形框（每个 glyph 的包围盒） |
| `F3` | 行框 + 基线（两者同时切换） |
| `F4` | advance 宽度标注 |
| `F5` | 像素对齐开关（关掉后可看到亚像素定位的差异） |

### 本示例专属环境变量

| 变量 | 取值 | 影响 |
|---|---|---|
| `COSINE_TEXT_DEBUG` | 任意非空 | 启动即打开字形框 + 行框 + 基线 + advance（等价于连按 F2–F4） |
| `COSINE_TEXT_SNAP` | `0` / 其他 | `0` 关闭像素对齐，绘制原点保留小数；其他值（或缺省）保持对齐 |

> `ui_demo` 没有接 `COSINE_EXIT_AFTER`，自动化脚本请用窗口关闭或 `timeout` 兜底。

---

## ui_gallery — XEL UI Gallery

全组件验收展厅：左侧 8 个分区导航，右侧内容区可滚动；
同时承担主题、焦点策略、过渡时长、圆角、滚动模式等样式/交互参数的目视与自动化验证。

```bash
./build/examples/ui_gallery
COSINE_GALLERY_SECTION=7 ./build/examples/ui_gallery     # 直接打开 Style 分区
```

### 分区索引

| 索引 | 名称 | 重点内容 |
|---|---|---|
| 0 | Overview | 能力概览、焦点顺序提示、overlay tooltip |
| 1 | Text | 字号档位、颜色、对齐、换行与行距 |
| 2 | Inputs | Button / Checkbox / Radio / Slider / TextInput / Dropdown |
| 3 | Containers | Panel / ScrollView / ScrollPanel / Splitter / Table / TreeView |
| 4 | Overlays | Tooltip / Dialog / 菜单弹层叠放 |
| 5 | Menu | MenuBar 与多级子菜单 |
| 6 | Layout | Grid 表单、Wrap 流式、百分比与宽高比、绝对定位与 z-index |
| 7 | Style | 主题切换、经典样式、焦点策略、过渡时长、圆角 |

### 按键

| 键 | 作用 |
|---|---|
| `F1` | 调试面板 |
| `Tab` / `Shift+Tab` | 按 Tab 顺序遍历焦点 |
| `Enter` / `Space` | 激活当前焦点控件 |

### 本示例专属环境变量

| 变量 | 取值 | 影响 |
|---|---|---|
| `COSINE_GALLERY_SECTION` | `0`–`7` | 启动即切到指定分区；越界忽略 |
| `COSINE_GALLERY_SCROLL` | 像素（浮点） | 与 `SECTION` 配合：首帧布局后把当前分区滚动到该偏移，用于覆盖"内容溢出视口"的路径 |
| `COSINE_GALLERY_SCROLL_MODE` | `smooth` / `fast` / `line` | 全部滚动控件（ScrollView/TreeView/Table）的滚动模式，并同步状态标签 |
| `COSINE_GALLERY_THEME` | `light` | 启动即切亮色主题；其他值保持暗色 |
| `COSINE_GALLERY_CLASSIC` | 任意非空 | 启用经典样式（方角、明确描边、Outline 焦点） |
| `COSINE_GALLERY_FOCUS` | `keyboard` / `always` / `never` | 焦点策略：仅键盘聚焦 / 始终显示焦点环 / 从不显示 |
| `COSINE_GALLERY_TRANSITION` | `off` 或毫秒数 | 状态过渡时长；`off` = 0，其他值按 `atof` 解析（常用 60/120/240） |
| `COSINE_GALLERY_RADIUS` | 浮点 | 圆角缩放系数，`0` 为直角，`2` 为两倍圆角 |
| `COSINE_GALLERY_MENU` | 一级菜单索引 | 启动即展开该菜单（多级子菜单用鼠标/键盘继续下钻） |
| `COSINE_GALLERY_DROPDOWN` | 任意非空 | 首帧打开一次下拉，用于截图下拉展开态 |
| `COSINE_GALLERY_DIALOG` | 任意非空 | 启动即打开 Dialog |

---

## ui_anim_demo — XEL Animation Demo

最小动画内核演示：`Scene` + `Timeline` + `Player` 驱动，
用同一区间、不同缓动的多条 position 轨道做对比，附一条静止 guide 线。

```bash
./build/examples/ui_anim_demo
COSINE_EXIT_AFTER=2 ./build/examples/ui_anim_demo
```

### 构成

- `SceneView`（720×300）：五条 `position` 轨道（Linear / EaseIn / EaseOut / EaseInOut / cubic-bezier）
  与一条 guide 线，直观对比曲线差异。
- 控制行：`Pause`/`Play` 切换、`Reset`（`Player::seek(0)`）。
- `Progress` 与时间标签：读 `Player::time()`。

### 交互

只有界面按钮，无键盘快捷键。

### 本示例专属环境变量

| 变量 | 取值 | 影响 |
|---|---|---|
| `COSINE_EXIT_AFTER` | 秒 | 到点正常退出；用于冒烟与截图 |

---

## ui_anim_showcase — XEL Animation Showcase

M4 能力全景：三个展区分别演示缓动对比、父子变换累积、属性动画，
外加播放控制与时间点事件。作为 M4.5 的验收与对外演示。

```bash
./build/examples/ui_anim_showcase
COSINE_EXIT_AFTER=3 ./build/examples/ui_anim_showcase
```

### 展区

| 展区 | 内容 |
|---|---|
| A. Easing | 五条 6s `position` 轨道：Linear / EaseIn / EaseOut / EaseInOut / 自定义 cubic-bezier |
| B. Hierarchy | 一个 `Group` 做 scale 脉冲（1.0→1.4→1.0），三个子方块以相对坐标跟随，验证父变换累积 |
| C. Properties | 同一方块的 `position`（往返）+ `scale`（横向拉伸）+ `color`（红↔蓝）；`text` 阶跃（phase 0→3）；`visible` 每秒闪烁 |

### 控制行

| 控件 | 行为 |
|---|---|
| `Pause` / `Play` | `Player::toggle()`，按钮文字随状态切换 |
| `Back 1s` / `Fwd 1s` | `Player::seek(time ∓ 1)`，夹在 `[0, duration]` |
| `Reset` | `seek(0)` |
| `Speed 1x` | 在 0.5x → 1x → 2x 之间轮换（`Timeline::set_time_scale`） |
| `Loop: On` | 切换循环（`Timeline::set_loop`） |

底部另有 `Progress`、时间标签与事件标签：2.0s / 4.0s 的时间点事件会改写事件标签。

### 本示例专属环境变量

| 变量 | 取值 | 影响 |
|---|---|---|
| `COSINE_EXIT_AFTER` | 秒 | 到点正常退出；用于冒烟与截图 |
| `COSINE_EXPORT_PNG` | 输出路径 | 完成一次布局后，把整棵 UI 树离屏渲染为 1200×800 的 PNG 并退出；用于导出与回归（M6.0） |

---

## 构建与冒烟速查

```bash
# 全部示例
cmake --build build -j

# 逐个冒烟（正常退出 + 校验错误计数）
for ex in ui_gallery ui_anim_demo ui_anim_showcase; do
  (cd build/examples && COSINE_EXIT_AFTER=2 ./$ex 2>&1 \
     | grep -Ei 'VUID-|validation (error|warning)' && echo "$ex: 有校验问题" || echo "$ex: ok")
done

# 高 DPI 下检查 tooltip 跟随与字形清晰度
COSINE_UI_SCALE=2 ./build/examples/ui_gallery
```
