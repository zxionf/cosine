# 20261025-m3-b-input-boundaries 任务清单

## M3-B 输入抽象与头文件边界

- [x] B01 新增 `events/key_code.hpp`：KeyCode、Modifier、has_modifier
- [x] B02 InputEvent/Widget/控件签名改为 KeyCode/Modifier
- [x] B03 Application 负责 GLFW → KeyCode/Modifier 映射
- [x] B04 UI 头文件移除 GLFW/backend Window 依赖；UISystem 前向声明
- [x] B05 更新测试到 KeyCode/Modifier；删除 tester.hpp
- [x] B06 全量构建、ctest、ui_demo validation 冒烟

## 当前进度

- KeyCode/Modifier 已落地，GLFW 键码只在 Application 映射
- UI 头文件移除 GLFW/backend Window 依赖；UISystem 使用前向声明
- 11/11 ctest 通过；ui_demo validation 冒烟正常
