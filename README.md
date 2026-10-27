# cosine animation engine

cosine 是一个用 C++20 + Vulkan 编写的 2D GUI 与动画引擎。
控件、布局、文本与绘制批处理全部自带实现，不依赖第三方 UI 库。

XEL 是 cosine 的脚本语言，用于描述动画与场景；解释器目前在 `interpreter/`，
后续会作为可嵌入库接入动画内核（M5）。

## 当前状态

版本 `0.4.0`。已完成 M0–M4（含 M3.5 组件与样式、M3.6 滚动体验、M4 2D 动画内核、M4.5 动画展厅），
M4.6 文本/图集打磨进行中，下一步是 M5 XEL 绑定。路线见 `.design/阶段计划.md`。

## 能力

### GUI 框架

- Retained-mode Widget 树：measure/arrange、padding/margin/min/max、flex grow、主轴分布。
- 布局容器：`Vbox` / `Hbox` / `Grid`（固定列 + 弹性列）/ `Wrap`（流式折行）。
- 尺寸：百分比尺寸、宽高比、绝对定位 + `z-index`（绘制升序、命中降序）。
- 控件：`Panel`、`Label`、`Button`、`Checkbox`、`Radio`/`RadioGroup`、`Slider`、`Progress`、
  `TextInput`（多行）、`ScrollView`/`ScrollPanel`、`TabBar`/`TabView`、`Splitter`、`Tooltip`、
  `Dropdown`、`Dialog`、`MenuBar`（多级子菜单）、`Table`、`TreeView`、`Image`（占位）、`SceneView`。
- 样式：暗/亮主题、现代与经典两种风格、状态过渡（0/60/120/240 ms）、
  焦点策略（Keyboard/Always/Never）、几何九宫格圆角（可缩放）。
- 交互：命中路径与冒泡消费、鼠标捕获、右键/双击/长按、键盘 Tab 遍历与 roving tabindex、拖拽反馈。
- 滚动：`Smooth`（指数趋近）/ `Fast`（停止输入后加速收尾）/ `Line`（离散步进）三模式。

### 文本

- UTF-8 与常用 CJK；FreeType 运行时生成图集，生僻字按需动态加载。
- 字形按绘制字号逐档栅格化（`4..256`），不放大基准图集；图集空间不足可重试并可用 `used_ratio()` 监控。
- 行高保证覆盖 `ascender + |descender|`，降部不再被行框裁切。
- 换行、省略号、`max_lines`、行距、左/中/右对齐、像素对齐；`Label` 测量结果带缓存。
- 新增字形只上传图集的脏矩形，不再整张重传。
- DPI 缩放：`COSINE_UI_SCALE` 让 UI 按逻辑像素布局，字形按 `字号 × 比例` 栅格化。

### 动画内核

- 缓动：`Linear` / `EaseIn` / `EaseOut` / `EaseInOut` / 自定义 `CubicBezier`。
- 场景图：`NodeId` 自增、`Group`/`Shape`/`Text`/`Image`/`Camera` 节点、父子变换累积。
- 属性系统：名称 ↔ id ↔ 类型的注册表，`variant` 类型擦除读写。
- 时间轴：轨道 + 关键帧（有序插入、两端外推、阶跃类型）、循环、倍速、seek。
- 播放器：`tick(dt)` 推进、时间点事件（`(before, after]`，循环回绕重新生效）。
- `SceneView`：把场景渲染成 DrawList，作为普通 Widget 参与布局。

### 渲染与工程化

- DrawList 批处理、scissor 裁剪栈（两端对齐像素网格）、overlay 图层、`RenderTarget` 抽象。
- 帧统计（frame time / draw call / 顶点与索引数）、`DebugOverlay`、F11 截图。
- 静态库分层（`cosine_ui` / `cosine_anim` / `cosine_text` / backend）、51 个 CTest 用例、ASAN 构建、
  lavapipe 冒烟、Linux/Windows 打包脚本。

## 示例

| 示例 | 用途 |
|---|---|
| `examples/ui_demo` | XEL Text Lab：文字排版实验台（对齐、换行、行距、降部、CJK、文字调试层） |
| `examples/ui_gallery` | XEL UI Gallery：8 分区展示全部控件，可切主题/焦点策略/过渡/圆角/滚动模式 |
| `examples/ui_anim_demo` | XEL Animation Demo：最小动画演示（Scene + Timeline + Player） |
| `examples/ui_anim_showcase` | XEL Animation Showcase：缓动 / 层级 / 属性动画全景 + 播放控制与事件 |

