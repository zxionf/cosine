# 示例（examples/）

示例是 `Application` 的最小外壳：**建树 + `app.run(每帧回调)`**，主循环只在
`Application::run()` 里存在一份。构建产物在 `build/examples/`。

```bash
cmake --build build -j
cd build/examples && ./ui_demo
```

> 对外演示与打包用的是产品本体 `build/src/cosine/cosine`（见 `packaging.md`），
> 示例只用于开发期验证具体子系统。

## 通用约定

### 所有示例共享的环境变量

| 变量 | 取值 | 影响 |
|---|---|---|
| `COSINE_ASSET_DIR` | 目录路径 | 覆盖 `assets/` 查找根；用于从任意 cwd 运行或指定外部资源 |
| `COSINE_CJK_FONT` | 字体文件路径 | 指定生成图集用字体；缺省时按系统字体列表探测，全部失败则回退 `assets/fonts/default8.png` |
| `COSINE_CJK_FONT_FACE` | 整数 | TTC/OTC 集合内的 face 索引；选错会导致字形错位或加载失败 |
| `COSINE_UI_SCALE` | `auto` 或 `0.25`–`4` | UI 逻辑像素相对 framebuffer 的缩放比。`auto` 跟随 `framebuffer/window` 比值；数值按倍数放大（2 = 界面整体放大两倍，字形按 `字号×倍数` 重新栅格化，不会糊）。默认 1.0 |
| `COSINE_EXIT_AFTER` | 秒（浮点） | 运行到该秒数后正常关窗退出（走析构，释放 Vulkan 资源）。由 `Application::run()` 统一处理，所有示例都生效 |
| `COSINE_EXPORT_PNG` | 输出路径 | 由示例自己实现（`app.save_png()`）：完成一次布局后离屏渲染并退出 |

冒烟建议（正常退出，不要用 `timeout` 强杀，否则会跳过析构）：

```bash
cd build/examples
COSINE_EXIT_AFTER=3 ./font_lab 2>&1 | grep -Ei 'VUID-|validation (error|warning)'
```

计数应为 0。

---

## ui_demo — UI 展示台（广度 + 深度）

分区式展示台：第一行菜单/开关，**第二行标签页（每区一个 tab）**，下方内容区 + 底部指标。
**注册表驱动**：新增内容只需在 `demo.cpp` 的注册表里加一条 `{id, title, summary, build}`，
外壳（顶栏/标签页/状态栏）不需要改。

```bash
./build/examples/ui_demo
COSINE_DEMO_SECTION=layout ./build/examples/ui_demo          # 直接打开某个分区
COSINE_EXPORT_PNG=shot.png COSINE_DEMO_SECTION=data ./build/examples/ui_demo
```

### 文件结构（可扩展点）

```text
examples/ui_demo/
├── main.cpp              装配：Application + demo::create() + app.run()
├── demo.hpp              分区/外壳类型、构建辅助（hint/row/group/gap/subtitle）
├── demo.cpp              外壳构建 + 分区注册表 + 主题/DPI/状态栏
└── sections/
    ├── sections.hpp      各分区 build 函数声明（扩展点 ①：加声明）
    ├── basics.cpp        概览 / 基础控件 / 输入控件 / 文本与字体
    ├── layout.cpp        布局 / 容器与视图
    ├── data.cpp          数据视图（Table / TreeView / FileBrowser）
    └── overlay.cpp       浮层与模态 / 媒体与时间
                          （扩展点 ②：实现 build_xxx；③：在 demo.cpp 注册）
```

分区是**懒构建**的：首次点开某个标签页才建树，之后靠页签切换，切回时保留各自的滚动位置、
输入内容与选中状态（页内没有可折叠分组）。

### 分区一览

