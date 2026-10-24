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
