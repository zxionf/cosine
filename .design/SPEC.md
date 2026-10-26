# Spec

<!-- 当前设计真相源：一个概念一节，只写现状。历史见 git，取舍理由见 DECISIONS.log。 -->

## 项目定位
- 近期（MVP）：通用 2D GUI 框架。自研 retained-mode UI，Vulkan 渲染，Linux Wayland + Windows 双平台。
- 中期：脚本驱动的程序化动画引擎。XEL 首版内嵌，最终成为主要创作语言。
- 中期：时间轴关键帧动画编辑器，与脚本共用同一套场景/时间轴内核。
- 中期：2D 动画导出（离屏渲染、帧序列、视频编码）。
- 长期：2.5D / 3D 变换、完整创作工具链。
- 首版非目标：3D 网格/光照/物理、音频/视频解码、插件系统、多人协作、完整 IDE、即时模式 GUI。

## 技术栈
- 语言：C++20。
- 构建：CMake；backend 收敛为静态库，示例独立。
- 图形：Vulkan（vulkan-hpp RAII），仅 Vulkan 后端；当前渲染 2D。
- 窗口/输入：GLFW，Linux Wayland + Windows，X11 作为可选项。
- 数学：GLM，由 CMake 显式声明。
- 资源：STB（固定版本）、FreeType；字体图集纳入资源管线。
- Shader：Slang 为主，glslc 作为简单 shader 的回退。
- UI：自研 retained-mode Widget 树，不引入第三方 UI 库。
- 脚本：XEL 解释器（token/lexer/parser/AST/evaluator/environment）。

## 约束
- Vulkan-only；不保留 OpenGL 渲染路径。
- 当前只实现 2D。先完成 2D GUI → 2D 动画内核 → XEL 绑定 → 2D 编辑器与导出，再进入 2.5D/3D；变换/资源/渲染接口不得写死 2D 假设。
- Linux Wayland 与 Windows 是一级目标；构建与资源定位不能依赖 Linux 路径或当前工作目录。
- 资源统一放 `assets/`，运行时通过可执行文件路径或 `COSINE_ASSET_DIR` 定位。
- 依赖保持最小集；新增依赖必须先记录决策。STB 固定 commit；不提前引入 VMA。
- 视频导出优先使用外部编码器管道（如 ffmpeg），避免核心直接耦合编解码库。
- Debug 构建必须能通过 Vulkan validation；资源/初始化失败必须给出明确错误。
- 每个阶段按可运行、可验证的出口标准验收；不设硬性性能指标，先做 instrumentation。

## 分层与目录结构
模块职责：

- `core/`：场景图、属性、时间轴、关键帧、缓动、序列化。
- `renderer/`：Vulkan 后端、资源、draw list、批次、裁剪、离屏渲染。
- `ui/`：Widget、布局、事件、焦点、控件、主题。
- `app/`：Application、窗口、输入、资源定位。
- `interpreter/`：XEL 解释器与宿主 API。
- `examples/`：可运行示例；`tests/`：自动化测试；`assets/`：字体、纹理、shader。

当前实现位于 `src/backend/`（`cosine_backend` 静态库：窗口、Vulkan、资源路径）、`src/graphics/`、`src/ui/`、`src/shaders/`，由 `src/xel_application.*` 组合；`assets/` 存放运行时资源，`test/` 存放 CTest，`scripts/` 存放 CI 脚本。模块边界按上述职责继续收敛。

## 实现约定
- 渲染批次携带纹理槽位（`WHITE` / `TEXTURE`）与显式初始化的 scissor；`Rectangle` / `Triangle` 使用 `uint32_t` 索引基址。
- GLFW user pointer 统一为 `Application*`；resize 经 `Application` 转发到 `Window`；`UISystem` 支持鼠标按下/抬起/点击与 `pressed_` 生命周期。
- `Widget` 使用非拥有 `parent` 指针；`add()` 设置 parent；布局脏标记沿 parent 链传播；`UIRoot` 是自由画布。
- `ScrollView` 用 owning children 暴露内容给命中测试，`set_content` 不再显式析构，滚轮方向与 offset clamp 明确。
- backend 收敛为 `cosine_backend` 静态库；资源统一放 `assets/`，运行时按 `COSINE_ASSET_DIR` → 可执行文件旁 `assets/` → 构建期注入目录解析。
- GLM 由 CMake 显式声明；STB 固定版本；slangc/glslc 使用 `find_program` 并在缺失时 fail fast；Wayland/X11 为 CMake option。
- 测试使用 CTest + 简单 assert；布局测试不依赖 Vulkan；`test_ui_layout` 与 3 个解释器测试注册为 ctest。

## 事件模型
- 输入事件坐标为窗口/framebuffer 坐标；控件通过 `global_bounds` 与父链换算。
- 命中返回从根到目标的路径；事件从最深目标向上冒泡，处理方法返回 `bool` 表示是否消费。
- 鼠标按下捕获 `pressed_`，移动与抬起优先发给捕获目标；抬起时若仍命中同一目标则触发 `click`。
- 焦点由 `focused_` 管理；Tab / Shift+Tab 在可聚焦控件间循环；键盘与字符事件从焦点控件向上冒泡。
- 容器通过 `clips_children()` 约束命中范围；滚动内容超出 viewport 不可点。

## 布局模型
- `measure(constraints)` 只测量，`arrange(slot)` 只定位；容器在尺寸变化或 `layout_dirty` 时重排。
- 提供 Vbox、Hbox、Spacer；padding/margin、spacing、交叉轴对齐、主轴分布统一由容器处理。
- 控件支持固定尺寸、stretch、grow/flex；文本测量接入布局，支持换行与行高。
- ScrollView 支持纵向、横向、双轴，滚动后 clamp offset；resize 后重新 clamp。

## 控件与主题
- MVP 控件：Panel、Label、Button、Checkbox/Toggle、Slider、TextInput、ScrollView/List、Splitter、Tooltip。
- TextInput 首版单行；多行输入留到后续。
- 控件状态与主题解耦：颜色、圆角、边框、间距、字体大小集中在 `Theme`。
- 首版提供一套深色主题；结构允许后续增加浅色主题。

## 文本
- `Label::set_text_align(TextAlign)`：Left / Center / Right，按控件宽度对齐整段文本。
- `Label::set_pixel_snap(bool)`：开启时把绘制原点对齐到整数像素，避免线性过滤导致的模糊（默认开启）。
- `Label::set_line_spacing(float)`：行间额外间距，用于多行排版（仅影响换行测量与绘制）。
- 以上选项不改变 `measure()` 的语义（除行距外），保证布局可预测。

## 渲染与裁剪
- 统一 draw list：批次记录 vertex/index offset、count、scissor、texture。
- 容器 push/pop scissor，渲染层合并裁剪区域；按 texture/scissor 合批。
- 明确图层与 z-order，弹窗置顶；保留 `WHITE` 纹理路径。
- 为后续离屏渲染预留 render target 抽象，但 M2 不实现离屏管线。

## UIContext
- `UIContext` 持有 `Theme` 与 `graphics::FontAtlas*`（非拥有），作为 UI 树的资源入口。
- `UIRoot` 拥有一个 `UIContext`；子控件通过 `Widget::context()` 沿 parent 链查找，不再直接读取全局 `theme()` / `active_font_atlas()`。
- `Widget::ui_theme()` / `Widget::font_atlas()` 优先使用 context；无 context 时保留默认 theme 和 active atlas 兼容路径，保证 standalone 测试与旧代码可迁移。
- 本次只迁移 UI 控件；`Line`/`Rectangle`/`Triangle` 的白像素 UV 仍走 active atlas，留到后续 graphics context 处理。

## 上下文传播
- `Widget::set_context()` 递归设置自身、children 与 overlays。
- `UIRoot::add/add_overlay`、`Vbox/Hbox::add`、`ScrollView::set_content` 在挂接子项时传播 context。
- `Panel` 在 context 变化时刷新 theme 的 padding/spacing；其余控件在 measure/append 时读取 `ui_theme()`。
- `Application` 在构造后把 `Renderer` 的 FontAtlas 绑定到 `UISystem` root 的 `UIContext`。
- `UISystem::context()` 暴露 root context，便于应用层配置字体/主题。

## 兼容策略
- 保留全局 `theme()` / `set_active_font_atlas()` 作为无 context 时的 fallback，不在本次删除。
- 新增 `test_ui_context` 覆盖 context 传播、字体绑定、嵌套 Panel 主题刷新。
- 现有 `test_ui_*` 与 `test_text` 不改行为，作为兼容性回归。

## KeyCode 与 Modifier
- UI 层不直接依赖 GLFW 键码；`events/key_code.hpp` 定义稳定的 `KeyCode` 与 `Modifier` 位标志。
- `InputEvent` 的 `key/mods` 改为 `KeyCode/Modifier`；GLFW 键码到 `KeyCode` 的映射只存在于应用层 `xel_application.cpp`。
- 首版只覆盖 UI 需要的键：Tab、Enter/KeypadEnter、Space、Backspace、Delete、方向键、Home/End、A。
- `Modifier` 用位标志支持 Shift/Ctrl/Alt/Super 组合。

## 头文件边界
- Table 不支持列宽拖拽、列排序、多选、单元格编辑、横向滚动。

## TreeView
- `TreeView` 是独立控件（不是 `Table` 子类）：`TreeNode { title, children, expanded }` + `set_roots` / `add_root` / `clear_roots`。
- 选中为根索引路径 `std::vector<int>`；`selected_path` / `set_selected_path` / `selected_node` / `set_on_select(path)`。
- 可见行是展开节点的前序遍历；绘制、命中与键盘共用 `visible_rows()`。
- 绘制：面板背景、行高 26、缩进 16、chevron（展开下指/折叠右指）、选中/hover/斑马纹、自绘滚动条、整控件裁剪。
- 交互：点击行选中、点击箭头切换展开；`Up/Down/Home/End` 移动，`Right` 展开或进入第一个子节点，`Left` 折叠或回到父级，`Enter/Space` 切换；滚轮 1.5 行，选中自动保持可见。
- 折叠父级时子树内的选中上移到被折叠节点；`set_roots` / `collapse_all` 后选中截断到仍可见的最长前缀。
- 边界：无多选、拖拽、内联重命名、懒加载、图标/复选标记。

## 滚动目标保持（M3.6 修复）
### 现象

Table 与 TreeView 的滚轮滚动完全无效，而 ScrollView 正常。
gallery 缺少 Smooth / Line 的运行时切换（只能靠环境变量）。

### 根因

`Table::arrange` / `TreeView::arrange` 里写的是：

```cpp
scroll_ = clamp_scroll(scroll_);
scroll_target_ = scroll_;   // ← 每帧把滚轮目标清零
```

layout 每帧都会跑（子控件 `arrange` 里的 `set_size` 触发 `mark_layout_dirty`），
于是滚轮刚设置的 `scroll_target_` 在下一帧布局时被清零，`on_update` 看不到差值，
位置永远停在 0。`ScrollView::arrange` 用的是 `target_ = clamp_offset(target_)`（保留目标），
所以只有它正常。

调试证据：dispatch 后 `table.target=126`，下一帧打印 `target=0`。

### 修复

- `Table::arrange` / `TreeView::arrange` 改为 `scroll_target_ = clamp_scroll(scroll_target_);`
  —— 保留目标，只在可滚范围变化（内容变短）时收紧。
- `set_scroll_value()` 继续用于「程序化跳转 / 键盘 / 结构变化（set_roots、collapse）」：
  立即设置位置并同步目标。

