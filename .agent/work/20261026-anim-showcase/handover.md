# 20261026-anim-showcase 交接

## 阶段计划调整

- M3.5 的 T1–T13 backlog 整段移到新小节 **M4.6 文本 / 图集 / 输入 backlog（未开始）**。
- 新增 **M4.5 动画能力全面展示**，本次已完成并标记。
- 路线总览表补上 M3.6 行（此前遗漏）。

## 结果

- 新增 `examples/ui_anim_showcase`（target `ui_anim_showcase`，链接 `cosine_app` + `cosine_anim`）。
- 三个展区：
  - **A. 缓动**：五条 position 轨道（Linear / EaseIn / EaseOut / EaseInOut / cubic-bezier）；
  - **B. 层级**：Group 做 scale 脉冲，三个子方块跟随（验证父子变换累积）；
  - **C. 属性**：position + scale + color 驱动同一方块；text 阶跃；visible 每秒闪烁。
- 控制：Pause/Play、Back/Fwd 1s、Reset、Speed 0.5/1/2x、Loop 开关、Progress、时间标签。
- 事件：2.0s / 4.0s 更新底部事件标签。

## 验证

- `COSINE_EXIT_AFTER=2 ./ui_anim_showcase`：exit=0、`validation` 计数 0。
- 截图确认三个展区、控制行、进度/时间/事件标签。
- `ctest` 47/47（既有测试未受影响）。
- `sele close` 为 `appended`（`DECISIONS #120`）。

## 注意

- `SceneView` 暂不支持 rotation，展厅因此用 scale/position/color 表达动感。
- 展厅用 Back/Fwd/Reset 代替时间轴滑块：Slider 的 `set_value` 与 `on_change` 解耦放到 M6 编辑器。

## 下一步

- **M5 XEL 绑定**（主线），或按需处理 **M4.6** 的文本/图集 backlog。
