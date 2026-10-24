# 工具与环境

## sele CLI

- 版本 0.2.2，已在 PATH（`D:\Program Files\sele\bin\sele.exe`）。
- 命令文档见 `sele0.2.2.md`；常用：
  - `sele status` 查询活跃变更与决策数
  - `sele lint` 检查 ID 一致性、决策只追加、SPEC 禁过程词、目录完整性
  - `sele new --slug <name>` 创建变更（同时生成 `.design/changes/<id>/design.md` 与 `.agent/work/<id>/tasks.md`）
  - `sele append-decision --context <c> --decision <d> --consequences <e>` 追加决策
  - `sele close <id> --dry-run` 预览折叠，去掉 `--dry-run` 执行
- 在仓库根目录执行；不在根目录时加 `--root <path>`。

## 环境

- Linux 开发机：Wayland 会话，NVIDIA RTX 4060 Laptop，`vulkaninfo` 可用。
- 编译器：g++（C++20）；构建：CMake + Ninja。
- sele：Linux 下为 `~/.local/bin/sele`，版本 0.2.2。
- Windows 侧目标：MSVC + Vulkan SDK，M1 起纳入构建验证。

## 踩坑

- 用户明确：任何时候都不允许使用 Python 脚本；文件修改只用编辑工具或必要 shell 命令。
- 旧笔记里的 sele 路径是 Windows 的 `D:\Program Files\sele\bin\sele.exe`；Linux 下使用 `~/.local/bin/sele`（已在 PATH）。
- `slangc` 不在 PATH 时，`find_program` 会在 `$HOME/tools/slang/bin` 查找；`glslc` 使用 Vulkan SDK 的 `/usr/bin/glslc`。
- `scripts/ci-linux.sh` 会优先复用 `build/_deps/{glfw,freetype,stb}-src`，离线环境下也能完成全新 `build-ci/` 构建。
- `ex4` 的运行资源从 `assets/` 解析：`COSINE_ASSET_DIR` -> 可执行文件旁 `assets/` -> 构建期注入目录，因此不再依赖当前工作目录。
- 全量 `cmake --build build` 当前会因 `src/main.cpp` 引用已删除的 `first_app.hpp`（`ex0`）失败；M1 已删除 `ex0` 与 `main.cpp`。
