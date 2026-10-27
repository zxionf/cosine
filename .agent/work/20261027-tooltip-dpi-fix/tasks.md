# 20261027-tooltip-dpi-fix 任务清单

- [x] T01 gallery：光标坐标除以 `ui_scale` 再交给 tooltip
- [x] T02 冒烟：`COSINE_UI_SCALE=2 ./ui_gallery` exit=0 / validation=0
- [x] T03 回归：ctest 51/51（build + ASAN）
- [ ] T04 决策记录、close

## 完成定义

- 非 1 倍缩放下 tooltip 位置与鼠标一致，且不随倍率漂移。