### gallery

- `UiGallery::set_scroll_mode(ScrollMode)`：统一切换所有分区 `ScrollPanel`、
  示例 `ScrollView`（`scroll_vertical` / `scroll_both`）、`Table`、`TreeView`。
- Style 分区新增 **Smooth / Line** 两个按钮 + `scroll: smooth|line` 状态标签。
- `COSINE_GALLERY_SCROLL_MODE=line|smooth` 改为走同一入口，保持行为一致。

### 验证

- 复现程序：`on_scroll` → `arrange` → `update`，`target` 保持 126、`offset` 推进到 29.5。
- `ctest` 41/41（build 与 ASAN）。
- gallery 冒烟：Style、Containers、Containers(line) 三组 `validation=0`。
- 截图：Style 分区可见 Smooth / Line 按钮与 `scroll: smooth` 状态。

### 边界

- 不改滚轮步长、平滑参数与模式语义（沿用 M3.6 的定义）。
- 不做「每个容器独立的模式 UI」——gallery 只提供一个全局切换入口。

## Tooltip
- `Tooltip` 是 overlay 控件，由 `update_hover(dt, over_target, cursor, bounds)` 驱动；`set_delay` 默认 0.35s。
- 悬停累计达到延迟才显示，离开立即隐藏并清零计时；显示后跟随光标（偏移 16,18）。
- 右下溢出时翻到左上，并夹在 `bounds`（root 的 `global_bounds`）内；bounds 为空时只按偏移定位。
- 位置用相对 root 的局部坐标，由 `UIRoot::layout` 转全局；`hit_testable()` 为假，不拦截鼠标。
- 目标判定（哪个控件显示什么文本）留在调用方（gallery）。

## 兼容与迁移
- 测试中的 `GLFW_KEY_*` / `GLFW_MOD_*` 改为 `KeyCode` / `Modifier`。
- 保留 GLFW 映射集中在应用层，后续 Windows/其他后端可替换。
- 本次不改图形 primitives 的 active atlas 依赖，也不引入 IME；这些留到后续。

## FrameStats
- `graphics::FrameStats` 统一承载 frame time、FPS 与 draw 统计。
- `collect_draw_stats(DrawList, viewport)` 统计 batches、非空 draw calls、vertices、indices、clipped batches。
- Renderer 在每帧 begin 更新 frame time/FPS，在 end 更新上一帧 DrawList 统计。
- Renderer 暴露 `frame_stats()` 只读快照；overlay 允许一帧延迟。

## DebugOverlay
- `ui::DebugOverlay` 是 Panel 子类，作为 UIRoot overlay 使用，不拦截鼠标。
- 显示 FPS、frame time、draw calls、batches、vertices、indices、clipped batches、hover/focus 节点、ScrollView offset。
- hover/focus 节点通过 dynamic_cast 映射到可读名称。
- `toggle()` 控制显示；ui_demo 用 F1 切换，`COSINE_DEBUG_OVERLAY=1` 可直接启动显示。

## 测试与边界
`test_ui_text_input` 新增 `test_single_line_vertical_centering`：

- 用合成图集（行高 1.5em）构造「行高 < 控件高度」的场景
- 断言文字四边形中心与控件中心相差 ≤ 2px
- 断言上/下留白相差 ≤ 4px
- 反向验证：恢复旧实现（`max(pad, ...)`）该用例立即失败

## Asset 解析错误
- `find_asset()` 返回 `AssetLocation { path, found, searched }`，保留 `asset_path()` 兼容旧调用。
- 新增 `require_asset()`：文件缺失时抛出包含相对路径、所有搜索目录和修复提示的 `runtime_error`。
- 字体/shader/默认图集加载区分“找到路径”和“fallback 候选路径”，不再静默使用不存在的路径。

## 运行时错误上下文
- 资源、文件读取、字体、Vulkan 初始化、SwapChain 错误统一使用 `[Stage] detail` 格式。
- 字体构建/动态字形失败保留 warning，但包含字体路径、face index、coverage 文件。
- `read_file()` 错误包含完整文件路径和 shader/资源提示。
- Vulkan 设备/队列/surface 错误包含物理设备名或结果上下文。
- `SwapChain` 对空 format/present mode、image view 创建失败做显式检查，不再依赖 `assert` 或 `[0]`。

## 应用入口
- `examples/ui_demo` 的 main 增加顶层 try/catch，输出 `fatal: ...` 并返回非零。
- 不引入日志库、不改变异常层级；先用可读的 `runtime_error` 文本收口。

## Sanitizer 构建
- 顶层提供 `COSINE_SANITIZE`（`address`/`undefined`/`thread`），通过 `cosine_apply_sanitizers(target)` 应用到自有 target，不污染第三方 FetchContent 目标。
- 静态库用 `PUBLIC` 传递编译与链接选项，测试与示例自动继承。
- `scripts/ci-linux.sh` 透传 `COSINE_SANITIZE` 与 `COSINE_BUILD_DIR`，支持本地跑 ASAN 构建而不覆盖普通 build-ci。

## lavapipe 冒烟
- 新增 `scripts/smoke-lavapipe.sh`：查找 lavapipe ICD（`COSINE_LAVAPIPE_ICD` 或已知路径），在 `VK_ICD_FILENAMES` + 校验层下运行 `ui_demo` 固定秒数。
- 无 ICD 或无显示环境时明确 skip（退出码 0），有环境时校验日志不含 validation error。
- 冒烟依赖窗口系统；无显示环境用 Xvfb/xvfb-run 提供显示。

## CI
- 当前不维护远端 CI：`.github/workflows/` 已移除（2026-10-26）。
- 质量门由本地脚本承担：`scripts/ci-linux.sh` / `scripts/ci-windows.ps1`（构建、ctest、ASAN），以及 `scripts/smoke-lavapipe.sh`（Xvfb/xvfb-run 提供显示）。
- 发布阶段再按当时的 GitHub runner 工具链（编译器版本、Vulkan 头版本、源码编码选项）重建 GitHub Actions 脚本。

## 版本号
- `project(cosvideo VERSION 0.3.0)` 作为唯一版本源；CMake 生成 `version.hpp`（`xel::version_string()`）。
- `ui_demo --version` 输出 `cosine <version>`，用于打包校验。
- 版本号不在多个脚本里重复硬编码，打包脚本从 CMake 缓存 `PROJECT_VERSION` 读取。

## Linux 打包
- `scripts/package-linux.sh`：Release 配置 → 构建 `ui_demo` → 组装 `dist/cosine-<ver>-linux-x86_64/`。
- 目录布局：`ui_demo`、`assets/`、`README.md`、`VERSION`、`run.sh`；二进制 `strip`。
- assets 从 `COSINE_ASSET_BUILD_DIR`（`build-release/src/assets`）复制，包含 `shaders/`、`fonts/`。
- 产出 `dist/cosine-<ver>-linux-x86_64.tar.gz`。

## Windows 打包
- `scripts/package-windows.ps1`：Release 构建 → 组装同一布局 → `Compress-Archive` 出 zip。
- 不额外打包 Visual C++ 运行库；README/文档说明依赖。

## 字体与资源策略
- 打包内置 `default8.png` 与 GB2312 一级覆盖表；CJK 仍优先系统字体，可用 `COSINE_CJK_FONT` 覆盖。
- 不内置 CJK TTF（体积与许可证）；文档说明如何随包放置并指定。

## 悬垂交互目标
- 症状：`ui_gallery` 中点击 Dropdown 选项后下一次鼠标移动在 `ui_system.cpp` 的 `hovered_->on_mouse_leave()` 段错误。
- 原因：点击选项触发 `Dropdown::close()` → `UIRoot::remove_overlay()` 把浮层移入 `trash_`；下一次 `tick()` 的 `layout()` 销毁它们，但 `UISystem::hovered_` 仍指向已释放的 `DropdownList`。
- 同类路径：`Vbox::clear_children()`、`ScrollView::set_content()`、`RadioGroup::set_options()`、`List::clear_items()` 会**立即**销毁子控件，悬垂窗口更短，仅靠树校验无法覆盖。

## 生命周期令牌
- `Widget` 持有 `std::shared_ptr<const bool> alive_token_`，析构后 `weak_ptr` 过期。
- `UISystem` 的三个交互目标改为 `Target{ Widget*, weak_ptr }`：
  - `valid()` 同时满足指针非空与令牌未过期；
  - 隐式转换到 `Widget*`，`operator->` 与 `explicit operator bool` 让现有事件代码基本无需改写。
- `prune_stale_targets()` 在 `tick()` 开头、`layout()` 之前执行：
  - 令牌过期 → 直接清空（不能再调用虚函数）；
  - 令牌有效但已不在树上（overlay 待销毁）→ 先 `on_mouse_leave()` / `on_focus_lost()` 再清空。

## 文字调试可视化
- 新增 `graphics::TextDebugOptions` 与 `append_text_debug()`：在文字之上叠加
  - 每个字形的包围盒（按 bearing/advance 计算，与绘制同源）
  - 整段文本的行框
  - 基线
  - advance 竖线（字距刻度）
- `Label` 增加 `set_text_debug(TextDebugOptions)`，与文本一起裁剪。
- 调试绘制取主题的强调色，可 `set_text_debug_color()` 覆盖。

## 排版组件
- `ui/typography.hpp`：
  - `TextRole`（Display / Title / Heading / Subheading / Body / Caption）
  - `text_style(role, theme)` 返回字号与颜色
  - `Heading`（按 level 生成 Title/Heading/Subheading）
  - `Paragraph`（默认换行 + 省略号可配 + 对齐）

## 优化点
- 像素对齐：绘制原点取整，减少半像素采样模糊。
- 复用 `FontAtlas` 现有 1px 字形内边距（线性采样不吸边）。
- 多行按行绘制，行高来自字体 metrics，避免叠加误差。
- 不在本次引入 SDF/MSDF、字距微调（kerning）与 shaping。

## TabView
- 新控件 `TabView`：上方 `TabBar` + 下方内容区，只显示当前选中页面。
- 页面以 `children_` 暴露（`children_[0]` 为标签栏，其余为页面），
  这样命中测试、Tab 焦点遍历、布局脏标记都能复用现有机制。
- API：
  - `add_page(label, widget)` → 返回页面下标（0 起）
  - `set_page(index, widget)`、`page(index)`、`page_count()`
  - `set_selected(index)` / `selected()`（直接切换并触发 `on_change`）
  - `set_on_change(std::function<void(int)>)`
  - `tab_bar()` 访问底层标签栏
  - `set_page_fill(bool)`：页面是否填满内容区（默认 true）
- 切换语义：`TabBar::set_selected` → `TabView::apply_selection` →
  非当前页 `set_visible(false)`，因此不可见页面不参与命中与焦点遍历。
- 布局：标签栏按自身测量高度占顶部，剩余空间减去 padding 后给页面；
  所有页面按同一内容矩形 arrange（切换无额外布局抖动）。
- 绘制：自绘底部/右侧边框，内容区裁剪到自身范围。

## Dialog / Modal
- `Dialog : Panel`：标题 + 正文区（Vbox）+ 右下角操作按钮行（Hbox）。
  - `set_message()` 便捷正文（自动换行、宽度跟随卡片）。
  - `add()` 内容进入正文区；`add_action(label, handler, primary)` 追加按钮。
  - `preferred_focus()` 优先主按钮，否则第一个按钮。
  - `set_dismissable(false)` 禁止 Esc / 点击遮罩关闭。