| 分区 | 覆盖内容 |
|---|---|
| 概览 | 设计说明、操作方式表、运行指标 |
| 基础控件 | Button（含禁用）/ Checkbox / Radio + RadioGroup / Tag / ColorRect / Spacer / Progress（确定 + 不确定） |
| 输入控件 | TextInput（单行/多行）、DragFloat（范围/精度/后缀）、Slider、ColorInput（调色板）、Dropdown，均带联动回显 |
| 文本与字体 | Heading 1–3 / Paragraph 换行 / 三种对齐 / 单行省略 / 字号滑杆实时重栅格化 |
| 布局 | Vbox 主轴对齐四态、flex_grow 与百分比对比、aspect_ratio、Grid、Wrap、绝对定位 + z_index |
| 容器与视图 | TabView（多页 + 空页）、嵌套卡片（Panel，不可折叠）、Splitter（可拖分栏）、ScrollPanel（独立滚动） |
| 数据视图 | Table（弹性列/斑马纹/选中/滚动）、TreeView（展开/选中/键盘）、FileBrowser（只读目录） |
| 浮层与模态 | Window（拖动/8 向 resize/自带折叠按钮）、模态 Window、Dialog + show_dialog、下拉与 Tooltip 的层叠 |
| 媒体与时间 | Image 占位与 Fit 三态、PreviewRuler 拖动定位、色板随主题变化 |

### 顶栏能改什么（深度）

| 开关 | 作用 |
|---|---|
| 暗/亮 | `UIRoot::set_theme` 全树刷新（样式过渡按新 revision 重建） |
| 密度 | Compact / Normal / Spacious，字号与控件高度一起缩放 |
| 焦点 | `FocusPolicy`：Keyboard / Always / Never（用 Tab 观察焦点环差异） |
| 主题菜单 | 圆角开关、过渡开/关、UI 缩放 0.75×–1.5×（DPI 与字形重栅格化） |
| 视图菜单 | 直接跳转分区（带数字提示），与第二行标签页等价 |

### 状态栏

每帧刷新：当前分区 · fps · 帧耗时 · draw call · 顶点数 · 焦点控件名 · 悬停控件名 · 最近一次交互反馈。
名称取自 `Node::semantics()`，可直接看到语义投影的结构。

---

## font_lab — 字体实验室

字形与排版实验台：左侧一列控件，右侧大字号字符串预览。

```bash
./build/examples/font_lab
COSINE_EXPORT_PNG=font_lab.png ./build/examples/font_lab
```

### 内容分区

- 字号：`Slider` + `DragFloat`，范围 8–256，实时反映到预览。
- 字符串：`TextInput` 编辑预览文本；字符输入框改单个字形。
- 度量与调试：`COSINE_TEXT_DEBUG` 打开字形框 / 行框 / 基线 / advance；
  `COSINE_TEXT_SNAP=0` 关闭像素对齐以观察亚像素差异。
- 右侧 `ScrollPanel` 内是 `Label` 大字号预览，随字号与文本即时重排。

### 本示例专属环境变量

| 变量 | 取值 | 影响 |
|---|---|---|
| `COSINE_TEXT_DEBUG` | 任意非空 | 启动即打开字形框 + 行框 + 基线 + advance |
| `COSINE_TEXT_SNAP` | `0` / 其他 | `0` 关闭像素对齐，绘制原点保留小数；其他值（或缺省）保持对齐 |
| `COSINE_EXPORT_PNG` | 输出路径 | 离屏渲染为 PNG 并退出 |

---

## moss — 手写冒烟用例

最小手工验证程序（约 50 行）：一个 `Window` → `Panel` → `ScrollView` → `ColorRect` 的嵌套，
用来快速确认「构建 → 布局 → 绘制」链路和主题参数（圆角关闭、焦点环常显、2× 缩放）。

```bash
./build/examples/moss
COSINE_EXIT_AFTER=2 ./build/examples/moss
```

它不是验收对象，改动 UI 核心后手动跑一眼即可。

---

## 构建与冒烟速查

```bash
# 全部示例
cmake --build build -j

# 逐个冒烟（正常退出 + 校验错误计数）
for ex in ui_demo font_lab moss; do
  (cd build/examples && COSINE_EXIT_AFTER=2 ./$ex 2>&1 \
     | grep -Ei 'VUID-|validation (error|warning)' && echo "$ex: 有校验问题" || echo "$ex: ok")
done

# 高 DPI 下检查字形清晰度与命中位置
COSINE_UI_SCALE=2 COSINE_EXIT_AFTER=3 ./build/examples/font_lab

# 离屏导出（不需要窗口内容正确也能看出布局问题）
COSINE_EXPORT_PNG=/tmp/shot.png ./build/examples/font_lab
```
