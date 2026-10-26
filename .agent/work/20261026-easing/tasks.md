# 20261026-easing 任务清单

- [x] T01 新建 `src/anim/` 与 `cosine_anim` 静态库（CMake）
- [x] T02 `anim/easing.hpp`：`Easing` / `CubicBezier` / `ease` 与预置曲线
- [x] T03 `anim/easing.cpp`：二分求逆 + clamp + 枚举分派
- [x] T04 `test/test_anim_easing.cpp` + 注册到 `test/CMakeLists.txt`
- [x] T05 回归：ctest 42/42（build + ASAN）
- [x] T06 SPEC 折叠（先 `--dry-run` 确认 appended）、决策记录

## 完成定义

- `ease()` 对五种模式在 `t=0/1` 精确返回 0/1，曲线单调，无 NaN。
- 自定义 cubic-bezier 的求逆精度优于 1e-4。
- 不引入 UI 依赖，测试独立可跑。