- `DialogHost : Widget`：全屏 overlay，绘制半透明遮罩并居中对话框。
  - 点击卡片外 → 可关闭时 dismiss；所有点击都被吞掉，避免穿透。
  - 滚动事件被吞掉，背景不滚动。
  - Esc 通过按键冒泡（焦点控件 → 宿主）关闭。
  - `close()` 从 `UIRoot::remove_overlay` 移除自身（延迟销毁），`on_close` 只回调一次。
- `show_dialog(root, dialog, on_close)` 便捷入口，返回 `DialogHost*`。

## 布局与焦点
- 宿主按窗口尺寸铺满，卡片测量后居中，宽高被限制在窗口留 24px 边距内。
- 卡片 `cross_align = top_left`；按钮行 `h_stretch + main_align End`，标题左对齐、按钮右对齐。
- 打开时建议 `ui.set_focus(host->preferred_focus())`；无按钮时宿主自身可聚焦，保证 Esc 有效。

## MenuBar
- `MenuItem`：`label` / `shortcut` / `enabled` / `action` / `separator`，
  静态构造 `MenuItem::make(...)`、`MenuItem::separator_item()`。
- `MenuBar`：横向标题栏，点击标题切换 overlay 菜单。
  - `add_menu(title, items)` / `menu_count()` / `title(i)` / `items(i)`
  - `open(i)` / `close()` / `toggle(i)` / `is_open()` / `open_menu()`
  - 键盘：`Down/Enter/Space` 打开当前菜单；打开后 `Left/Right` 切换菜单、
    `Up/Down` 移动高亮（跳过分隔线与不可用项）、`Home/End`、`Enter/Space` 激活、`Esc` 关闭。
  - 打开状态下鼠标在标题区移动会切换菜单（经典菜单栏行为）。
- overlay 组成：全屏 `MenuScrim`（点击空白关闭、把标题区 hover 转发给栏）
  + `MenuPopup`（绘制菜单项、快捷键右对齐、悬停高亮、点击激活）。

## 交互细节
- 激活顺序：先 `close()` 再执行 `action`，回调里可以安全打开对话框或改菜单。
- 关闭走 `UIRoot::remove_overlay` 延迟销毁；`close()` 幂等。
- 菜单宽度取「标题宽度」与「内容最大宽度（含快捷键）」的较大者，最小 160px。
- 分隔线高度 9px，普通行高 = `theme.control_height`。

## 菜单可见性约束
1. **菜单项文字垂直居中**：行内文字按 `FontAtlas::line_height(size)` 居中（而非 `text_size`）；
   单行 `TextInput` 同样按行高居中。
2. **弹层上下留白对称**：`popup_height = 行高之和 + padding * 2`。
3. **悬停切换菜单后内容同步**：`MenuPopup::set_menu_index()` 由 `update_popup_geometry()` 调用，
   内容与几何一起随当前标题切换。

## 半透明矩形颜色
- `graphics::Rectangle` 在构造顶点时对 rgb 预乘 alpha，匹配管线 `One / OneMinusSrcAlpha`。
- 选中高亮、斑马纹、文字选区、遮罩等半透明矩形均按预乘 alpha 计算颜色。

## 菜单项 hover 高亮丢失
**现象**：菜单打开后，鼠标移到菜单项上没有任何高亮，但点击能正常触发。

**根因**：

1. `update_popup_geometry()` 每次布局都会调用 `MenuPopup::set_menu_index(open_)`，
   而该函数无条件把 `hovered_` 清成 `-1`；
2. `MenuPopup::arrange()` 走 `set_pos/set_size`，每次都会 `mark_layout_dirty()`，
   于是菜单打开期间**每帧都会重新布局** → 每帧都会清掉 hover。

**修法**：

- `set_menu_index()` 只在菜单真正变化时才清空 hover；
- `MenuPopup::arrange()` 直接写 `local_/global_`，不再通过 setter 标脏；
- `update_popup_geometry()` 不再调用 `set_min_size/set_max_size/set_pos/set_size`，
  只做 `set_menu_index` + `arrange`。

## 切换菜单后弹层尺寸不变
**现象**：File → Edit 悬停切换后，菜单内容变了，但弹层仍保持 File 的宽度/高度。

**根因**：`MenuPopup` 构造时把 `set_min_size/set_max_size` 固定成创建那一刻的尺寸；
`UIRoot::layout_child()` 会对每个 overlay 做 `child_size = child.clamp_size(child.measure(...))`，
`clamp_size` 把新尺寸夹回旧 min/max，于是 `arrange()` 拿到的一直是初始尺寸。

**修法**：

- 构造时不再设置 min/max，只写入 `local_ = {0, size}` 作为初始测量值；
- 尺寸完全由 `arrange()` 写入（`local_ = global_ = slot`），
  `measure()` 返回当前 `local_.size`，因此每帧测量与几何一致。

## 单行 TextInput 文字偏下
**根因**：上一轮把垂直居中基准从「字号」改成「行高」，但同时写了

```cpp
text_y = y + std::max(pad, (height - line_height) * 0.5f);
```

当行高（≈23）小于控件高度（30）时，`(30-23)/2 = 3.5 < padding(8)`，
`max()` 取了 padding，文字被推到 `y + 8`，整体偏下 4~5px。

多行输入不受影响，因为多行本来就从 `padding` 开始排版。

**修法**：

```cpp
const float line_h = line_height();
const float text_y = y + std::max(0.0f, (height - line_h) * 0.5f);
```

同时把单行的选区高度改为 `line_h`、光标在行盒内居中（高度仍为字号）。

## Table
- `TableColumn`：`title` / `width` / `align`（Left/Center/Right）；`width <= 0` 表示弹性列，
  剩余宽度在弹性列间均分。
- `Table`：固定表头 + 可滚动正文。
  - `set_columns` / `set_rows` / `add_row` / `clear_rows` / `row_count` / `rows`
  - `set_selected(i)` / `selected()` / `set_on_select`
  - `set_row_height` / `set_header_height` / `set_striped`
  - `scroll_offset()`；滚轮按 1.5 行滚动
- 绘制：表头背景 + 底部分隔线；正文行支持选中高亮（focus_ring 半透明）、
  hover 高亮、斑马纹；列文本按对齐方式绘制并按列宽裁剪。
- 交互：点击行选中；`Up/Down/Home/End` 移动选中并自动滚动保持可见；
  表头点击不改变选中；`focusable()` 参与 Tab 遍历。
- 列宽按「去掉滚动条后的正文宽度」分配，避免右对齐的最后一列被滚动条裁掉。

## 样式系统
### 模型

样式分三层，数据与绘制分离：

1. **调色板与度量**（`ui/theme.hpp`）：颜色字段 + 尺寸字段，仍是唯一数据源。
2. **语义解析**（`ui/style.hpp`）：把控件抽象成「样式角色 + 交互状态」，解析成 `BoxStyle`。
3. **绘制辅助**（`graphics/box.hpp`）：圆角矩形、圆角环、内阴影，纯函数追加到 `DrawList`。

控件不再自己拼 `Rectangle` 与颜色分支，只做三件事：求状态 → `Theme::style(role, state)` → 调绘制辅助。

### 角色与状态

```cpp
enum class StyleRole { Surface, Control, Input, Track, Accent, Row, MenuItem, Tooltip };
enum class ControlState { Normal, Hovered, Pressed, Disabled };

struct BoxStyle {
    glm::vec4 fill{0.0f};
    glm::vec4 border{0.0f};
    float border_width = 0.0f;
    float radius = 0.0f;
    glm::vec4 text{1.0f};
    float text_size = 16.0f;
    glm::vec4 focus_ring{1.0f};
};
```

- `Theme::style(StyleRole, ControlState) const` 返回 `BoxStyle`；未显式定义的状态回落到 `Normal`，再回落到调色板默认值。
- 状态优先级固定：`Disabled > Pressed > Hovered > Normal`；`Focused` 不作为填充状态，焦点环是叠加层（可与 hover 共存）。
- `control_state(bool enabled, bool hovered, bool pressed)` 是唯一的派生入口，控件不得再自行写 if 链。
- 角色语义：`Surface` 面板/卡片底，`Control` 按钮类，`Input` 输入类框体，`Track` 滑轨/进度/滚动条槽，`Accent` 选中与强调填充，`Row` 列表行，`MenuItem` 菜单项，`Tooltip` 浮层。
- 现有 `*_bg` 调色板字段保留（`Theme::dark/light` 与既有测试依赖），`style()` 从中取值，二者不并存出第二套真相。

### 圆角

- `graphics::append_rounded_rect(list, rect, radius, color)`：`radius` 先 clamp 到 `[0, min(w,h)/2]`。
- 几何用**九宫格不重叠切分**：中心矩形 + 四条边矩形 + 四角扇形。区域互不重叠，预乘 alpha 混合下不会二次提亮，半透明圆角面板（tooltip/dialog）因此可用。
- 角分段数按半径自适应：`segments = clamp((int)ceil(radius * 0.6f), 2, 6)`。
- `radius < 0.75` 退化为现有 4 顶点矩形，保持顶点数与旧路径一致。
- 不做抗锯齿、不做 SDF：边缘为硬边，与当前矩形/线段一致。改用 SDF 需要给 `Vertex` 增加局部坐标、改 `ui.slang` 与管线顶点描述、波及文本顶点，收益不足以支持现在做（记为后续评估项）。

### 圆角环（边框与焦点环）

- `graphics::append_rounded_ring(list, rect, radius, thickness, color)`：外圆角矩形与内圆角矩形之间的环带，内部不填充、不遮挡内容。
- 边框与焦点环共用该函数：`thickness = style.border_width` 画边框，`thickness = focus_thickness` 画焦点环。
- 焦点环画在控件外扩 `focus_offset`（默认 2px）的矩形上，不再用四条实心矩形压住内容边缘。

### 焦点环与键盘可见性

- `enum class InputMode { Mouse, Keyboard }` 放进 `UIContext`；`UISystem` 维护：`KeyDown`/`KeyUp` 置 `Keyboard`，`MouseButtonDown`/`MouseMove` 置 `Mouse`。
- `Widget::focus_visible()` = 有焦点 且 `input_mode == Keyboard`；鼠标点击聚焦时不画焦点环，键盘 Tab/方向键聚焦时画。
- 焦点由基类持有：`Widget` 增加 `focused_` 与 `set_focused()`，`on_focus_gained/lost` 默认维护它；控件删除自己的 `focused_` 字段，改读 `is_focused()`。`hovered_` 已在基类，沿用。
- `pressed_` 只在少数控件有意义（Button/Checkbox/Dropdown/Radio/Slider/TabBar/MenuItem），保留在控件内部，但要通过 `control_state()` 参与解析。

### Theme 扩展

