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
