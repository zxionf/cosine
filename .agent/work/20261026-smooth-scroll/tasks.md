# 20261026-smooth-scroll 任务清单

- [x] T01 `src/ui/scroll_mode.hpp`：`ScrollMode { Smooth, Line }`
- [x] T02 `ScrollView`：`target_` + mode/smoothing/speed/line_step 配置，`on_scroll` 改目标
- [x] T03 `ScrollView::on_update(dt)`：帧率无关指数逼近 + 0.25px 吸附 + clamp
- [x] T04 `TreeView` / `Table`：同样的模式与平滑（行步进不变），Line 立即
- [x] T05 `test/test_ui_smooth_scroll.cpp` + 注册到 `test/CMakeLists.txt`
- [x] T06 gallery `COSINE_GALLERY_SCROLL_MODE=line|smooth`
- [x] T07 全量回归：ctest 41/41 + ASAN 41/41 + gallery 冒烟 3 组（validation=0）
- [x] T08 阶段计划新增 M3.6 小节、SPEC 折叠、决策记录

## 完成定义

- 默认平滑滚动，帧率无关、可吸附、不越界；切到 Line 后与迁移前行为一致。
- 平滑期间不触发整树重排（只平移内容）。
- 构建零新增警告，全部测试与 ASAN 通过。