- 圆角档位：`radius_small = 3`、`radius_medium = 6`、`radius_large = 10`；`radius`（控件默认，取 `radius_medium`）与 `radius_panel`（面板）。
- 胶囊圆角：`radius_pill = 1000`（故意取大，绘制时 clamp 到短边一半）。Slider 轨道/手柄、Progress、Radio 用它，因此「圆形控件」也跟随圆角开关；classic 样式归零后它们是直角。
- 强调色：新增 `accent`（dark `#3399FF` / light `#1F73F2`）。`focus_ring`、`checkbox_fill`、`slider_fill`、进度填充统一由 `accent` 派生，旧字段保留为别名值。
- 密度：`enum class Density { Compact, Normal, Spacious }`，`Theme::with_density(Density)` 返回缩放后的副本（Compact ×0.85、Normal ×1.0、Spacious ×1.15），作用于 `padding`/`spacing`/`control_height`/`row_height`/`checkbox_size`/`slider_handle_radius`/`scrollbar_width`；字号与圆角按 ×0.9 / ×1.0 / ×1.1 缩放。
- 字号档位：`text_size`（Body）、`small_text_size`（Caption）、`heading_text_size`（Heading）；`typography.hpp` 的比例角色保持不变，只把基础档位对齐到这三个字段。
- 内阴影：`inset_shadow`（颜色，alpha 低）+ `inset_shadow_size`（默认 6）。`append_inset_shadow()` 用 3–4 层递减 alpha 的圆角条模拟，只用于 `Surface` 顶部与 `Tooltip` 内缘；classic 样式下 `inset_shadow_size` 归零。
- 角色填充语义：`Row` / `MenuItem` 的 Hover/Pressed 沿用按钮色系（`button_hover` / `button_pressed`），保持旧观感；选中态由控件自己叠加 `accent` 半透明，不进 `BoxStyle`。

### 几何风格与焦点策略（两维正交）

旧视觉不是被替掉，而是两个可自由组合的维度：

- **几何**：`FocusStyle { Outline, Border }`。`Outline` = 外扩圆角环，`Border` = 贴控件内边缘的四条实心框（旧 Button/TextInput 的画法）；`with_rounding(false)` 同时把全部 `radius*`（含 `radius_pill`）归零。
- **时机**：`FocusPolicy { Keyboard, Always, Never }`。`Keyboard` 只在键盘操作期间显示，`Always` 聚焦即显示，`Never` 从不显示；几何与时机互不影响，任意组合都成立。
- `Theme::with_rounding(bool)` 只改几何（圆角与 `focus_style`），不碰 `focus_policy`；`Theme::with_focus_policy(...)` 只改时机。链式调用后写者胜，顺序任意。
- `with_rounding` 是单向变换：要回到圆角样式，从基础主题重新派生（`base.with_rounding(true)`），避免清零值被当成新基准。
- 判定集中在 `Widget::focus_visible()`（读 `focus_policy`）与 `Widget::append_focus_ring()`（读 `focus_style`），控件不分支。
- 像素对齐：Checkbox 的外框、边框环与勾选块都 `round` 到整像素，勾选块 inset 也取整（`round(box*0.25)`），保证四边留白严格相等；半像素 inset 会让勾选块看上去偏心、边缘发虚。
- 经典预设 = `with_rounding(false).with_focus_policy(Always)`，复刻旧观感（鼠标点击聚焦也显示边框）；现代预设 = `with_rounding(true).with_focus_policy(Keyboard)`。
- gallery 左侧导航有 `Style: Rounded/Classic` 与 `Focus: Keyboard/Always/Never` 两个独立按钮；`Style` 分区另有 `Rounded/Classic` 与 `Keyboard only/Always/Never` 两行对照。环境变量：`COSINE_GALLERY_CLASSIC=1`、`COSINE_GALLERY_FOCUS=keyboard|always|never`。
- 切换 Style 会把焦点时机复位为该风格的预设（Classic → Always、Rounded → Keyboard），之后点 Focus 按钮可任意覆盖，两维仍然自由组合。

### 第二批落地细节（列表与浮层）

- `Panel`：`Surface` 角色 + `radius_panel` 圆角 + `append_rounded_ring` 边框，hover 边框仍用 `accent`；顶部加 `append_inset_shadow`。
- `Tooltip` / `MenuPopup` / `DropdownList`：`Surface` 或 `Tooltip` 角色 + 圆角；半透明底（`tooltip_bg` alpha 0.95）依赖九宫格不重叠切分。
- `MenuBar` / `Dropdown` 按钮 / `TabBar` 标签：`Control` 角色（Normal/Hovered/Pressed），焦点统一 `append_focus_ring`。
- `Table` / `TreeView`：面板背景 `Surface`，行高亮 `Row` 角色 + `radius_small`（左右内缩 2px），选中态仍是 `accent` 28% 半透明叠加，表头保持直角（面板内部元素，不跟随圆角）。
- 滚动条（ScrollView / Table / TreeView）：轨道 `Track` 角色，滑块 `scrollbar_thumb`，圆角用 `radius_pill`（modern 圆头 / classic 直角）。
- `Splitter`：手柄颜色仍为 `panel_border`，hover 时用 `Control` hover 填充，圆角 `radius_small`。
- 内阴影随 `with_rounding(false)` 关闭（`inset_shadow_size` 归零），classic 完全回到旧观感。

### 迁移顺序

- 第一批（状态最丰富、复用最广）：`Button`、`Checkbox`、`Radio`、`Slider`、`Progress`、`TextInput`。
- 第二批（列表与浮层）：`Panel`、`MenuBar`、`Dropdown`、`TabBar`、`Table`、`TreeView`、`Tooltip`、`Dialog`、`ScrollView` 滚动条、`Splitter`。
- 每批迁完即构建 + 全量 ctest，不留不可编译的中间态。

### 验证

- `test_ui_style`：`style()` 各角色/状态取值来源；`control_state()` 优先级；`with_density()` 缩放与幂等；`with_rounding()` 归零与 `Border` 焦点框几何（16 顶点、不外扩）；`with_focus_policy()` 与几何的 2×3 组合矩阵（可见性 + 几何顶点数）；圆角 clamp；圆角矩形几何（顶点/索引数、索引不越界、退化到 4 顶点）；环带厚度与半径退化；圆形控件退化（slider 12 / progress 24 / radio 26 顶点）；Checkbox 像素对齐（整数顶点、勾选块与外框同心、四边留白相等）。
- `test_ui_theme` 现有断言必须继续通过（`name`、明暗对比、切主题后 padding/text_size 生效）。
- gallery 新增 `Style` 分区：同一组控件并列展示 normal/hover/pressed/disabled/focused 五种外观（用 `set_hovered()`/`set_enabled(false)`/`set_focused()` 构造伪状态），半径与密度各一组按钮，键盘 Tab 进入控件时可见焦点环、鼠标点击后隐藏。
- 全量：`cmake --build build -j` + `ctest` 全绿，gallery 连续运行无 validation error。

### 边界（本变更不做的事）

- 不做颜色/圆角插值过渡：需要 `Widget::update(dt)` 与 `UISystem::tick(dt)` 的节拍改造，与「UI 动画」事项同批做，避免本次同时改状态模型与时间模型。
- 经典样式只保留「直角 + 贴边焦点框」这一组几何，不保留其它历史特例（各控件曾经各自写的 hover 高亮色改为统一 `ControlState`）。
- 不做 SDF/MSDF 圆角、不做真正的高斯内阴影与投影、不做模糊/毛玻璃。
- 不做主题文件（JSON/TOML）加载与样式表覆盖；`Theme` 仍是 C++ 结构体，`style()` 可被继承替换。
- 不改控件布局语义（度量字段只被密度缩放，不改变 measure/arrange 规则）。
- 不引入第三方 UI/样式库。

## UI 动画（状态过渡）
### 目标

样式系统把控件外观收敛到 `ControlState` + `BoxStyle`，但状态切换仍是瞬时跳变：
鼠标进入的一帧内前景/背景直接换色，视觉上生硬。本次给状态切换加平滑过渡，
同时补上 UI 层的时间节拍（`tick(dt)`），为后续弹层出现/消失过渡打底。

### 时间节拍

- `UISystem::tick(float dt)` 新增；保留 `tick()` 无参重载，内部转调 `tick(1.0f / 60.0f)`，
  既有测试与 demo 的调用点不动。
- 主循环应传真实帧时间：`app.uisys.tick(frame_dt)`，并在入口把 `dt` clamp 到 `[0, 0.1]`，
  避免卡顿或断点恢复后一帧跳完整段动画。
- 更新顺序：`prune_stale_targets()` → resize/layout（脏标记）→ `update_tree(dt)`。
- `update_tree` 由 `UISystem` 统一遍历（`children()` / `overlays()`，跳过不可见项），
  容器不需要各自实现递归。
- `Widget::update(float dt)` 是新的虚钩子，默认空实现；只有需要动画的控件覆写。

### 过渡模型（集中在 Widget 基类）

```cpp
struct StyleTransition {
    StyleRole role = StyleRole::Control;
    ControlState state = ControlState::Normal;
    BoxStyle from{}, to{};
    float progress = 1.0f;   // 1 = 已完成
    bool started = false;
};
```

- 控件绘制时调用 `visual_style(role, state)`（替代直接 `theme.style(...)`）。
- 首次调用、或 `transition_ms == 0`：直接返回目标样式，不建立过渡。
- `(role, state)` 变化时：`from` 取**当前插值结果**（不是上一个状态的目标值），
  因此中途反向切换（hover→normal→hover）不会跳变；`to` 取新目标；`progress` 归零。
- `progress` 在 `Widget::update(dt)` 中按 `dt / (transition_ms/1000)` 推进并 clamp 到 1。
- 缓动固定 ease-out quad（`1 - (1-t)^2`）：起手快、收尾稳，与 UI 反馈的直觉一致。

### 只过渡颜色，几何瞬时切换

- 插值字段：`fill`、`border`、`text`、`focus_ring`。
- 不插值：`radius`、`border_width`、`text_size`。原因是圆角半径变化会改变圆弧分段数，
  相邻帧顶点数不同会让批次数与绘制结果抖动；几何瞬时切换在 100ms 尺度上不可见。
- 由此保证：同一控件在整段过渡中顶点/索引数恒定，便于测试断言。

### Theme 字段

- `float transition_ms = 120.0f`：0 表示关闭动画（立即到位）。
- `Theme::with_transition_ms(float)`：链式设置，与主题/密度/圆角/焦点策略并列。
- 过渡不随 `with_rounding(false)` 自动关闭：经典样式约束的是**静态外观**
  （直角、贴边焦点框、无内阴影），动效是独立维度，可用 Transition 选项自行关掉。
- 不做“减少动效”的系统偏好读取（无平台 API 接入），需要时把 `transition_ms` 设 0。

### 接入范围

- 已完成样式迁移的全部控件：控件内 `t.style(role, state)` 改为 `visual_style(role, state)`。
- 不接入：`Table` / `TreeView` 的行高亮（行状态是行索引，不是控件状态），
  `MenuBar` 的标题项（同理），滚动条（无状态色过渡意义）。
  它们仍用即时取色，若后续需要再单独加行级过渡。

### 验证

- `test_ui_transition`（纯逻辑，无 Vulkan）：
  - `transition_ms = 0`：状态切换后立即等于目标色。
  - 建立过渡后首帧等于 `from`；`tick(dt)` 推进到一半时各颜色分量严格位于 `from` 与 `to` 之间。
  - 累计 `dt` 达到时长后等于目标色，且 `progress` 归 1。
  - 中途反向：`progress = 0.4` 时切回 `Normal`，切换后的值接近（而非跳回）切换前的值。
  - 几何恒定：过渡全程 `radius` / `border_width` / `text_size` 等于目标；`append_to` 顶点数不变。
  - 确定性：同样的 `dt` 序列重放得到相同的插值结果。
- 现有 28 个测试继续通过（`tick()` 无参重载保证不破坏调用点）。
- gallery `Style` 分区新增 Transition 行（Off / 120 ms / 240 ms），
  环境变量 `COSINE_GALLERY_TRANSITION=off|120|240`；主循环传真实帧时间。
- gallery 冒烟：hover/pressed 切换时无 validation 错误，检查 `validation=0`。

### 边界（本次不做）

