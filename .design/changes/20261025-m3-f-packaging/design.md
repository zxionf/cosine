# 20261025-m3-f-packaging

<!-- 为什么改、边界与不属于本次的范围 -->

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

## 边界

- 不做安装器（deb/msi）、不做签名、不做自动更新。
- 不修改 README（由用户维护）；打包会原样复制。
