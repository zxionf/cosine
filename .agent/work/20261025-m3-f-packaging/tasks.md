# 20261025-m3-f-packaging 任务清单

## M3-F 打包

- [x] F01 CMake 项目版本 + 生成 `version.hpp` + `--version`
- [x] F02 `scripts/package-linux.sh`（Release 构建、strip、tar.gz）
- [x] F03 `scripts/package-windows.ps1`（Release 构建、zip）
- [x] F04 打包目录布局与资源复制（assets/shaders/fonts、VERSION、run.sh）
- [x] F05 验证：从 dist 目录、任意 cwd 运行；`--version` 正确
- [x] F06 文档：`docs/packaging.md`（字体/资源/依赖说明）
- [x] F07 全量回归（14/14，含 test_version）

## 当前进度

- 版本 0.3.0 由 CMake 生成 version.hpp；ui_demo --version 输出 cosine 0.3.0
- Linux 打包产出 dist/cosine-0.3.0-linux-x86_64/（2.0MB）与 tar.gz（876KB），strip 后从任意 cwd 可运行
- Windows 打包脚本已就绪，依赖真实 Windows 环境验证
- 14/14 ctest 通过（新增 test_version）
