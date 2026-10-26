# 20261026-anim-demo 交接

## 结果

- 新增 `examples/ui_anim_demo`（target `ui_anim_demo`，链接 `cosine_app` + `cosine_anim`）：
  - 舞台内 5 条轨道：Linear / EaseIn / EaseOut / EaseInOut / 自定义 cubic-bezier；
  - 每条一个绝对定位 `ColorRect`，位置 = `origin + ease(t) × span`；
  - Pause/Play、Reset、总进度条与时间标签；周期 3s 循环。
- 修复绝对定位漂移：`Widget::absolute_pos_` + `set_absolute_pos()`；
  `Vbox`/`Hbox`/`Grid`/`Wrap` 改为读 `absolute_pos()`。
- `ColorRect::set_color()`（示例着色 + 后续动画用）。
- `test_ui_absolute_zindex` 新增「多次 arrange 不漂移」用例。

## 验证

- `ctest` 43/43（build 与 ASAN）。
- `ui_anim_demo` 与 `ui_gallery` 冒烟均 `validation=0`。
- 截图：五条轨道方块按各自曲线分布、进度条与时间标签同步。
- `sele close` 为 `appended`（`DECISIONS #113-114`）。

## 运行

```bash
cd build/examples && ./ui_anim_demo
```

## 下一步

M4 `property-system`：`PropertyId` 注册表 + `variant` 类型擦除读写，
按节点类型注册属性（transform + size/color/text/text_size），作为时间轴/轨道的地基。

## 注意

- 绝对定位一律用 `set_absolute_pos()`，不要用 `set_pos()`（后者会被 arrange 改写成全局坐标）。
- `absolute_pos_` 与 `local_.pos` 是两个概念：前者是相对父内容区的偏移，后者是当前全局位置。