- 不做面板/弹层的出现/消失过渡（需要 alpha 合成与可见性生命周期改造，另立一条）。
- 不做尺寸/位置动画（那是 M4 时间轴的职责）。
- 不做行级高亮过渡（Table/TreeView/MenuBar 标题）。
- 不做“减少动效”系统偏好、不做可配置缓动曲线（固定 ease-out quad）。
- 不引入第三方动画库。

## 指针事件
### 现状

- `event::InputEvent` 已带 `mouse_button`（直接沿用 GLFW 值：0 左 / 1 右 / 2 中）。
- `UISystem::dispatch_mouse_button` 对**任何**按钮都做同一套事：可聚焦就抢焦点、走 `on_mouse_down/up`、同目标再走 `on_click`。
- 因此右键也会抢焦点、也会触发 `on_click`（按钮被右键激活）；双击与长按完全没有，控件无法表达「双击重命名」「右键菜单」「长按拖拽」。

### 目标

补齐右键、双击、长按三种语义，作为右键菜单、TreeView 内联重命名、长按拖拽的前置能力；
所有判定用 UI 自己的时间基准，保证纯逻辑可测。

### 设计

**新增虚钩子（`Widget`）**

```cpp
virtual bool on_right_click(glm::vec2 pos);          // 右键单击
virtual bool on_double_click(glm::vec2 pos, int button);
virtual bool on_long_press(glm::vec2 pos, int button);
```

默认返回 `false`（不消费），沿用现有冒泡 + 消费机制。

**按钮语义**

- 左键（0）：行为不变 —— 抢焦点、`on_mouse_down/up`、同目标 `on_click`。
- 右键（1）：**不抢焦点、不触发 `on_click`**；释放且 `pressed == target` 时发 `on_right_click`。
  右键抢焦点会让「右键菜单弹出后原控件失焦」等行为难以推理，故明确排除。
- 中键（2）：保持现状（走 `on_click`，由 `button` 参数区分）。

**双击**

- 时间基准：`UISystem` 自己维护 `elapsed_`，在 `tick(dt)` 累加；不使用系统时钟，因此可确定性测试。
- 判定（全部满足）：同一目标、同一 button、距上次 click ≤ `double_click_ms`（默认 400）、
  两次位置距离 ≤ `double_click_slop`（默认 6px）。
- 触发顺序：两次 `on_click` 都照常发，第二次之后再追加一次 `on_double_click`；
  触发后清空 `last_click`，避免三击连发。
- 位置容差用于吸收手抖；不同控件上的两次点击不算双击。

**长按**

- 按下时记录 `press_time / press_pos / press_target / press_button`。
- `tick(dt)` 中检查：仍按住、指针仍在目标 `global_bounds()` 内、`elapsed - press_time ≥ long_press_ms`（默认 600）、
  尚未触发过 → 发一次 `on_long_press`，并标记已触发。
- 指针离开目标范围 → 取消（不再触发）；释放后重置状态。

**配置**

`UISystem::set_double_click_ms(float)`、`set_long_press_ms(float)`、`set_double_click_slop(float)`，
供应用层按平台/设备习惯调整；0 表示禁用对应手势。

### 接入范围（最小）

- `TreeView`：双击行切换展开/折叠（叶子行不响应），与已有的 `Enter`/`Space` 行为一致。
- gallery：`Inputs` 分区新增 `Pointer events` 行 —— 一个目标按钮 + 一个状态标签，
  显示最近一次 click / double / right / long，用于人工验收。
- 其余控件暂不接：右键菜单本体、双击重命名、长按拖拽分别留给后续变更。

### 验证

- 新增 `test_ui_pointer_events`（纯逻辑）：
  - 左键：同目标 down/up → `on_click` 一次且获得焦点。
  - 右键：同目标 down/up → `on_right_click` 一次，**不**触发 `on_click`、**不**改变焦点。
  - 双击：阈值内 → click×2 + double×1；超过 `double_click_ms` → 无 double；位置偏移超过 slop → 无 double。
  - 双击：目标不同 → 无 double。
  - 长按：按下后 `tick` 累计超阈值 → long×1，继续 tick 不重复；释放后不再触发。
  - 长按取消：按下后指针移出目标 bounds，超时也不触发。
  - 消费：子控件消费 `on_double_click` 时父控件不再收到（冒泡验证）。
- 现有 29 个测试全部通过；ASAN 通过。
- gallery 冒烟：`validation=0`。

### 边界（本次不做）

- 不做右键菜单本体（需要 overlay 菜单 + 上下文绑定）。
- 不做三击、多指/触摸、手势识别（缩放/旋转）。
- 不做长按拖拽的视觉反馈（与 `Slider`/`Splitter` 的拖拽反馈分开做）。
- 不改 `on_click` 签名，不引入自定义按钮映射（沿用 GLFW 值）。
- 不改应用层事件桥接（`mouse_button` 已透传）。

## 键盘导航
### 现状

- Tab 顺序完全由 `UISystem::collect_focusable` 的深度优先树序决定（overlay 先于 children），控件无法调整。
- `TabBar` 已有 Left/Right/Home/End 切换；`Dropdown` 已有 Up/Down/Home/End/Enter/Esc。
- `RadioGroup` 没有方向键，且组内每个 `Radio` 都是独立的 Tab 停靠点 —— 与「单选组只占一个 Tab 停靠点、组内用方向键切换」的标准语义不符。

### 设计

**1. Tab 顺序可配置**

```cpp
// Widget
void set_tab_order(int order);
int tab_order() const;
```

- `> 0`：显式顺序，数值小的先；建议 1..9999。
- `0`（默认）：按树序，与现状一致。
- `< 0`：不参与 Tab 遍历（仍可鼠标点击聚焦，也可被 `set_focus` 程序聚焦）。

排序规则：`collect_focusable` 先按树序收集，再 `std::stable_sort`：

```text
键 = (order > 0) ? order : kAutoOrder   // kAutoOrder = 1'000'000
```

于是显式顺序整体排在「按树序」的控件之前，同类内部保持树序（`stable_sort` 不改变相对位置）。

**2. RadioGroup 组内导航（roving tabindex）**

- `RadioGroup::focusable()` 返回是否已有选项。
- `set_options` 时给每个 `Radio` 设 `tab_order(-1)`：整个组只占一个 Tab 停靠点。
- 方向键：`Up`/`Left` 上一项，`Down`/`Right` 下一项，`Home`/`End` 首/尾；
  切换时调用 `select(index, true)` 并把「组内焦点标记」移到新选项。
- `on_focus_gained` 把焦点标记放到当前选中项；`on_focus_lost` 清理（并调用基类）。
- 焦点框仍由 `Radio` 自己的 `append_focus_ring` 绘制，因此与 `FocusStyle`/`FocusPolicy` 自动一致。

**3. 不改的东西**

- `TabBar` / `Dropdown` 的方向键已具备，本次不动。
- 程序性焦点（`set_focus`）不受 `tab_order < 0` 限制，便于弹窗默认焦点等场景。

### 验证

`test_ui_focus_order`（纯逻辑）：

- 默认树序：与迁移前一致（回归保护）。
- 显式顺序 3/1/2 → Tab 顺序 1→2→3，Shift+Tab 反向。
- `tab_order < 0` 的控件被 Tab 跳过，但 `set_focus` 仍可聚焦。
- 混排：显式 order 的控件排在未设 order 的控件之前；同 order 保持树序。
- RadioGroup：Down/Up/Home/End 切换选中且"组内焦点"跟随；组内 Radio 的 `tab_order < 0`；
  Tab 从组直接跳到下一个控件（组只占一个停靠点）。
- 现有 31 个测试全部通过；ASAN 通过。

gallery：`Inputs` 分区新增 `Tab order (3 → 1 → 2)` 三个按钮，验证 Tab 实际顺序；
Radio group 标题补「Up/Down 切换，组只占一个 Tab 停靠点」。

### 边界（本次不做）

- 不做焦点陷阱 / 焦点环（modal 已通过 overlay 优先级实现类似效果）。
- 不做任意容器的方向键焦点移动（只 RadioGroup；TabBar/Dropdown 已有）。
- 不做焦点记忆（切回窗口恢复上次焦点）。
- 不引入 `tab_order` 的持久化或运行时重排（顺序在布局树变化后按同一规则重算）。

## 拖拽反馈
### 现状

- `Slider` 拖动时只有值在变：手柄与轨道颜色几乎不变，也看不到当前数值。
- `Splitter` 拖动时手柄样式与 hover 相同，没有"正在拖拽"的视觉；`ratio` 固定 clamp 在 `[0.1, 0.9]`，不考虑子项自己的 `min_size`，子项可能被压到 0 宽。

### 设计

**Slider**

- 拖动中（`dragging_`）：
  - 手柄外圈一条 `accent` 描边环（`append_rounded_ring`，厚度 2，半径 = 手柄半径 + 1）。
    复用焦点环的几何，但**不依赖** `FocusPolicy`（拖拽反馈不是焦点，键盘策略为 Never 时也应显示）。
  - 手柄上方显示数值气泡：圆角矩形 + 居中文本。
    宽度 = 文本宽 + `padding`，高度 = 行高 + `padding * 0.5`；
    位置 = 手柄中心上方 10px，水平中心对齐手柄，并 clamp 到控件 bounds 内（避免被父级裁剪）。
  - 文本格式：`step_ >= 1` 用 `{:.0f}`，否则 `{:.2f}`；没有 `font_atlas` 时跳过气泡。
- hover（未拖动）：维持现有的手柄混入 `accent` 15%。
- 边界：`value_` 已有 clamp；气泡只反映当前值。
- 副作用：拖动期间顶点数增加（气泡 + 环），松开后恢复原状。

**Splitter**

- hover：维持现有 `Control` hover 填充 + 主题圆角。
- 拖动中（`dragging_`）：手柄填充改为 `accent` 28% 半透明，并叠加 `accent` 描边环（厚 1），明确"正在拖拽"。
- 边界约束：`set_ratio` 保留 `[0.1, 0.9]` 兜底，并按子项 `min_size` 推导实际上下限：

```text
usable = 主轴尺寸 - handle_size
lower  = clamp(first_min / usable, 0, 0.9)
upper  = clamp(1 - second_min / usable, lower, 1)
ratio  = clamp(ratio, max(0.1 兜底…仅当 min_size 为 0), ...)
```

取主轴分量：`Horizontal` 用 `min_size().x`，`Vertical` 用 `min_size().y`。
`arrange` 时也 clamp 一次，保证容器尺寸变化后不越界。

**gallery**

- `Inputs` 分区 Slider 标题补「drag shows value bubble」。
- `Containers` 分区 Splitter 子面板设 `min_size`，标题补「drag highlight, respects child min size」，用于人工验收约束效果。

### 验证

`test_ui_drag_feedback`（纯逻辑）：

- Slider：按下后 `append_to` 顶点数增加（气泡 + 环），松开后恢复；拖动到范围外时值仍被 clamp；`step_ >= 1` 与 `< 1` 两种气泡都不崩。
- Splitter：给子项设较大 `min_size` 后拖到极端，`ratio` 被限制在可满足 min_size 的区间；`dragging` 时顶点数增加；松开后恢复。
- 现有 32 个测试全部通过；ASAN 通过；gallery 冒烟 `validation=0`。

### 边界（本次不做）

- 不做鼠标光标形状切换（需要 window API，属应用层）。
- 不做拖拽起始阈值、双击复位、拖拽结束吸附。
- 气泡不做出现/消失动画（瞬切）。
- 不改 `Splitter::handle_size`（固定 6），不改 Slider 的轨道内缩布局。

