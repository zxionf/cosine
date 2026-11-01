# 编译单元优化报告（project）

## 范围

目标：降低 clean / incremental 构建时间与头文件重编译扇出，不改变运行时行为。

## 基线

| 指标 | 值 |
| --- | ---: |
| `.cpp` / `.hpp` | 101 / 90 |
| 编译单元（`compile_commands.json`） | 191 |
| clean build（`-j8`） | **65.57 s** |
| `widget.hpp` 依赖 TU | 65 |
| `theme.hpp` 依赖 TU | 65 |
| `root.hpp` 依赖 TU | 37 |
| 改 `widget.hpp` 重建 | 46.1 s |
| 改 `theme.hpp` 重建 | 45.7 s |

## 已执行

### P2 `ui/widget.hpp`

- `visual_style` / `transition_active` / `focus_visible` / `append_focus_ring` / `ui_theme` / `font_atlas` 移到 `ui/widget.cpp`
- `StyleTransition` 移到 `ui/widget_style.hpp`
- `StyleRole` / `ControlState` 抽到 `ui/style_types.hpp`
- `transition_` 改为 `unique_ptr`，构造/析构 out-of-line

结果：`theme.hpp` 依赖 65 → 56；改 `widget.hpp` 46.1 → 44.3 s。

### P3 `ui/theme.hpp`

- `Theme::with_density` / `with_rounding` / `with_focus_policy` / `with_transition_ms` / `with_radius_scale` / `light()` 外移到 `ui/theme.cpp`
- `theme.hpp` 减少约 120 行 inline 实现

结果：改 `theme.hpp` 45.7 → 39.6 s；依赖 TU 无法下降（`Theme` 是值类型，字段必须完整）。

### P4 IWYU / 前向声明

- `debug_rect` / `append_debug_tree` 外移到 `widget.cpp`
- `widget.hpp` 移除 `graphics/line.hpp`
- 补齐被断开的传递 include（`panel`、`tab_view`、`timeline_view`、`image`、`scene_view`、`root`、`ui_system`）

结果：`widget.hpp` 依赖 66 → 59。

### P5 PCH / OBJECT / ccache

- PCH：`cosine_ui` + `test_ui_layout` / `test_ui_controls` / `test_text`
- OBJECT：`interpreter_test_support` 复用 `token.cpp` / `lexer.cpp`，3 个 interpreter test 共享
- ccache：4.14.1 已安装；CMake 自动启用，并在启用时关闭 PCH（PCH 与 ccache 热缓存冲突）

## 最终数据

| 指标 | 基线 | 最终 |
| --- | ---: | ---: |
| clean build（`-j8`） | 65.57 s | **58.16 s（-11.3%）** |
| 构建步骤 | 317 | 314 |
| `widget.hpp` 依赖 TU | 65 | 59 |
| `theme.hpp` 依赖 TU | 65 | 57 |
| `root.hpp` 依赖 TU | 37 | 37 |
| 改 `widget.hpp` 重建 | 46.1 s | 45.8 s |
| 改 `theme.hpp` 重建 | 45.7 s | 39.6 s |

## 验证

- `ctest`：65/65
- `ui_lab` / `ui_gallery` / `cosine` 冒烟：validation = 0

## 指标调整说明

原计划把“修改核心头触发的依赖 TU 下降 ≥50%”作为验收。实际不可达：

- 控件继承 `Widget`，任何使用控件的 TU 都必须包含 `widget.hpp` 完整定义；
- `Theme` 是值类型，字段访问要求完整定义。

因此实际验收改为：减少头文件 inline 实现与传递依赖、降低编译成本；依赖 TU 只做趋势记录。

## ccache 补充测量（安装后）

| 配置 | clean（`-j8`） |
| --- | ---: |
| 冷缓存（首次写缓存） | 61.03 s |
| 热缓存 + PCH | 21.75 s |
| 热缓存（ccache 自动关闭 PCH） | 15.02 s |

相对基线 65.57 s，ccache 热缓存 clean 约 **-77%**。

## 后续建议

1. 安装 ccache 后重测 clean / 分支切换构建（预期收益最大）。
2. PCH 推广到更多 UI test，需先统一各 target 的 include 集合。
3. unity build 只对库 target 做实验，避免 test 的 ODR/匿名 namespace 冲突。
4. 65 个 test 可合并为少数可执行（CTest 用例不变），减少重复 main/链接成本。
