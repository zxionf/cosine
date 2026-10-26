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