## 进度与子菜单
### Progress 不确定态

- `set_indeterminate(bool)` / `indeterminate()`：切换确定/不确定两种表现，默认确定。
- 动画时间用已有的 `Widget::on_update(dt)`（由 `UISystem::update_tree` 每帧驱动），
  推进 `phase_ ∈ [0, 1)`，周期固定 1.2s；仅在 `indeterminate()` 为真时推进。
- 绘制：`Track` 角色轨道 + 一条宽 30% 的 `accent` 条带，位置 `x = (w + band) * phase - band`，
  即从左外滑到右外循环；条带两端圆角用 `radius_pill`（classic 为直角）。
- 确定态绘制不变；切回确定态时保留 `phase_`（下次再进入不确定态从上次位置继续）。
- 不确定态不显示百分比文本（本来就没有）。

### 菜单子菜单（多层）

- `MenuItem::children`（`std::vector<MenuItem>`，默认空）+ 工厂 `MenuItem::submenu(label, children, enabled)`；
  `has_submenu()` = `!children.empty()`。递归结构天然支持任意层数。
- `MenuPopup` 改为持有自己的 `items` **拷贝**（`set_items`）：顶层来自 `MenuBar` 的菜单，子层来自父项 `children`。
  菜单项数量小，拷贝换来地址稳定与生命周期简单。
- `MenuBar` 用 `std::vector<std::unique_ptr<MenuPopup>> popups_` 维护层级栈：
  `popups_[0]` 是顶层菜单，`popups_[1..]` 是逐层打开的子菜单。
- 绘制：有子菜单的行右侧画右向三角（`graphics::Triangle`），快捷键文本让位。
- 交互：
  - 悬停到「有子菜单」的行 → 关闭该层之后的所有子菜单，再打开新子菜单；
  - 悬停到无子菜单的行 → 关闭该层之后的子菜单；
  - 定位：子菜单 `x = 父 popup 右边 - 4`；右边界不足时翻到父 popup 左侧；`y = 该行顶部`，并 clamp 到窗口内；
  - 点击项：有子菜单则打开子菜单；否则执行 `action` 并关闭整条链；
  - `Escape` / 点击菜单外 → 关闭整条链。
- 键盘（作用于 `active_depth_ = popups_.size() - 1` 这一层）：
  - `Up`/`Down`/`Home`/`End` 在当前层移动高亮；
  - `Right` 进入子菜单（高亮项有子菜单时）；`Left` 退回父层；
  - `Enter`/`Space`：有子菜单则进入，否则执行 `action` 并关闭全部；
  - `Escape` 关闭全部。
- 每层的高亮：`MenuPopup` 自己持有 `hovered_`（鼠标）与 `keyboard_highlight_`（键盘），
  绘制时鼠标优先、否则用键盘高亮，不再依赖 `MenuBar` 的单一 `highlight_`。
- 顶层菜单之间的 Left/Right 切换保留（在 `active_depth_ == 0` 且当前项无子菜单时生效）。

### 验证

`test_ui_progress`：

- 不确定态：`tick(dt)` 后 `phase` 增加，条带位置随之变化；不同 `dt` 序列可复现。
- 确定态：`phase` 不推进，绘制与迁移前一致（顶点数不变）。
- 顶点数：不确定态 > 0，且条带在两端不越出控件裁剪区。

`test_ui_submenu`：

- `MenuItem::children` 结构与 `has_submenu`。
- 悬停有子菜单的行 → overlay 数量 +1；悬停无子菜单的行 → 回到原数量。
- 导航到第二层：继续悬停另一带子菜单的项 → overlay 数量按层增加。
- 键盘：`Right` 进入、`Left` 退回、`Enter` 执行并全关；多层时 `Left` 只退一层。
- 点击子菜单项 → `action` 被调用一次，且所有 overlay 关闭。
- `Escape` 关闭整条链。
- 子菜单项的 `action` 与快捷键文本不影响布局（右三角与快捷键不重叠）。

gallery：

- `Inputs` 分区新增 indeterminate Progress（gallery 主循环已传真实 `dt`）。
- `Menu` 分区增加一个多层子菜单示例（如 `Edit → Advanced → Text → …`），用于人工验收。

### 边界（本次不做）

- 子菜单支持任意层数；不做子菜单的打开/关闭动画（与「面板/弹层出现消失过渡」一起留到以后）。
- 不做 `Progress` 的百分比文本、不做条带走动的缓动曲线（匀速）。
- 不做子菜单的虚拟滚动（项过多时不做滚动，只 clamp 到窗口内）。

## Grid 布局容器
### 为什么

现有容器只有 `Vbox`/`Hbox` 两种一维排布。M6 时间轴编辑器的检查器面板是典型的
「左标签 / 右控件」两列表单，用一维容器拼需要嵌套多层 Hbox 且列宽无法对齐；
工具栏、属性网格也需要规则的多列布局。`Grid` 用一次排布给出对齐的列。

### API

`class Grid : public Widget`，与 `Vbox`/`Hbox` 同层，放 `src/ui/grid.hpp` + `grid.cpp`。

列宽规格二选一，互斥：

- `set_columns(int count)`：`count` 等分（每列权重 1），非法值（<=0）回退 1 列。
- `set_column_widths(std::vector<float> widths)`：逐列指定。
  - `> 0`：固定像素宽；
  - `<= 0`：弹性列，按 `|值|` 作权重分享剩余空间（`-1` 即常见的 `1fr`）；
  - 全部非正时按权重分整宽；空列表回退 1 列。

其余设置：

- `set_spacing(float all)` / `set_spacing(float x, float y)`：列间距、行间距。
- `set_cell_align(Align)`：单元格内对齐，默认 `Align::h_stretch()`
  （横向填满单元格、纵向靠上），表单场景最常用。子项可用 `set_align()` 单独覆盖。
- padding / margin / min_size / max_size / clamp_size 沿用 `Widget`。

### 排布规则

- 行数由子项数推导：`rows = ceil(children / cols)`，不单独设置；子项增多自动加行。
- 第 `i` 个子项落在 `row = i / cols`、`col = i % cols`。
- 行高 = 该行所有子项 measure 高度（含上下 margin）的最大值；行内各格等高。
- 列宽在 `arrange` 时按实际 slot 宽计算：先扣 padding 与间距，固定列取指定值，
  剩余宽度按弹性权重分配；固定列总和超过可用宽时按比例压缩，避免负宽。
- 单元格内的子项可用区域 = 格宽 − 水平 margin，格高 − 垂直 margin；
  按 `align_self()`（缺省用 `cell_align`）在可用区域内摆放，`stretch` 时填满。
- `measure(constraints)`：
  - 列宽确定规则同上；`constraints.x` 为无界（>= 1e8）时，弹性列取该列子项的自然宽最大值，
    使「内容自适应」可用；
  - 行高按列宽约束测量后取行内最大；总高 = 行高和 + 行间距 + padding；
  - 与 `Vbox` 一致做 `(constraints, measure)` 缓存。
- 忽略子项 `flex_grow()`：格大小由列宽/行高决定，不做纵向剩余空间分配（与 `Vbox` 语义不同）。
- 本版不做跨列（span）/ 跨行、不做固定行高、不做自动行列重排。

### 绘制与命中

- `hit_testable() == false`，无自身绘制（仅 debug 边框与子项）。
- `append_to` 只遍历可见子项，与 `Vbox` 一致。

### 验证

新建 `test/test_ui_grid.cpp` 并注册到 `test/CMakeLists.txt`：

- 等分：3 列 × 2 行，检查每格 x/宽与行 y/高、间距与 padding。
- 固定 + 弹性：`{80, -1}`，检查第一列 80、第二列吃满剩余。
- 权重：`{-2, -1}` 按 2:1 分配。
- 无界约束下 measure：弹性列取自然宽最大值。
- 行高取行内最大，总高 = 行高和 + 间距 + padding。
- 默认 `h_stretch` 横向填满；`set_align(center)` 覆盖后按内容居中。
- 子项 margin 生效；空 Grid / 非法列数不崩、回退 1 列。

gallery `Layout` 分区增加示例：

- 2 列表单（标签 + 输入框，标签列固定 120，内容列弹性）；
- 3 列按钮矩阵（等分），用于人工验收对齐与间距。

### 边界（本次不做）

- 跨列 / 跨行、固定行高、自动重排（`grid-auto-flow: dense` 之类）。
- 百分比尺寸与 aspect ratio 属于后续「百分比尺寸」变更。
- 不做 Grid 的线条/背景绘制（只做布局，视觉由子项与父容器提供）。

## Wrap（流式换行）布局容器
### 为什么

`Grid` 要求列数固定；工具栏、标签云、卡片流这类「子项数量不定、宽度不一」的场景需要
「放不下就换行」。用 `Hbox` 嵌套无法表达，用 `Grid` 会在末尾留下空格。`Wrap` 一次排布
就能让子项按可用宽度自然折行。

### API

`class Wrap : public Widget`，放在 `src/ui/wrap.hpp` + `wrap.cpp`。

- `set_spacing(float all)` / `set_spacing(float dx, float dy)`：水平间距、行间距。
- `set_row_align(MainAlign)`：行内对齐，`Start`（默认）/ `Center` / `End` / `SpaceBetween`。
- `set_cross_align(Align)`：行内子项垂直对齐，默认 `Align::center_left()`
  （在行高内垂直居中）。子项可用 `set_align()` 单独覆盖。
- padding / margin / min_size / max_size / clamp_size 沿用 `Widget`。

### 排布规则

- 按子项顺序从左到右累加：当「当前子项含 margin 的宽」放不进剩余行宽，且该行已有子项时换行。
- 单个子项宽于整行时独占一行，按自身宽摆放（超出部分由父容器/滚动容器裁剪），x 不为负。
- 行高 = 行内子项高（含上下 margin）的最大值；行内子项按 `cross_align` 在行高内垂直摆放。
- 行内剩余宽按 `row_align` 分配：`Start` 靠左、`Center` 居中、`End` 靠右、
  `SpaceBetween` 均分到子项间隙（仅当行内至少两个子项）。
- `measure(constraints)`：
  - 有限宽（`< 1e8`）：按 `constraints.x - padding` 计算折行，总宽 = 最宽行的实际宽，
    总高 = 各行高之和 + 行间距 + padding；
  - 无界宽（`>= 1e8`）：不折行，全部排成一行；
  - 与 `Vbox` 一致做 `(constraints, measure)` 缓存。
- 忽略子项 `flex_grow()`：折行布局没有「纵向剩余空间分配」的概念。

### 绘制与命中

- `hit_testable() == false`，无自身绘制（仅 debug 边框与子项）。
- `append_to` 只遍历可见子项，与 `Vbox` 一致。

### 验证

新建 `test/test_ui_wrap.cpp` 并注册到 `test/CMakeLists.txt`：

- 有限宽折行：内容宽 100、5 个 30 宽子项、间距 5 → 第一行 3 个、第二行 2 个；检查 x/y 与总高。
- 无界宽：全部一行，总宽 = 5×30 + 4×5 = 170。
- `row_align`：`Center` / `End` / `SpaceBetween` 下的行内起始 x 与间隙。
- `cross_align`：行内高度不同的子项按 `center_left` 垂直居中。
- 单个子项宽于容器：独占一行、x 不为负。
- padding、margin、空容器、零/负间距不崩。
- measure 与 arrange 的结果一致（同一约束下总高相同）。

