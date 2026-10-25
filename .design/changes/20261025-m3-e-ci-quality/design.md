# 20261025-m3-e-ci-quality

<!-- 为什么改、边界与不属于本次的范围 -->

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

## 边界

- 不引入 GPU 硬性依赖的 CI 用例；lavapipe 仅做启动与 validation 冒烟。
- 不做缓存优化、不做多编译器矩阵（clang 留待后续）。
