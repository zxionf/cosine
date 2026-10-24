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
- [x] A09 HiDPI：窗口光标坐标按 framebuffer/window 比例换算，布局使用 framebuffer 尺寸
- [x] A10 组件 debug 绘制（debug_recursive / append_debug_tree）+ on_hover 钩子 + hover 高亮（默认关闭）

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

- [x] C01 `Theme` 结构与深色主题
- [x] C02 Panel / Frame
- [x] C03 Label / 基础多行文本
- [x] C04 Button（normal/hover/pressed/disabled + 键盘触发）
- [x] C05 Checkbox / Toggle
- [x] C06 Slider（float/int）
- [x] C07 TextInput（单行：光标、选择、删除、输入、Ctrl+A）
- [x] C08 ScrollView / List
- [x] C09 Splitter
- [x] C10 Tooltip（独立浮层，自动 hover 触发待接）

## M2-D 文本

- [x] D01 FontAtlas 改 codepoint 维度，支持 UTF-8 解码
- [x] D02 字形 advance / line height / 文本宽度（基础版）
- [x] D03 自动换行 / 最大宽度 / 裁剪 / 省略号
- [ ] D04 字体图集格式与字号策略文档化
- [ ] D05 CJK 字体来源与 FreeType 生成方案（系统字体或仓库字体）
- [ ] D06 多字号或按需光栅化 / 评估 SDF
- [x] D07 TextInput 按 codepoint 维护光标/选择，修复水平滚动后的点击索引
- [x] D08 FontAtlas / TextInput / 换行测试

## M2-E 渲染与裁剪

- [ ] E01 统一 draw list / DrawCmd
- [ ] E02 裁剪栈（容器 push/pop scissor）
- [ ] E03 ScrollView / TextInput / Panel 自动裁剪
- [ ] E04 按 texture/scissor 合批
- [ ] E05 图层 / z-order，弹窗/Tooltip 置顶
- [ ] E06 render target 抽象占位
- [x] E07 validation 冒烟（ex4 短时运行）
- [ ] E08 裁剪 / draw list 测试

## M2-F 应用层与 demo 收口

- [ ] F01 删除 ex4 手动 scroll->on_scroll/measure/arrange，统一走 UISystem
- [ ] F02 Renderer 的 draw_frame / F11 截图 / 空 process_mouse_move 移出应用层
- [ ] F03 形成 examples/ui_demo 作为 M2 验收对象
- [ ] F04 归档/删除 ex2 与未使用资源/代码
- [ ] F05 清理后的 demo 连续 60 秒 validation 无错误
- [ ] F06 demo / UI 集成测试

## M2 验收

- [ ] V01 GUI demo：面板、按钮、复选框、滑块、文本输入、滚动列表
- [x] V02 Tab 焦点 + 回车/空格触发按钮
- [ ] V03 1x / 1.5x / 2x HiDPI 表现一致
- [ ] V04 60 秒交互 + Vulkan validation 无错误
- [ ] V05 布局 / 事件 / 文本测量 / 裁剪测试全绿
- [ ] V07 文本支持 UTF-8 + 常用 CJK，换行/省略号正常
- [ ] V08 ScrollView / TextInput / Panel 内容不溢出 viewport
- [ ] V06 用户验收后 `sele close 20261024-m2-gui-mvp`

## 当前进度

- M2-A 完成：事件、焦点、HiDPI 光标换算、组件 debug 绘制、on_hover
- M2-B 完成：Hbox、Spacer、padding/margin、flex/grow、min/max、主轴分布、measure 缓存、ScrollView 双轴
- M2-C 完成：Theme、FontAtlas、Label、Panel、Button、Checkbox、Slider、TextInput、List、Splitter、Tooltip；控件已接入 ex4
- M2-D 部分完成：UTF-8、codepoint FontAtlas、换行/省略号、TextInput codepoint 光标/点击索引、文本测试
- 已修复面板不响应：光标坐标按 framebuffer 缩放后与布局坐标一致；debug 树保留但 ex4 已关闭，hover 视觉保留
- 评估与问题已归档到 `docs/project_assessment.md`；R01–R18 已纳入阶段计划
- 7/7 ctest 通过；`ex4` 可运行
- 下一步：M2-D CJK 字体/多字号，然后 M2-E 裁剪栈/draw list