gallery `Layout` 分区：一排不同宽度的按钮/标签自动折行，用于人工验收。

### 边界（本次不做）

- 不做垂直方向的换列（column flow）、不做 `dense` 重排（不改变子项顺序）。
- 不做子项跨行。
- 百分比尺寸与 aspect ratio 属于后续「百分比尺寸」变更。

## 百分比尺寸与宽高比
### 为什么

M6 时间轴编辑器的预览卡需要固定 16:9；属性面板需要「占一半宽」「撑满高」这类相对尺寸。
现在所有控件只能用固定像素，容器尺寸变化后子项不跟随，只能在外层手工换算。
把「相对尺寸」下沉到 `Widget` 与容器的测量契约里，一处声明、所有容器生效。

### API（Widget 层）

- `set_size_percent(glm::vec2 pct)`：每轴占父级**可用空间**的比例。
  `> 0` 启用，`<= 0` 不启用（默认两轴都不启用）。
- `set_aspect_ratio(float ratio)`：宽 / 高。`> 0` 启用，默认关闭。
- 两者可组合；`min_size` / `max_size` 仍然生效（最后 clamp）。

### 解析规则（`Widget::measure_child`）

容器测量子项时统一走 `static glm::vec2 Widget::measure_child(Widget& child, glm::vec2 constraints)`：

1. **百分比**：`constraints` 该轴有限（`< 1e8`）时，把传给子项的约束换成
   `constraints × pct`；约束无界时百分比不生效（退回子项自然尺寸）。
2. 子项按调整后的约束 `measure`；启用百分比的轴直接用调整后的约束值作为尺寸。
3. **宽高比**：
   - 指定了百分比高、没指定百分比宽 → `宽 = 高 × ratio`；
   - 其余情况 → `高 = 宽 / ratio`（优先宽）。
4. 最后 `clamp_size` 到 `min_size` / `max_size`。

百分比与容器对齐的关系：百分比先确定尺寸，`stretch` 不再覆盖它
（即 `set_size_percent({0.5, 0})` + `h_stretch` 时宽仍是一半）。

### 接入范围

- `Vbox` / `Hbox` / `Grid` / `Wrap` 的子项测量与排布统一改用 `measure_child`，
  替换原来的 `child->measure(...)` + `child->clamp_size(...)` 组合。
- `ScrollView` / `TabView` / `Dialog` 测量的是「内容自然尺寸」（内容应撑满或不受限），
  语义不同，本次不改；它们内部的 Vbox/Grid 等仍会正常解析子项百分比。

### 验证

新建 `test/test_ui_percent_aspect.cpp` 并注册：

- Vbox 中 `set_size_percent({0.5, 0})` → 宽 = 容器内容宽的一半。
- 百分比与 `stretch` 同时存在时，百分比优先。
- 无界约束下百分比不生效（退回自然尺寸）。
- `aspect_ratio` 由宽推高；百分比高 + ratio 时由高推宽。
- Grid 单元格内的百分比相对格宽；Wrap 内相对内容宽。
- `min_size` / `max_size` 仍生效。
- 未启用时行为与迁移前一致（现有 37 个测试全绿）。

gallery `Layout` 分区：
- 16:9 预览卡（宽 40%）；
- 50% 宽 + 100% 高的色块，验证相对尺寸随容器变化。

### 边界（本次不做）

- 不做相对兄弟节点或相对内容的百分比（CSS 的 min-content / auto）。
- 不做百分比 margin / padding。
- 不做绝对定位与 z-index（下一项）。

## 绝对定位与层叠顺序
### 为什么

现有容器只能顺序排布，没法表达「角标固定在卡片右下角」「浮层盖在某个控件上方」。
`UIRoot` 的 overlays 只解决全局弹层，容器内部仍是纯流式。需要两个能力：

1. **绝对定位**：子项脱离流，用自身坐标相对父内容区定位，不影响兄弟。
2. **z-index**：同层内显式控制绘制与命中的先后。

### API（Widget 层）

- `set_absolute(bool)` / `absolute()`：默认 `false`。为真时容器不把它计入流式尺寸，
  按 `{内容区原点 + x(), 内容区原点 + y()}` 放置；尺寸仍走 `Widget::measure_child`
  （因此支持百分比与宽高比）。
- `set_z_index(int)` / `z_index()`：默认 `0`。数值大的画在上面、命中优先。
- `std::vector<Widget*> paint_order()`：按 z 稳定升序（同 z 保持加入顺序）；
  没有任何非零 z 时直接按原顺序返回，避免额外排序。

### 顺序规则

- 绘制：同层内按 `paint_order` 升序（z 小的先画）；`overlays` 始终在 `children` 之上。
- 命中：同层内按 `paint_order` **降序**（z 大的先命中）；`overlays` 仍优先于 `children`。
- 更新（动画）与 Tab 收集使用同一顺序，保证行为一致。

### 接入范围

- `Vbox` / `Hbox` / `Grid` / `Wrap`：`measure` 跳过绝对定位子项（不计入尺寸），
  `arrange` 单独摆放它们；`append_to` 用 `paint_order`。
- `UIRoot`：`append_to` 的 children 与 overlays 各自按 `paint_order`；
  `layout_child` 本来就按 `x()/y()` 定位，无需改动。
- `UISystem`：`collect_hit_path` 与 `update_tree` 改走 `paint_order`（反向命中）。
- 其它容器（Panel / TabBar / Table 等）暂不排序，仍按加入顺序绘制——它们没有重叠场景。

### 验证

新建 `test/test_ui_absolute_zindex.cpp` 并注册：

- Vbox 中一个绝对定位子项：兄弟位置与总高不受影响，子项按 `(x, y)` 相对内容区放置。
- 绝对定位 + 百分比宽：尺寸按父内容区解析。
- Grid / Wrap 中绝对定位子项不占格、不参与折行。
- `paint_order`：z 升序稳定；全为 0 时保持原序。
- 绘制顺序：两个重叠子项，z 大的顶点在 DrawList 中靠后。
- 默认行为不变（现有 38 个测试全绿）。

gallery `Layout` 分区：一个卡片容器，右下角绝对定位角标（徽标），说明不参与流式布局。

### 边界（本次不做）

- 不做 `position: fixed`（相对窗口）与相对最近定位祖先的 CSS 语义；绝对定位一律相对直接父容器的内容区。
- 不做 z-index 作用于 overlay 与 children 之间的跨界排序（overlays 永远在上）。
- 不做负 z-index 的特殊语义（只做数值排序）。

## Image 控件（占位实现）
### 为什么

M3.5 组件扩充只剩 `Image`。真实纹理采样依赖 M6 的资源管线（图片解码、上传、描述符、采样器），
但布局层现在就需要一个「图片位」：能声明尺寸/比例/来源，参与容器排布，在 UI 里占位与验收。
先给出稳定的公开接口，等 M6 把 `texture_id` 接到真实纹理时，控件本身不用改。

### API（`src/ui/image.hpp`，header-only）

- `Image(float width, float height)` / `Image(glm::vec2 size)`。
- `set_source(std::string)` / `source()`：图片路径，仅供 M6 加载使用，当前不读文件。
- `set_texture_id(std::uint32_t)` / `texture_id()`：预留纹理句柄，`0` 表示无纹理。
- `set_fit(Fit)` / `fit()`：`Fill` / `Contain` / `Cover`，当前只存储，M6 采样时生效。
- `set_tint(glm::vec4)` / `tint()`：叠加色，占位绘制用它着色。
- `set_placeholder_text(std::string)` / `placeholder_text()`：占位文案。
  未显式设置时取 `source()` 的文件名，仍为空则回退 `"image"`。
- 尺寸规格复用 `Widget`：`set_size_percent` / `set_aspect_ratio` / `min_size` / `max_size`。

### 绘制（占位）

- 圆角底色（`accent` 降低不透明度）+ 1px 边框（`panel_border`）。
- 对角交叉线，明确表示「尚未绑定纹理」。
- 居中占位文案（`small_text_size` + `text_muted`），裁剪在控件内。
- 即使 `texture_id != 0` 也保持同样的占位绘制——真实采样在 M6 接入，
  避免出现「看起来支持纹理、实际是空图」的误判。

### 验证

新建 `test/test_ui_image.cpp` 并注册：

- 构造尺寸、`measure`、`arrange` 后 bounds 正确。
- `source` / `texture_id` / `fit` / `tint` 存取。
- 未设置占位文案时：有来源取文件名、无来源回退 `"image"`。
- 在 `Vbox` 中声明 `size_percent` 与 `aspect_ratio` 时按父内容区解析。
- `append_to` 不崩（无字体上下文时静默跳过文字）。

gallery `Layout` 分区：两张占位图（一张有来源、一张空），验证百分比宽与 16:9。

### 边界（本次不做）

- 不做图片解码、纹理上传、采样（M6 资源管线）。
- 不做九宫格拉伸、平铺（tile）、mipmap、色彩空间处理。
- 不做图片缓存与异步加载。

## M3.6 平滑滚动与离散行滚动
### 为什么

`ScrollView` 的滚轮是「像素步进 + 立即生效」，`TreeView` / `Table` 是「行步进 + 立即生效」，
都缺少过渡：滚一格内容瞬移，长列表阅读时容易丢失位置感。需要两件事：

- **平滑滚动**：滚轮只改变目标偏移，实际偏移按时间常数指数逼近，帧率无关；
- **保留离散按行滚动**：需要「一格 = 一行」的精确步进时仍可切换。

### API

新增 `src/ui/scroll_mode.hpp`：`enum class ScrollMode { Smooth, Line }`。

`ScrollView`：

- `set_scroll_mode(ScrollMode)` / `scroll_mode()`，默认 `Smooth`；
- `set_scroll_smoothing(float seconds)` / `scroll_smoothing()`，默认 `0.12`，`0` = 立即到位；
- `set_scroll_speed(float pixels_per_step)`，默认 `40`（Smooth 模式每格像素）；
- `set_scroll_line_step(float pixels_per_line)`，默认 `40`（Line 模式每格像素）；
- `scroll_target()` 暴露目标值（测试与程序化查询）。

`TreeView` / `Table`：

- `set_scroll_mode(ScrollMode)` / `scroll_mode()`，默认 `Smooth`；
- `set_scroll_smoothing(float seconds)` / `scroll_smoothing()`，默认 `0.12`；
- 步进单位保持「行」：一格 = `row_height × 1.5`；模式只决定「立即 / 过渡」。

### 平滑算法

`offset += (target - offset) × (1 - exp(-dt / tau))`：

- 与帧率无关：同样的总时长，不同 dt 分割不影响收敛轨迹；
- 差值 < 0.25px 时吸附到目标，避免无限逼近；
- 全程 `clamp` 到合法范围（内容变短时目标同步收紧）。

`set_scroll_offset()` 是程序化跳转：同时设置 `offset` 与 `target`（立即生效）。
拖动滚动条、点击轨道翻页直接改 `offset` 并同步 `target`（跟手优先，不做过渡）。

### 性能

- 平滑更新只调用 `place_content()`（内容平移），**不** `mark_layout_dirty()`，
  避免每帧整树重排；
- `TreeView` / `Table` 的绘制按 `scroll_` 直接算行位置，同样不触发重排；
- `on_update(dt)` 由已有的 `UISystem::update_tree` 驱动，无需新定时器。

### 验证

新建 `test/test_ui_smooth_scroll.cpp` 并注册：

