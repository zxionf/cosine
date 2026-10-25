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
- UTF-8 解码，至少 ASCII + 常用 CJK。
- 字形 advance、line height、文本宽度、自动换行、裁剪与省略号。
- 字体图集格式文档化；CJK 通过可配置字体路径（系统字体或仓库字体）经 FreeType 生成，具体字体来源在文本子阶段决策。
- 后续可评估 SDF / FreeType 直接排版，但不在 M2 强制。

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
- 不改变控件所有权模型（仍由父容器 `unique_ptr` 拥有）。
- 不引入全局控件注册表，避免新增全局可变状态。
- 令牌只用于交互目标校验，不用于控件间通信。

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
- `test_asset_path` 覆盖找到/缺失/搜索目录和 `require_asset` 文本。
- 现有测试保持通过；Vulkan 错误通过手动/冒烟验证。

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
- 冒烟依赖窗口系统；CI 用 Xvfb/xvfb-run 提供显示。

## CI 矩阵
- 新增 `.github/workflows/ci.yml`：Linux（apt 安装 Vulkan/lavapipe/xvfb，构建、ctest、ASAN 构建、lavapipe 冒烟）与 Windows（MSVC + Vulkan SDK，构建、ctest）。
- 复用 `scripts/ci-linux.sh` / `scripts/ci-windows.ps1`，避免 CI 与本地命令分叉。
- Windows 首次在真实 CI 上验证；本地只保证脚本与 CMake 可配置。

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
