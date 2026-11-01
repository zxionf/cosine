# 编译单元基线（P1）

## 环境

| 项 | 值 |
| --- | --- |
| 编译器 | GCC（`/usr/bin/g++`） |
| 构建类型 | Debug |
| 并行度 | `-j8` |
| ccache / PCH / unity | 均未启用 |
| 机器 | 32 核 |

## 基线数字

| 指标 | 值 |
| --- | --- |
| `.cpp` 文件 | 101 |
| `.hpp` 文件 | 90 |
| 编译单元（`compile_commands.json`） | 191 |
| clean build | **65.57 s**（`ninja -C build -j8`） |
| 构建步骤 | 317 |

## 热点头

| 头文件 | 直接被 include 次数 | 依赖它的编译单元（`ninja -t deps`） |
| --- | ---: | ---: |
| `ui/widget.hpp` | 40 | **65** |
| `ui/theme.hpp` | 36 | **65** |
| `ui/root.hpp` | 31 | **37** |
| `ui/core/label.hpp` | 25 | — |
| `ui/containers/vbox.hpp` | 25 | — |
| `ui/ui_system.hpp` | 23 | — |
| `ui/style.hpp` | 23 | — |
| `graphics/box.hpp` | 23 | — |
| `graphics/font_atlas.hpp` | 21 | — |
| `ui/core/button.hpp` | 17 | — |
| `graphics/rectangle.hpp` | 17 | — |

## 增量重编译

方法：先 `ninja -C build -j8` 回到最新，然后 `touch <header>`，计时一次重建。

| 头文件 | 重建时间（`-j8`） | 说明 |
| --- | ---: | --- |
| `ui/widget.hpp` | 46.14 s | 触发约 65 个编译单元 |
| `ui/theme.hpp` | 45.71 s | 触发约 65 个编译单元 |
| `ui/root.hpp` | 30.41 s | 触发约 37 个编译单元 |

时间包含 ninja 的 CMake `VerifyGlobs` 重跑开销，但主要成本是重新编译依赖该头的 TU。

## 结论

- 当前 clean build 约 65.6 s；改一个核心 UI 头会重编译 65 个 TU，增量接近 46 s。
- `ui/widget.hpp` 和 `ui/theme.hpp` 是最大扇出点；两者都直接/间接进入几乎所有 UI、cosine、test TU。
- 优化优先级：

```
P2  ui/widget.hpp
P3  ui/theme.hpp / ui/root.hpp
P4  IWYU（label/button/font_atlas/box/rectangle 的前置声明与去 include）
P5  PCH（库 + 代表性 test）
```

## 目标

| 项 | 基线 | 目标 |
| --- | ---: | ---: |
| `widget.hpp` 依赖 TU | 65 | ≤ 32（-50%），理想 ≤ 10 |
| `theme.hpp` 依赖 TU | 65 | ≤ 32（-50%） |
| `root.hpp` 依赖 TU | 37 | ≤ 18（-50%） |
| 改 `widget.hpp` 重建时间 | 46.1 s | 显著下降（同时依赖 TU 下降） |
| clean build | 65.6 s | 记录 P6 对比，PCH 后评估 |

## 测量命令

```bash
# clean build
ninja -C build -t clean
start=$(date +%s.%N)
ninja -C build -j8
end=$(date +%s.%N)

# 热点头重编译
touch src/ui/widget.hpp
ninja -C build -j8

# 依赖 TU 数
ninja -C build -t deps | grep -c 'src/ui/widget.hpp'
```

## P5 PCH / OBJECT / ccache 实验

- PCH：`cosine_ui` + 代表性 test（`test_ui_layout`、`test_ui_controls`、`test_text`），预编译标准库、glm、`ui/widget.hpp`、`ui/theme.hpp`。
- OBJECT 复用：`interpreter_test_support` 把 `token.cpp` / `lexer.cpp` 编译一次，供 3 个 interpreter test 共享。
- ccache：当前环境未安装；根 `CMakeLists.txt` 已加 `find_program(ccache)`，安装后自动启用。

clean build（`-j8`）：

| 配置 | 时间 |
| --- | ---: |
| 基线 | 65.57 s |
| PCH + OBJECT | 58.59 s |
| PCH + OBJECT（复测） | 58.74 s |

约 **-10.6%**。保留 PCH（库 + 3 test）与 OBJECT 复用；ccache 等安装后再评估。
