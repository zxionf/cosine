# 20261024-m1-stability 任务清单

## P0 正确性

- [x] T01 `Align`：成员改回 `stretch_x/stretch_y`，工厂改名 `h_stretch/v_stretch`，修正 `Vbox`
- [x] T02 `ScrollView::set_content` 去掉显式析构
- [x] T03 `ScrollView` 暴露 `children()`，修正 `append_to` 空指针
- [x] T04 `ScrollView` 滚轮方向与 offset clamp（渲染裁剪栈延后到 M2，见决策 #12）
- [x] T05 `DrawBatch` 纹理槽位；`Renderer` 按批次绑定 `WHITE/TEXTURE`
- [x] T06 `Rectangle` / `Triangle` 索引基址改 `uint32_t`
- [x] T07 `DrawBatch::scissor` 显式初始化
- [x] T08 GLFW user pointer 统一 + resize 转发
- [x] T09 `MouseButtonDown/Up/Click` 分发与 `pressed_` 生命周期
- [x] T10 `Renderer` 帧状态：保留 `frame_acquired_`，补索引范围检查（进一步 API 校验留待 M2）

## P1 布局与 UI

- [x] T11 `Vbox` measure spacing/padding 修正
- [x] T12 `UIRoot` 移除演示内容，明确自由画布语义，修正 measure/arrange
- [x] T13 `Widget::parent()` 改非拥有指针；`add()` 设置 parent
- [x] T14 拆掉 `rect.hpp` / `widget.hpp` 循环 include
- [x] T15 `layout_dirty_` + `UISystem::tick` 条件重排
- [x] T16 `ScrollView` arrange 后 clamp offset
- [ ] T17 文本测量 API 雏形（延后到 M2）
- [ ] T18 HiDPI 坐标约定（延后到 M2）

## 工程化

- [x] T19 `find_package(Vulkan)` 提前
- [x] T20 CMake 声明 GLM（`find_package(glm CONFIG REQUIRED)`）
- [x] T21 STB 固定 commit `003501f`
- [x] T22 slangc/glslc `find_program` + fail fast
- [x] T23 Wayland/X11 CMake option
- [x] T24 `assets/` 统一与运行时定位（`asset_path`）
- [x] T25 字体图集纳入 `assets/fonts/`
- [x] T26 删除 `ex0/ex1/ex3`、`compile.sh/.bat`、`font_renderer.*`
- [x] T27 `cosine_backend` 静态库
- [x] T28 开启警告 `-Wall -Wextra -Wshadow`，清理自身警告
- [x] T29 `scripts/ci-linux.sh` / `scripts/ci-windows.ps1`
- [x] T30 布局测试 `test_ui_layout` + ctest 注册

## 验收

- [x] T31 重新 configure 成功；`scripts/ci-linux.sh` 在全新 `build-ci/` 下完成构建与测试
- [x] T32 `ex4` 从 `/tmp` 启动成功，短时运行无 validation error
- [x] T33 `ctest` 4/4 通过
- [x] T34 更新 code_review 勾选状态
- [x] T35 用户验收后 `sele close 20261024-m1-stability`（已执行，SPEC 已折叠）

## 遗留 / 后续

- ScrollView 渲染裁剪栈（M2）
- 文本测量 / UTF-8 / HiDPI（M2）
- 干净目录的完整 after-clone 构建需在 M3 CI 中复验（当前依赖已缓存的 FetchContent）
