# 20261024-m2-gui-mvp 任务清单

## M2-A 事件系统（先做）

- [x] A01 `InputEvent` 补全 MouseDown/Up/Click/Move/Scroll/KeyDown/KeyUp/Char 字段与语义
- [x] A02 `Widget` 的鼠标/键盘处理返回 `bool`，支持消费
- [x] A03 `UISystem` 命中路径 + 从深到浅冒泡
- [x] A04 鼠标捕获：按下目标固定，抬起释放；同目标触发 click
- [x] A05 焦点：`focused_`、`set_focus`、`on_focus_gained/lost`
- [x] A06 Tab / Shift+Tab 在可聚焦控件间循环
- [x] A07 `Application` 注册 key / char 回调并分发
- [x] A08 事件/焦点纯逻辑测试（不依赖 Vulkan）

## M2-B 布局

- [x] B01 `Hbox`（与 Vbox 对称）
- [x] B02 `Spacer`
- [x] B03 padding/margin 统一到 Widget/容器（Edges + 容器应用子项 margin）
- [x] B04 尺寸策略：固定、stretch、grow/flex、min/max
- [x] B05 主轴分布 start/center/end/space-between（Vbox/Hbox）
- [x] B06 measure 缓存与 dirty 传播（Vbox/Hbox）
- [x] B07 ScrollView 纵向/横向/双轴（滚动条暂未绘制）
- [x] B08 布局测试覆盖 Vbox/Hbox/Spacer/ScrollView/嵌套、margin、flex、min/max、缓存

## M2-C 控件与主题

- [ ] C01 `Theme` 结构与深色主题
- [ ] C02 Panel / Frame
- [ ] C03 Label / 基础多行文本
- [ ] C04 Button（normal/hover/pressed/disabled + 键盘触发）
- [ ] C05 Checkbox / Toggle
- [ ] C06 Slider（float/int）
- [ ] C07 TextInput（单行：光标、选择、删除、输入）
- [ ] C08 ScrollView / List
- [ ] C09 Splitter
- [ ] C10 Tooltip

## M2-D 文本

- [ ] D01 UTF-8 解码
- [ ] D02 字形 advance / line height / 文本宽度
- [ ] D03 自动换行 / 裁剪 / 省略号
- [ ] D04 字体图集格式文档化
- [ ] D05 CJK 字体来源与 FreeType 生成方案（系统字体或仓库字体）
- [ ] D06 多字号或按需光栅化

## M2-E 渲染与裁剪

- [ ] E01 统一 draw list / DrawCmd
- [ ] E02 裁剪栈（容器 push/pop scissor）
- [ ] E03 按 texture/scissor 合批
- [ ] E04 图层 / z-order，弹窗置顶
- [ ] E05 render target 抽象占位
- [ ] E06 渲染层测试或 validation 冒烟

## M2 验收

- [ ] V01 GUI demo：面板、按钮、复选框、滑块、文本输入、滚动列表
- [ ] V02 Tab 焦点 + 回车/空格触发按钮
- [ ] V03 1x / 1.5x / 2x HiDPI 表现一致
- [ ] V04 60 秒交互 + Vulkan validation 无错误
- [ ] V05 布局 / 事件 / 文本测量测试全绿
- [ ] V06 用户验收后 `sele close 20261024-m2-gui-mvp`

## 当前进度

- M2-A 完成：事件命中路径、冒泡/消费、鼠标捕获、click、焦点、Tab、key/char、测试
- M2-B 完成：Hbox、Spacer、padding/margin、flex/grow、min/max、主轴分布、measure 缓存、ScrollView 双轴
- 5/5 ctest 通过；`ex4` 可运行
- 下一步：M2-C Theme 与基础控件（先把文本能力接入，再做 Button/Checkbox/Slider/TextInput）
