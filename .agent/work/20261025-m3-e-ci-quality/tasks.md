# 20261025-m3-e-ci-quality 任务清单

## M3-E CI 与质量门

- [x] E01 `COSINE_SANITIZE` + `cosine_apply_sanitizers`
- [x] E02 `ci-linux.sh` 支持 `COSINE_BUILD_DIR` / `COSINE_SANITIZE`
- [x] E03 新增 `smoke-lavapipe.sh`（无环境时 skip）
- [x] E04 本地 ASAN 构建 + ctest + ui_demo 冒烟
- [x] E05 本地 lavapipe 冒烟
- [x] E06 新增 `.github/workflows/ci.yml`（Linux + Windows）
- [x] E07 全量回归与文档更新
## 当前进度

- sanitizer 选项、ci-linux 参数扩展、lavapipe 冒烟脚本、GitHub Actions 矩阵已落地
- 修复 interpreter 环境泄漏与 token 测试泄漏（ASAN 全绿）
- 13/13 ctest；ASAN 13/13；lavapipe 冒烟通过；无显示环境正确 skip
