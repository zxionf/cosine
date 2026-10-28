# anim 线任务

> 定位：`scene` / `property` / `timeline` / `easing` / `player`，以及 XEL 宿主侧
> （根目录 `interpreter/` 由本线管理，未来可独立成线）。
> 目录：`src/anim/`、`interpreter/`。编号前缀 `AN`。

## 进行中

（无。AN-3 尚未开工。）

## 待开始

### AN-3 M5 XEL 绑定

- **状态**：`todo`
- **目标**：解释器从独立可执行改成可嵌入库，脚本能驱动场景与时间轴
- **验收**：XEL 脚本创建多节点动画并播放；错误脚本给出行/列可读报错且不崩溃
- **依赖**：无（`examples` 线负责写演示，见 `EX-3`）
- **步骤**：
  - [ ] `interpreter/` → `xel_core` 静态库（保留现有可执行作为 CLI 入口）
  - [ ] 宿主 API：创建/查找节点、设置属性、创建轨道与关键帧、播放控制
  - [ ] 值互转：宿主对象 ↔ XEL 值；错误不崩溃
  - [ ] 脚本生命周期：加载、执行、错误定位（行/列）
  - [ ] 类型系统边界决策（动态类型是否够用，见 `docs/risks.md` 未决问题 2）
  - [ ] 单元测试：宿主 API 正常路径 + 错误路径

## 已完成

| 编号 | 内容 | 完成 | 记录 |
|---|---|---|---|
| AN-1 | M4 动画内核（scene / property / timeline / easing / player，确定性 seek 与事件） | 2026-10-26 | `docs/plan_archive.md` → M4 |
| AN-2 | M4.5 `ui_anim_showcase`（缓动对比、层级累积、属性阶跃） | 2026-10-26 | `docs/plan_archive.md` → M4.5 |
