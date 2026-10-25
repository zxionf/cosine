# 20261025-ci-fixes 任务清单

- [x] T01 定位 Linux 失败原因：`--strip-components=1` 与归档布局不匹配
- [x] T02 改为解压后 `find` 定位 `slangc`，失败时打印归档结构
- [x] T03 Windows 改用 `ilammy/msvc-dev-cmd@v1` + Ninja
- [x] T04 显式解析并导出 `VULKAN_SDK`
- [x] T05 两个 job 打印 CMake/Ninja/编译器版本
- [x] T06 PowerShell 检查 `$LASTEXITCODE`
- [x] T07 本地校验：YAML 可解析；两种归档布局的 `find` 逻辑均可定位 slangc
- [ ] T08 远端 CI 首次通过（需要在 GitHub 上重跑，本地无法验证）

## 当前进度

- 本地校验完成；等待远端重跑确认

## 第二轮（CMake 版本 / Vulkan 头文件）

- [x] T09 定位：Linux runner CMake 3.31.6 < 项目要求的 4.0.0
- [x] T10 新增 `Ensure CMake >= 4` 步骤：已满足则跳过，否则下载固定版本到 `/opt` 并链接
- [x] T11 定位：`vertex.hpp` 公开包含 `<vulkan/vulkan.hpp>`，`cosine_text` 缺少头文件路径（Windows C1083）
- [x] T12 `cosine_text` 以 PUBLIC 链接 `Vulkan::Headers`
- [x] T13 本地验证：重新配置/编译/ctest 通过，YAML 可解析
- [ ] T14 远端 CI 重跑确认
