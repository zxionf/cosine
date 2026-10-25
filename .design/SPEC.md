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
- 不改多行输入布局；不引入基线对齐的自定义策略。

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

## 菜单可见性修复
1. **菜单项文字垂直居中**：行内文字原来用 `text_size`（16）当高度参与居中，
   实际行盒是 `line_height`（≈23），导致文字整体偏下。改为按 `FontAtlas::line_height(size)` 居中。
   同一问题也存在于单行 `TextInput`（`y + (h - font_size)/2`），一并改为按行高居中。
2. **弹层上下留白对称**：`popup_height` 原来只加了一次 `padding`（顶部留白、底部贴边），
   改为 `padding * 2`，最后一行与底边之间有空隙。
3. **悬停切换菜单后内容不更新**：`MenuPopup` 在创建时固定了 `menu_index_`，
   切换菜单只更新了几何尺寸。新增 `MenuPopup::set_menu_index()`，
   在 `update_popup_geometry()` 里同步，内容随标题切换。

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