每个示例的内容、按键与专属环境变量见 `docs/examples.md`。

```bash
cmake --build build -j
./build/examples/ui_gallery
COSINE_GALLERY_SECTION=7 ./build/examples/ui_gallery   # 直接打开 Style 分区
```

## 构建

需要 CMake ≥ 4.0、C++20 编译器、Vulkan SDK 与 Slang（`slangc`）。

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
ctest --test-dir build --output-on-failure
```

或者用脚本：

```bash
./scripts/ci-linux.sh                                  # 构建 + 测试
COSINE_SANITIZE=address COSINE_BUILD_DIR=build-asan ./scripts/ci-linux.sh   # ASAN
./scripts/smoke-lavapipe.sh                            # lavapipe 软件驱动冒烟
```

`slangc` 不在 PATH 时：

```bash
cmake -S . -B build -DSLANGC_EXECUTABLE=/path/to/slangc
```

## 冒烟

```bash
cd build/examples
COSINE_EXIT_AFTER=3 ./ui_gallery 2>&1 | grep -Ei 'VUID-|validation (error|warning)'
```

计数为 0 即通过。`COSINE_EXIT_AFTER` 走正常退出路径（覆盖析构与资源释放），
不要用 `timeout` 强杀，否则会跳过析构。

## 字体与资源

默认用系统 CJK 字体生成图集；全部探测失败时回退 `assets/fonts/default8.png`。

```bash
COSINE_CJK_FONT=/usr/share/fonts/noto-cjk/NotoSansCJK-Regular.ttc \
COSINE_CJK_FONT_FACE=2 ./build/examples/ui_demo
```

`assets/` 查找顺序：`COSINE_ASSET_DIR` → 可执行文件旁的 `assets/` → 构建期注入目录。

## 打包

```bash
./scripts/package-linux.sh     # dist/cosine-<version>-linux-<arch>.tar.gz
./scripts/package-windows.ps1  # dist/cosine-<version>-windows-x64.zip
```

## 目录结构

```text
src/
├── anim/             缓动、场景图、属性、时间轴、播放器（只依赖 glm）
├── backend/          窗口、Vulkan 上下文、swapchain、renderer、资源路径
├── events/           InputEvent、KeyCode/Modifier
├── graphics/         DrawList/Drawable、图元、FontAtlas、FrameStats
├── ui/               Widget 树、布局、控件、UISystem、UIContext、SceneView、DebugOverlay
└── xel_application.* 窗口 + Vulkan + UI 的装配与事件桥接

examples/             ui_demo / ui_gallery / ui_anim_demo / ui_anim_showcase
interpreter/          XEL 解释器（与 UI 解耦，M5 起作为可嵌入库）
test/                 CTest 用例
scripts/              CI、打包、冒烟脚本
assets/               shader、字体与图集
```

## 文档

入口：`docs/index.md`。

- 示例与各自环境变量：`docs/examples.md`
- 构建与 CI：`docs/ci.md`
- 打包与字体策略：`docs/packaging.md`
- 依赖版本：`docs/dependencies.md`
- 字体图集与文本渲染：`docs/font_atlas.md`
- 历史与已完成基线：`docs/history.md`
- 现状评估（历史归档）：`docs/project_assessment.md`

## 已知限制

- Windows 构建脚本已就绪，但尚未在真实 Windows 环境完整实测。
- 无 IME、无文字 shaping（BiDi、复杂字形），阿拉伯/印度语系暂不支持。
- 图集为位图，未做 SDF/MSDF；超大字号大量 CJK 仍可能撑满 2048² 图集。
- `SceneView` 暂不支持 `rotation`；`Image` 为占位实现，不采样真实纹理。
- 无序列化/项目文件格式，编辑器尚未开始（M6）。
- 单窗口、单 Renderer；运行中切换 DPI 需重启。
- 无安装器、代码签名与自动更新。

## 路线

M5 XEL 绑定 → M6 2D 时间轴编辑器与导出 → M7 2.5D/3D 扩展 → M8 发布。
详见 `.design/阶段计划.md`。