- Smooth：`on_scroll` 后 `scroll_offset()` 不变、`scroll_target()` 变化；`update(dt)` 后单调逼近；
- 连续 `update` 后精确等于目标（吸附）；
- 帧率无关：同样总时长、不同 dt 分割，最终都吸附到同一目标；
- Line：`on_scroll` 后立即等于目标；
- clamp：目标不超出 `[0, max_scroll]`；
- `set_scroll_offset` 立即生效且同步 target；
- `Table`：Smooth 延迟、Line 立即（用 `set_rows` 造数据）。

gallery：新增 `COSINE_GALLERY_SCROLL_MODE=line` 切到离散行滚动，默认 `smooth`。

### 边界（本次不做）

- 不做惯性（flick）与速度衰减：只做「目标跟随」，不引入初速度模型。
- 不做滚动条拖动时的惯性/回弹/边缘橡皮筋。
- 不做触摸板亚像素 delta 的特殊处理（按同一比例缩放）。

## Rect 未初始化导致的 NaN 布局
### 现象

`ui_gallery` 的 Containers 分区只显示面板背景与顶部内阴影，内容（Splitter / TabView / Table）
全部不可见；其它分区里含 Table 的地方同样受影响。

### 根因

`Rect` 的 `pos` / `size` 没有默认成员初始化，而 `glm::vec2` 的默认构造**不写零**
（未定义 `GLM_FORCE_CTOR_INIT`）。因此 `Widget::global_` / `local_` 在首次 arrange 之前是未定义值。

`Table::measure` 直接读 `global_.size`：

```cpp
return clamp_size(global_.size.x > 0.0f ? global_.size : local_.size);
```

首次测量时读到未定义值（NaN）→ `glm::clamp(NaN, …)` 仍是 NaN → 表格尺寸 NaN →
父容器 `Vbox` 的内容高 NaN → `ScrollView::place_content` 把内容按 NaN 尺寸 arrange →
`ScrollPanel` 的 bounds 高度 NaN → 视口裁剪为空 → 只剩面板自身的背景与内阴影。

最近的布局改动给 `Widget` 增加了字段（`absolute_` / `z_index_` / `size_percent_` / `aspect_ratio_`），
改变了对象布局，使 `global_` 落在非零内存上，才把这个潜在缺陷暴露出来。

### 修复

1. `Rect` 显式初始化：`glm::vec2 pos{0.0f, 0.0f}; glm::vec2 size{0.0f, 0.0f};`
   —— 根本修复，所有控件的 `global_` / `local_` 在首次 arrange 前为 0。
2. `Table::measure` 改为 `clamp_size(min_size())`，尺寸由构造时设置的 min/max 决定，
   不再依赖 arrange 的执行顺序。

### 验证

- Containers 分区截图：Splitter 双栏、TabView 三页、Table 表头与行均正常显示。
- `ctest --test-dir build`：41/41；`ctest --test-dir build-asan`：41/41。
- gallery 冒烟：Containers、Layout(classic)、Overlays(line) 三组 `validation=0`。

### 边界

- 不改 `glm::vec2` 的全局行为（不定义 `GLM_FORCE_CTOR_INIT`），只在 `Rect` 层显式初始化。
- 不重写 `Dropdown` 内部列表的 `measure`（它返回 `local_.size`，但 arrange 会先 `set_size`，
  当前测试与冒烟均正常）。

## 第三种滚动模式：Fast（停止输入后快速收尾）
### 为什么

`Smooth` 的时间常数是 0.12s，偏柔和：滚轮停止后仍有明显「滑行尾巴」。
需要一种「一停就定」的手感，同时保留平滑过渡（而不是 `Line` 的瞬时跳变）。

### 设计

`ScrollMode` 增加 `Fast`：

- 基础时间常数 = `max(0.03, smoothing × 0.4)`（默认约 0.048s，Smooth 的 40%）；
- 吸附阈值 0.5px（Smooth 是 0.25px）；
- **停止输入超过 0.1s 后时间常数再减半**（约 0.024s），视觉上快速定住。

`idle_` 由控件的 `on_scroll` 清零、`on_update` 累加，不需要外部时间源，
也不依赖 `UISystem::elapsed_`。

### 接入

- `ScrollView` / `TreeView` / `Table` 三个滚动容器都支持 `Fast`。
- gallery Style 分区增加 `Fast` 按钮，与 `Smooth` / `Line` 并列；
  `COSINE_GALLERY_SCROLL_MODE=fast` 同样可选。

### 验证

- `test_ui_smooth_scroll` 新增用例：同样 0.096s，`Fast` 的位移大于 `Smooth`；
  再 0.64s 后 `Fast` 精确吸附到目标。
- `ctest` 41/41（build 与 ASAN）。
- gallery 冒烟与截图（Style 分区三个按钮）。

### 边界

- 不引入初速度 / 惯性模型；三种模式仍是「目标跟随」。
- 不改变 `Smooth` 与 `Line` 的既有参数与语义。

## M4 起手：缓动库
### 为什么

M4 动画内核的第一块地基：关键帧之间需要「进度 → 进度」的曲线。
UI 的 `lerp_style` 用的固定 `ease_out_quad` 不足以覆盖 M4 的需求（cubic-bezier 是时间轴编辑器的
标准配置项），而且动画模块不应依赖 UI。先做一个独立、可纯逻辑测试的缓动库。

### 设计

新建 `src/anim/` 与静态库 `cosine_anim`（只依赖 glm，不依赖 UI），后续 `scene` / `timeline` /
`property` 都放进这个库。

`easing.hpp`：

```cpp
enum class Easing { Linear, EaseIn, EaseOut, EaseInOut, CubicBezier };

struct CubicBezier {   // 控制点 (0,0) (x1,y1) (x2,y2) (1,1)
    float x1, y1, x2, y2;
    float operator()(float x) const;   // 时间 → 进度
};

float ease(Easing, float t);
float ease(Easing, float t, const CubicBezier& custom);
```

- `x1` / `x2` 求值时 clamp 到 `[0,1]`：保证 `x(s)` 单调，才能求逆。
- 求值用二分法（24 次迭代，精度约 1e-7）；输入 `t` 先 clamp 到 `[0,1]`。
- 预置：`kEaseIn = (0.42, 0, 1, 1)`、`kEaseOut = (0, 0, 0.58, 1)`、
  `kEaseInOut = (0.42, 0, 0.58, 1)`（CSS 标准值）。
- `Easing::CubicBezier` 用传入的 `custom`；其它枚举走预置曲线。

### 验证

新建 `test/test_anim_easing.cpp` 并注册：

- 所有模式：`f(0) == 0`、`f(1) == 1`；输入越界被 clamp。
- `Linear` 恒等。
- 单调性：对每个模式，`t` 递增时结果不减（采样 100 点）。
- `EaseOut` 在中点大于 `EaseIn`（前者先快）。
- 自定义 cubic-bezier：`(0.25, 0.1, 0.25, 1)` 与预置曲线不同，且求解满足
  `|x(s) − x| < 1e-4`（通过 `operator()` 与已知点对比）。
- 极端控制点（`x1 = x2 = 0` / `= 1`）不产生 NaN。

### 边界（本次不做）

- 不做弹簧 / 物理缓动（M4 只需要曲线）。
- 不做缓动的编辑器 UI（M6）。
- 不替换 UI 里已有的 `ease_out_quad`（那是 UI 状态过渡的专用曲线）。

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

## 动画演示 example + 绝对定位漂移修复
### 为什么

M4 的 `scene` 还没有渲染路径，但 `easing` 已经可用。需要一个能直观看到「缓动差异」的程序：
同一时间、五条曲线驱动的方块位置并列对比，同时验证现有 UI 控件承载动画的能力。

### example

`examples/ui_anim_demo/main.cpp`（target `ui_anim_demo`，链接 `cosine_app` + `cosine_anim`）：

- 舞台 `Panel` 内 5 条轨道：`Linear` / `EaseIn` / `EaseOut` / `EaseInOut` / 自定义 cubic-bezier，
  每条一个绝对定位的 `ColorRect`，位置 = `origin + ease(t) × span`。
- 控制：`Pause` / `Play` 切换、`Reset` 归零；`Progress` 显示当前归一化时间，`Label` 显示秒数。
- 时间用真实 `frame_time_ms` 推进，周期 3s 循环。

### 顺带修复：绝对定位位置漂移

`absolute` 子项的 `x()/y()` 是 `local_.pos`，而 `arrange` 会把它改写成**全局坐标**；
容器下一帧又用 `inner_x + child->x()` 计算，导致每帧累加父偏移，方块一路飘出屏幕。

修复：

- `Widget` 增加 `absolute_pos_`（相对父内容区的偏移）与 `set_absolute_pos()` / `absolute_pos()`；
  `set_absolute(true)` 时以当前 `local_.pos` 作为初始偏移（兼容旧用法）。
- `Vbox` / `Hbox` / `Grid` / `Wrap` 的绝对定位分支改用 `absolute_pos()`，不再读 `x()/y()`。
- `ColorRect` 增加 `set_color()`（示例按曲线着色，也便于后续动画）。

### 验证

- `test_ui_absolute_zindex` 新增用例：连续 4 次 `arrange` 后绝对定位子项位置不漂移。
- `ctest` 43/43（build 与 ASAN）。
- `ui_anim_demo` 冒烟 `validation=0`；截图确认 5 条轨道、进度条与时间标签。
- gallery 冒烟未受影响。

### 边界

- 不做 scene 渲染（M4 后续）；示例只用现有 UI 控件。
- 不做时间轴拖动 seek（先用 Pause/Reset；seek 属于 M4 的 player）。

## 退出时 Vulkan 资源在 pending 状态被销毁
### 现象

`ui_anim_demo` 正常关窗后输出大量校验错误：

```
vkFreeMemory(): can't be called on VkDeviceMemory ... currently in use by VkBuffer ... in use by VkCommandBuffer
vkDestroyFence(): ... currently in use by VkQueue
vkDestroySemaphore(): ... currently in use by VkQueue
vkFreeCommandBuffers(): pCommandBuffers[0] ... is in use
vkDestroySwapchainKHR(): ... currently in use by VkQueue
```

### 根因

`Renderer::~Renderer() {}` 是空析构。`swapchain`、vertex/uniform buffer 及其内存、
命令缓冲、fence/semaphore 都是 `Renderer` 的成员，在析构体之后按逆序释放；
此时 GPU 可能仍在执行上一帧提交，校验层因此判定「资源仍在使用」。

### 为什么之前没发现

所有冒烟都用 `timeout 5`（SIGTERM 强杀），进程来不及跑析构，校验层自然不会报。
这是测试盲区：**只有正常退出才会暴露资源释放问题**。

### 修复

1. `Renderer::~Renderer()` 开头调用 `context_.graphics_queue().waitIdle()`。
2. `ui_anim_demo` 与 `ui_gallery` 增加 `COSINE_EXIT_AFTER=<秒>` 钩子：
   到时 `glfwSetWindowShouldClose`，让进程走正常退出路径；CI 冒烟可以改用它，替代 `timeout` 强杀。

### 验证

- `COSINE_EXIT_AFTER=1 ./ui_anim_demo`：exit=0，`VUID-|validation (error|warning)` 计数 **0**。
- `COSINE_EXIT_AFTER=1 ./ui_gallery`：exit=0，计数 **0**。
- `ctest` 43/43（build 与 ASAN）。

### 边界

- 不做逐资源 fence 等待；退出路径整体 `queue.waitIdle()` 足够。
- 不改变运行时的每帧同步策略。
- 不处理 swapchain 重建路径（`recreate()` 内部已有 `waitIdle`）。

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
