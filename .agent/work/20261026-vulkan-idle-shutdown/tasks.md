# 20261026-vulkan-idle-shutdown 任务清单

- [x] T01 `Renderer::~Renderer()` 开头 `queue.waitIdle()`
- [x] T02 `ui_anim_demo` / `ui_gallery` 增加 `COSINE_EXIT_AFTER` 正常退出钩子
- [x] T03 验证：两个 example 正常退出后 `VUID/validation` 计数为 0
- [x] T04 回归：ctest 43/43（build + ASAN）
- [ ] T05 SPEC 折叠（`--dry-run` 确认 appended）、决策记录

## 完成定义

- 正常关窗退出不再产生 Vulkan 校验错误。
- 冒烟可以走正常退出路径，覆盖析构代码。
- 全量测试与 ASAN 通过。
