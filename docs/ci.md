# 构建与 CI

## 本地命令

```bash
# 常规构建 + 测试（复用 build/_deps，避免重复 clone 依赖）
./scripts/ci-linux.sh

# 自定义并行度 / 构建目录
COSINE_JOBS=8 COSINE_BUILD_DIR=build-local ./scripts/ci-linux.sh

# AddressSanitizer 构建 + 测试
COSINE_SANITIZE=address COSINE_BUILD_DIR=build-asan ./scripts/ci-linux.sh
```

`COSINE_SANITIZE` 支持 `address`、`undefined`、`thread`（可用分号组合）。
它只作用于本项目 target，不重新编译第三方依赖。

## 冒烟（正常退出）

示例都支持 `COSINE_EXIT_AFTER=<秒>`：到点后调 `glfwSetWindowShouldClose`，
走正常退出路径（覆盖析构与 Vulkan 资源释放）。

```bash
cd build/examples
COSINE_EXIT_AFTER=3 ./ui_gallery 2>&1 | grep -Ei 'VUID-|validation (error|warning)'
```

计数为 0 即通过。**不要用 `timeout` 强杀**——会跳过析构，掩盖资源释放问题。

不接该变量的示例只有 `ui_demo`（手工关窗或用 `timeout` 兜底）。
各示例的按键与专属环境变量见 `examples.md`。

高 DPI 冒烟：

```bash
COSINE_UI_SCALE=2 COSINE_EXIT_AFTER=3 ./ui_gallery
```

验证重点是界面整体放大、字形清晰（按 `字号×比例` 重新栅格化）、
tooltip 与应用内命中位置不偏移。

当前测试规模：`ctest` 共 **51** 个用例（含 ASAN 构建，同一份源码）。

## lavapipe 冒烟

`scripts/smoke-lavapipe.sh` 用 Mesa 软件 Vulkan 驱动依次运行 `ui_demo` 与 `ui_gallery`，
并检查日志中不出现 Vulkan validation 错误。

```bash
# 系统已安装 lavapipe（mesa-vulkan-drivers 提供 lvp_icd.*.json）
./scripts/smoke-lavapipe.sh

# 指定 ICD 或裸 .so 所在目录
COSINE_LAVAPIPE_ICD=/path/lvp_icd.json ./scripts/smoke-lavapipe.sh
COSINE_LAVAPIPE_LIB_DIR=/path/to/lib ./scripts/smoke-lavapipe.sh

# 调整时长
COSINE_SMOKE_SECONDS=30 ./scripts/smoke-lavapipe.sh
```

没有可用的 ICD 或没有显示环境（`WAYLAND_DISPLAY`/`DISPLAY`）时脚本会 skip 并返回 0。

## 远端 CI

当前仓库不维护 GitHub Actions：`.github/workflows/` 已移除（2026-10-26）。
质量门使用上面的本地命令（`scripts/ci-linux.sh`、`scripts/smoke-lavapipe.sh`）。

发布阶段重建工作流时，先按当时的 runner 工具链核对以下几点：

- 编译器版本满足 `COSINE_CXX_MODULE_SCAN` 门槛（GCC 14 / Clang 16 / MSVC 19.34）。
- Linux 发行版自带的 Vulkan 头可能过旧（例如 Ubuntu 24.04 只有 1.3.275）；
  需要时安装官方 Vulkan-Headers，并用 `-DVulkan_INCLUDE_DIR` 指定。
- Windows 上 MSVC 必须开启 `/utf-8`，否则源码中的 `U'中'` 会被当成多字符常量（C2015）。
- Slang 发行包布局随版本变化：解压后用 `find` 定位 `slangc`，不要假设 `--strip-components` 层级。
- Windows 用 `ilammy/msvc-dev-cmd` + `-G Ninja`，并显式解析 `C:\VulkanSDK\<version>` 写入 `VULKAN_SDK`。
- CMake 需要 >= 4.0.0（项目 `cmake_minimum_required`），镜像版本不足时安装固定版本。

Slang 版本在重建时固定在工作流顶部的 `SLANG_VERSION`，升级时同步更新。

### C++ 模块扫描

`src/backend/vulkan/*.hpp` 在定义了 `USE_CPP20_MODULES` 时用 `import vulkan_hpp;`，
因此相关 target 需要 `CXX_SCAN_FOR_MODULES ON`。但 **只能** 在编译器提供
import graph 扫描时打开，否则 CMake 生成阶段直接报错：

```text
The target named "cosine_backend" has C++ sources that may use modules, but
the compiler does not provide a way to discover the import graph dependencies.
```

顶层 `CMakeLists.txt` 按编译器版本判断 `COSINE_CXX_MODULE_SCAN`：

| 编译器 | 最低版本 |
| --- | --- |
| GCC | 14 |
| Clang | 16 |
| MSVC | 19.34（VS 17.4） |

不满足时相关 target 不开启扫描（当前代码也未定义 `USE_CPP20_MODULES`，
走 `#include <vulkan/vulkan_raii.hpp>` 回退路径）。

### Vulkan 头文件

`src/graphics/vertex.hpp` 公开包含 `<vulkan/vulkan.hpp>`，因此 `cosine_text`
以 `PUBLIC` 链接 `Vulkan::Headers`，把 Vulkan 头文件路径传递给所有消费者。
Linux 上系统头文件默认可见，Windows 上必须由 `find_package(Vulkan)` 提供，
否则 `cosine_text`（字体/文本库）会因为找不到 `vulkan/vulkan.hpp` 编译失败。

## 测试分层与质量门

### 测试分层

- **纯逻辑测试**（不依赖 Vulkan/窗口）：布局（Vbox/Hbox/Grid/Wrap/百分比/绝对定位/z-index）、
  命中测试、事件冒泡、文本测量与图集、时间轴求值与播放器事件、缓动、场景图、属性系统。
- **解释器测试**：沿用手工用例（token、AST、求值），XEL 绑定后增加宿主 API 测试。
- **渲染冒烟**：`COSINE_EXIT_AFTER` 正常退出 + validation 计数为 0（见上节）。
- **Windows**：MSVC 构建 + 单元测试（窗口级冒烟待补）。

### 质量门

- `-Wall -Wextra -Wshadow` 无新增警告。
- Linux 跑 ASAN 与 lavapipe 冒烟；Debug + `VK_LAYER_KHRONOS_validation` 无错误。
- 资源缺失、shader 编译失败、初始化失败都有明确错误信息。
- 每个阶段出口必须可运行、可验证；不接受“只写了代码但跑不起来”的交付。

### 性能

M1–M3 只做 instrumentation（frame time、draw call、顶点数），不设硬指标；
M6 之后有了真实项目再定预算。

## 已知限制

- 没有远端 CI，跨平台验证依赖本地脚本；Windows 只做构建与单元测试，没有窗口级运行冒烟。
- lavapipe 冒烟只验证启动与 validation 干净，不做像素比对。
- 未启用 clang / 多编译器矩阵。
